#ifndef EASYTCPCSERVER_HPP
#define EASYTCPCSERVER_HPP

#define TESTDEBUG ;

#pragma warning(disable : 4996)

#ifdef _WIN32

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Winsock2.h>
#include <Windows.h>

typedef  int socklen_t;

#elif __linux__


#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(-0)
#define SOCKET_ERROR (-1)

#endif //  _WIN32

#include <iostream>
#include <vector>
#include <algorithm>
#include<stdlib.h>



#include "NoAbleCopy.hpp"
#include "Header.hpp"
#include "CELLTimestamp.hpp"

const int SEND_BUFF = 1024; // 发送缓冲区大小
const int RECV_BUFF = 1024; // 接收缓冲区大小

#ifdef _WIN32

#pragma comment(lib, "ws2_32.lib")
#endif // _WIN32

//
class ClientSocket
{
public:
    ClientSocket(SOCKET sockfd ,sockaddr_in clientAddr) :sockfd_(sockfd),address_(clientAddr), lastPos_(0)
    {

        secondBuff_ = (char *)malloc(RECV_BUFF * 10);
    }
    ~ClientSocket() { free(secondBuff_); };
    SOCKET getSocket() { return sockfd_; };
    sockaddr_in getAddr() { return address_; };
    char *secondBuff_; // 第二接受缓冲区
    int lastPos_;      // 消息缓冲区尾部位置

private:
    SOCKET sockfd_; //客户端套接字
    sockaddr_in address_;//客户端地址

};

class EasyTcpServer : public NoAbleCopy
{
public:


private:
    std::vector<ClientSocket*> clientSockes_;    // 保存已经连接的套接字
    SOCKET server_sock_ = INVALID_SOCKET; // 服务端套接字
    SOCKET max_sock_ = INVALID_SOCKET;    // select监视的最大套接字
    sockaddr_in server_addr_;             // 服务器的地址
    char *recvBuff_ = nullptr;            // 接收缓冲区
    char *sendBuff_ = nullptr;            // 发送缓冲区
    bool isRun_ = true;                   // 判断客户端是否继续运行
    int recvPackCount_ = 0;                   //接受到的数据包个数
    CELLTimestamp timeCount_;             //时间计数器

    fd_set fdRead_;                       // 读套接字集合

public:
    EasyTcpServer(std::string ip, unsigned short port);
    ~EasyTcpServer();
    bool Start();   //程序开始
    bool Init();    //初始化资源 
    bool Socket();  //返回服务器套截字
    bool Bind();    //绑定地址
    bool Listen();  //监听套接字
    bool Accept();  //接受socket 的发生情况
    bool Recv(ClientSocket *clientSock); // 接受网络消息
    bool OnNetMsg(ClientSocket *clientSock, DataHeader *header);//处理网络消息
    bool Send(ClientSocket *clientSock,char *Msg,int n);    //向客户端套接字发送n个字节的消息
    char *getSendBuff() { return sendBuff_; };
    char *getRecvBuff() { return recvBuff_; };
   
};

EasyTcpServer::EasyTcpServer(std::string ip, unsigned short port)
{
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    server_addr_.sin_port = htons(port);
};

EasyTcpServer::~EasyTcpServer()
{
    if(nullptr != recvBuff_)
    {
        free(recvBuff_);
    }
    recvBuff_ = nullptr;
    if(nullptr != sendBuff_)
    {
        free(sendBuff_);
    }
    sendBuff_ = nullptr;


#ifdef _WIN_32
    WSACleanup();
    for(auto iter:clientSockes_)
    {
        closesocket(iter->getSocket());
    }
    closesocket(server_sock_);
#elif __linux__
    for(auto iter:clientSockes_)
    {
        close(iter->getSocket());
    }
    close(server_sock_);

#endif

    // 将已接受的客户端释放空间
    for (auto iter : clientSockes_)
    {
        delete iter;
    }
}
// 程序开始
bool EasyTcpServer:: Start()
{
    Init();
    Socket();
    Bind();
    Listen();
    return true;
}

 // 初始化资源
bool EasyTcpServer::Init()
{
    //情况读套接字集合
    FD_ZERO(&fdRead_);
#ifdef _WIN32
    // 1.启动windows socket编程环境
    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    if (0 != WSAStartup(ver, &dat))
    {
        std::cerr << "WSAStartup() error" << std::endl;
    }
#endif // _WIN32

    if(nullptr != recvBuff_ )
    {
        free(recvBuff_);
    }
    recvBuff_ = (char *)malloc(RECV_BUFF);
    if(nullptr == recvBuff_)
    {
        std::cout << "malloc() error" << std::endl;
        return false;
    }
    memset(recvBuff_, 0, RECV_BUFF);

    



    if(nullptr != sendBuff_)
    {
        free(sendBuff_);
    }
    sendBuff_ = (char*)malloc(SEND_BUFF);
    if (nullptr == sendBuff_)
    {
        std::cout << "malloc() error" << std::endl;
        return false;
    }
    memset(sendBuff_, 0, SEND_BUFF);

    return true;
}

// 返回服务器套截字
bool EasyTcpServer::Socket()
{
    server_sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(INVALID_SOCKET == server_sock_)
    {
        std::cerr << "socket() Error" << std::endl;
        return false;
    }
    max_sock_ = server_sock_;
    return true; 
}
// 绑定地址
bool EasyTcpServer::Bind()
{
    int ret = bind(server_sock_, (sockaddr*)&server_addr_, sizeof(server_addr_));
    if(SOCKET_ERROR == ret)
    {
        std::cerr << "bind() Error" << std::endl;
        
        return false;
    }
    return true;
}
// 监听套接字
bool EasyTcpServer:: Listen()
{
    int ret = listen(server_sock_, 5);
    if(SOCKET_ERROR == ret)
    {
        std::cerr << "listen() Error" << std::endl;
        return false;  
    }
    return true;
}

// 接受socket 的发生情况
bool EasyTcpServer:: Accept()
{
    
    while(isRun_)
    {

        FD_ZERO(&fdRead_);

        // 将服务器套接字加入 fdRead_
        FD_SET(server_sock_, &fdRead_);

        // 将其他client套接字加入fdRead_
        for (auto iter : clientSockes_)
        {
            FD_SET(iter->getSocket(), &fdRead_);
            if(max_sock_ < iter->getSocket())
            {
                max_sock_ = iter->getSocket();
            }
        }

        // 开始select
        int ret = select(max_sock_ + 1, &fdRead_, nullptr, nullptr, 0);
        if (ret < 0)
        {
            std::cout << "select() error" << std::endl;
            return false;
        }
        if (FD_ISSET(server_sock_, &fdRead_))
        {

            FD_CLR(server_sock_, &fdRead_);
            

            struct sockaddr_in _clientAddr;
            memset(&_clientAddr, 0, sizeof(struct sockaddr_in));
            SOCKET _clientSock = INVALID_SOCKET;
            int _clientLen = sizeof(_clientAddr);
            _clientSock = accept(server_sock_, (sockaddr *)&_clientAddr,(socklen_t *) &_clientLen);
            if (INVALID_SOCKET == _clientSock)
            {
                std::cerr << "accept() Error" << std::endl;
            }
            else
            {

                std::cout << "client " << _clientSock << " ip is: " << inet_ntoa(_clientAddr.sin_addr) << "port :" << ntohs(_clientAddr.sin_port) << std::endl;
            }
            // 向所有已加入的连接发送new_join
            NEW_JOIN *join = (NEW_JOIN *)sendBuff_;
            join->cmd_ = CMD::CMD_NEWJOIN;
            join->length_ = sizeof(NEW_JOIN);
            join->newUserSock_ = _clientSock;
            memcpy(join->newUserAddr_, inet_ntoa(_clientAddr.sin_addr), g_strLen);
            join->newUserPort = ntohs(_clientAddr.sin_port);

            for (auto iter : clientSockes_)
            {
                if (!Send(iter, (char *)join, join->length_))
                {
                    std::cout << "sercer Send() to client socket :" << _clientSock << " NEW_JOIN Error" << std::endl;
                }
            }

            ClientSocket *newClient = new ClientSocket(_clientSock, _clientAddr);
            clientSockes_.push_back(newClient);
            max_sock_ = _clientSock;
        }
        else 
        {
            for(auto iter:clientSockes_)
            {
                if(FD_ISSET(iter->getSocket(),&fdRead_))
                {
                    Recv(iter);
                }
            }
        }

    }

    return true;
}

// 接受网络消息
bool EasyTcpServer::Recv(ClientSocket *clientSock)
{
    char recvBuff[RECV_BUFF] = {0};

    //首先将缓冲区清零
    memset(recvBuff, 0, RECV_BUFF);
    int recvLen = recv(clientSock->getSocket(), recvBuff, RECV_BUFF, 0);
    if (recvLen <= 0)
    {
        std::cerr << "recv() Error,recv Len = " << clientSock->getSocket() << std::endl;
    }
    // 将内核接受缓冲区的数据移动至第二缓冲区
    memcpy(clientSock->secondBuff_ +clientSock->lastPos_,recvBuff , recvLen);
    clientSock->lastPos_ += recvLen;
    // 判断消息缓冲区的数据是否大于消息头
    if (clientSock->lastPos_ >= sizeof(DataHeader))
    {
        DataHeader *header = (DataHeader *)(clientSock->secondBuff_);
        if (clientSock->lastPos_ >= header->length_)
        {
            OnNetMsg(clientSock, header);
        }
        memcpy(clientSock->secondBuff_ , clientSock->secondBuff_ + header->length_, header->length_);
        clientSock->lastPos_ -= header->length_;
    }
    return true;
}
// 处理网络消息 bool OnNetMsg(ClientSocket *clientSock, DataHeader *header);//处理网络消息
bool EasyTcpServer::OnNetMsg(ClientSocket *clientSock, DataHeader *header)
{

    //响应消息时计数器+1
    ++recvPackCount_;
    if(timeCount_.getElapsedSecond() >= 1.0)
    {
        std::cout << "socket " << clientSock->getSocket() << " Time: " << timeCount_.getElapsedSecond() << " recv Packge:" << recvPackCount_ << std::endl;
        recvPackCount_ = 0;
        timeCount_.updata();
    }
    switch (header->cmd_)
    {
    case CMD::CMD_LOGIN:
    {
        // 登录逻辑判断不做了
        std::cout << "client socket:" << clientSock->getSocket() << "client ip:" << inet_ntoa(clientSock->getAddr().sin_addr) << " port:" << ntohs(clientSock->getAddr().sin_port) << " LOGIN" << std::endl;

        LOGIN_RESULT result;
        memset(&result, 0, sizeof(LOGIN_RESULT));
        result.cmd_ = CMD::CMD_LOGIN_RESULT;
        result.result_ = true;
        result.length_ = sizeof(LOGIN_RESULT);
        Send(clientSock, (char *)&result, result.length_);
    }


        break;
    case CMD::CMD_LOGOUT:
    {
        // 登录逻辑判断不做了
        LOGOUT_RESULT result;
        memset(&result, 0, sizeof(LOGOUT_RESULT));
        result.cmd_ = CMD::CMD_LOGOUT_RESULT;
        result.result_ = true;
        result.length_ = sizeof(LOGOUT_RESULT);
        Send(clientSock, (char *)&result, result.length_);
    }

        break;
    case CMD::CMD_QUIT:
    {
        isRun_ = false;
    }

        break;
    default:
        break;
    }
    return true;
}
// 向客户端套接字发送n个字节的消息
bool EasyTcpServer::Send(ClientSocket * clientSock, char *Msg, int n)
{
    if(nullptr == Msg)
    {
        std::cerr << "Send() Error,send message is a nullptr" << std::endl;
        return false;
    }
    int sendLen = send(clientSock->getSocket(), Msg, n, 0);
    if(sendLen <= 0)    //client断开连接
    {
        if(sendLen == 0)
        {
            std::cout << "client socket :" << clientSock << " close " << std::endl;
        }else{
            std::cout << "server Send() message to client Socket " << clientSock << " Failed" << std::endl;
            perror("send() return -1");
        }

        auto iter = std::find(clientSockes_.begin(), clientSockes_.end(), clientSock);
        if(iter == clientSockes_.end())
        {
            std::cout << "find socket in vector name clientSockes failed" << std::endl;
        }
        else
        {
            clientSockes_.erase(iter);
            FD_CLR(clientSock->getSocket(), &fdRead_);
#ifdef _WIN32
            closesocket(clientSock->getSocket());

#elif __linux__
            close(clientSock->getSocket());
#endif
            delete clientSock;
            //memset(sendBuff_, 0, SEND_BUFF);
            return false;
        }
        
    }
    std::cout << "server suceess send " << sendLen << " bytes to clien socket :" << clientSock << std::endl;
    return true;
}
#endif // EASYTCPCSERVER_HPP  