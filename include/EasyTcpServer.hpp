#ifndef EASYTCPCSERVER_HPP
#define EASYTCPCSERVER_HPP

//#define TEST_DEBUG ;

#pragma warning(disable : 4996)

#ifdef _WIN32

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Winsock2.h>
#include <Windows.h>

typedef int socklen_t;

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
#include <thread>
#include <stdlib.h>
#include <mutex>
#include<atomic>

#include "NoAbleCopy.hpp"
#include "Header.hpp"
#include "CELLTimestamp.hpp"

const int SEND_BUFF = 1024; // 发送缓冲区大小
const int RECV_BUFF = 1024; // 接收缓冲区大小
const int CELL_SERVER_THREAD_COUNT = 4;

#ifdef _WIN32

#pragma comment(lib, "ws2_32.lib")
#endif // _WIN32

//
class ClientSocket
{
public:
    ClientSocket(SOCKET sockfd, sockaddr_in clientAddr) : sockfd_(sockfd),
                                                          address_(clientAddr),
                                                          lastPos_(0)
    {
        secondBuff_ = (char *)malloc(RECV_BUFF * 10);
        if(secondBuff_ == nullptr)
        {
            std::cerr << "class ClientSocket malloc secondBuff_ Failed" << std::endl;
        }
 
       
    }
    ~ClientSocket() { free(secondBuff_); };
    SOCKET getSocket() { return sockfd_; };
    sockaddr_in getAddr() { return address_; };
    char *secondBuff_; // 第二接受缓冲区
    int lastPos_;      // 消息缓冲区尾部位置

private:
    SOCKET sockfd_;       // 客户端套接字
    sockaddr_in address_; // 客户端地址
};







// 专门处理数据的线程
class CellServer :NoAbleCopy
{
public:
    std::atomic_int recvPackCount_ = 0;

private:
    SOCKET sock_;                             // 服务器的套截字
    std::vector<ClientSocket *> clients_;     // 正式客户队列
    std::vector<ClientSocket *> clientsBuff_; // 缓冲客户端队列
    std::mutex mutex_;                        // 互斥锁
    
    CELLTimestamp timeCount_; // 时间计数器
    bool isRun_ = true;
    std::thread *thread_ = nullptr; // 管理CellServer的线程
public:
    CellServer(SOCKET sock = INVALID_SOCKET)
    {
        sock_ = sock;
    }
    ~CellServer()
    {
        sock_ = INVALID_SOCKET;
    }

    // 关闭套接字
    void Close()
    {
        if (sock_ != INVALID_SOCKET)
        {
            // 关闭连接的套接字
            for (auto iter : clients_)
            {
#ifdef _WIN32
                closesocket(iter->getSocket());
#elif __linux__
                close(iter->getSocket());
#endif
            }
#ifdef _WIN32
            closesocket(sock_);
#elif __linux__
            close(sock_);
#endif
            sock_ = INVALID_SOCKET;
        }
    }

    // 是否工作中
    bool isRun()
    {
        return isRun_;
    }

    // 添加连接到缓冲队列
    void addClient(ClientSocket *pClient)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        clientsBuff_.push_back(pClient);
    };

    // 处理网络消息
    bool OnRun()
    {
#ifdef TEST_DEBUG
        std::cout << "CellServer OnRun()" << std::endl;
#endif
        while(isRun())
        {
            //将缓冲队列的客户端加入到客户端队列
            if (clientsBuff_.size() > 0)
            {
                std::lock_guard<std::mutex> guard(mutex_);
                for (auto iter : clientsBuff_)
                {
                    clients_.push_back(iter);
                }
                clientsBuff_.clear();
            }

            //判断是否有客户端
            if(clients_.size() == 0)
            {
                //等待一毫秒
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            // 描述符集合(socket)集合
            fd_set fdRead; 
            fd_set fdWrite;
            fd_set fdExp;
            // 清理集合
            FD_ZERO(&fdRead);
            FD_ZERO(&fdWrite);
            FD_ZERO(&fdExp);
            // 将server套接字加入集合
            FD_SET(sock_, &fdRead);
            FD_SET(sock_, &fdWrite);
            FD_SET(sock_, &fdExp);

            // 将客户端套接字队列加入字符集,同时找到最大的套接字
            SOCKET maxSock = sock_;
            for(auto iter:clients_)
            {
                FD_SET(iter->getSocket(), &fdRead);
                FD_SET(iter->getSocket(), &fdWrite);
                FD_SET(iter->getSocket(), &fdExp);
                //将client socket加入字符集
                if (iter->getSocket() > maxSock)
                {
                    maxSock = iter->getSocket();
                }
            }


            //开始select
            timeval t = {1, 0};
            int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
            if (ret < 0)
            {
                std::cout << "select over" << std::endl;
                Close();
                return false;
            }
            for(auto iter:clients_)
            {   
                if(FD_ISSET(iter->getSocket(),&fdRead))
                {
                    recvData(iter);
                }
            }
            
        }

        return false;
    }
    // 接受网络消息
    bool recvData(ClientSocket *clientSock)
    {
#ifdef TEST_DEBUG
        std::cout << "CellServer recvData()" << std::endl;
#endif
        if (!isRun_)
        {
#ifdef TEST_DEBUG
            std::cout << "isRun_ :value is false,Stop RECV" << std::endl;
#endif
            return false;
        }
        char recvBuff[RECV_BUFF] = {0};

        // 首先将缓冲区清零
        memset(recvBuff, 0, RECV_BUFF);
        int recvLen = recv(clientSock->getSocket(), recvBuff, RECV_BUFF, 0);
        if (recvLen < 0)
        {
            //说明连接的套接字关闭了
            std::cerr << "socket" << clientSock->getSocket() << "recv() Error,recv Len = " << recvLen << std::endl;
            //这里清理套接字
            return false;
        }
        // 将内核接受缓冲区的数据移动至第二缓冲区
        memcpy(clientSock->secondBuff_ + clientSock->lastPos_, recvBuff, recvLen);
        clientSock->lastPos_ += recvLen;
        // 判断消息缓冲区的数据是否大于消息头
        if (clientSock->lastPos_ >= sizeof(DataHeader))
        {
            DataHeader *header = (DataHeader *)(clientSock->secondBuff_);
            if (clientSock->lastPos_ >= header->length_)
            {
                OnNetMsg(clientSock, header);
            }
            memcpy(clientSock->secondBuff_, clientSock->secondBuff_ + header->length_, header->length_);
            clientSock->lastPos_ -= header->length_;
        }
        return true;
    }

    // 处理网络消息 bool OnNetMsg(ClientSocket *clientSock, DataHeader *header);//处理网络消息
    bool OnNetMsg(ClientSocket *clientSock, DataHeader *header)
    {
#ifdef TEST_DEBUG
        std::cout << "CellServer OnNetMsg()" << std::endl;
#endif
       
        if (!isRun_)
        {
#ifdef TEST_DEBUG
            std::cout << "isRun_ :value is false,Stop OnNetMsg" << std::endl;
#endif
            return false;
        }

        // 响应消息时计数器+1
        ++recvPackCount_;
        if (timeCount_.getElapsedSecond() >= 1.0)
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
            // std::cout << "client socket:" << clientSock->getSocket() << "client ip:" << inet_ntoa(clientSock->getAddr().sin_addr) << " port:" << ntohs(clientSock->getAddr().sin_port) << " LOGIN" << std::endl;

            LOGIN_RESULT result;
            memset(&result, 0, sizeof(LOGIN_RESULT));
            result.cmd_ = CMD::CMD_LOGIN_RESULT;
            result.result_ = true;
            result.length_ = sizeof(LOGIN_RESULT);
            // Send(clientSock, (char *)&result, result.length_);
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
            // Send(clientSock, (char *)&result, result.length_);
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

    void Start()
    {
        #ifdef TEST_DEBUG
        std::cout << "CellServer Start()" << std::endl;
#endif
       
        thread_ = new std::thread(std::mem_fun(&CellServer::OnRun), this);

    }

    size_t getClientCount()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        return clients_.size() + clientsBuff_.size();
    }


};



class EasyTcpServer : public NoAbleCopy
{
public:
private:
    std::vector<ClientSocket *> clientSockes_; // 保存已经连接的套接字
    std::vector<CellServer *> cellServers_; //保存处理客户端连接的CellServer队列
    SOCKET server_sock_ = INVALID_SOCKET; // 服务端套接字
    SOCKET max_sock_ = INVALID_SOCKET;    // select监视的最大套接字
    sockaddr_in server_addr_;             // 服务器的地址
    char *recvBuff_ = nullptr;            // 接收缓冲区
    char *sendBuff_ = nullptr;            // 发送缓冲区
    bool isRun_ = true;                   // 判断客户端是否继续运行
    int recvPackCount_ = 0;               // 接受到的数据包个数
    CELLTimestamp timeCount_;             // 时间计数器
    fd_set fdRead_;                      // 读套接字集合

public:
    EasyTcpServer(std::string ip, unsigned short port);
    ~EasyTcpServer();
    void addClientToCellServer(ClientSocket *pClient);  //添加连接到消费者队列
    bool Start();                                                // 程序开始
    bool Init();                                                 // 初始化资源
    bool Socket();                                               // 返回服务器套截字
    bool Bind();                                                 // 绑定地址
    bool Listen();                                               // 监听套接字
    bool Accept();                                               // 接受socket 的发生情况
    bool Recv(ClientSocket *clientSock);                         // 接受网络消息
    bool OnNetMsg(ClientSocket *clientSock, DataHeader *header); // 处理网络消息
    bool Send(ClientSocket *clientSock, char *Msg, int n);       // 向客户端套接字发送n个字节的消息
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

    //清空开辟的内存
    if (nullptr != recvBuff_)
    {
        free(recvBuff_);
    }
    recvBuff_ = nullptr;
    if (nullptr != sendBuff_)
    {
        free(sendBuff_);
    }
    sendBuff_ = nullptr;
//关闭套接字
#ifdef _WIN_32
    WSACleanup();
    for (auto iter : clientSockes_)
    {
        closesocket(iter->getSocket());
    }
    closesocket(server_sock_);
#elif __linux__
    for (auto iter : clientSockes_)
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
bool EasyTcpServer::Start()
{

    Init();
    Socket();
    Bind();
    Listen();
    
    for (int i = 0; i < CELL_SERVER_THREAD_COUNT; ++i)
    {
        CellServer *iter = new CellServer(server_sock_);
        cellServers_.push_back(iter);
        iter->Start();
    }
    Accept();//服务器开始接受
    return true;
}

// 初始化资源
bool EasyTcpServer::Init()
{
    // 清空读套接字集合
    FD_ZERO(&fdRead_);
#ifdef _WIN32
    // 1.启动windows socket编程环境
    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    if (0 != WSAStartup(ver, &dat))
    {
        std::cerr << "WSAStartup() error" << std::endl;
        return false;
    }
#endif // _WIN32

    if (nullptr != recvBuff_)
    {
        free(recvBuff_);
    }
    recvBuff_ = (char *)malloc(RECV_BUFF);
    if (nullptr == recvBuff_)
    {
        std::cout << "malloc recvBUff_ Failed" << std::endl;
        isRun_ = false;
        return false;
    }
    memset(recvBuff_, 0, RECV_BUFF);

    if (nullptr != sendBuff_)
    {
        free(sendBuff_);
    }
    sendBuff_ = (char *)malloc(SEND_BUFF);
    if (nullptr == sendBuff_)
    {
        std::cout << "malloc sendBUff_ Failed" << std::endl;
        isRun_ = false;
        return false;
    }
    memset(sendBuff_, 0, SEND_BUFF);

#ifdef TEST_DEBUG
    std::cout << "Init success" << std::endl;
#endif

    return true;
}

// 返回服务器套截字
bool EasyTcpServer::Socket()
{
    if (!isRun_)
    {
#ifdef TEST_DEBUG
        std::cout << "isRun_ :value is false,Stop Socket" << std::endl;
#endif
        return false;
    }
    server_sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == server_sock_)
    {
        std::cerr << "socket() Error" << std::endl;
        return false;
    }
    max_sock_ = server_sock_;
#ifdef TEST_DEBUG
    std::cout << "Socket success" << std::endl;
#endif
    return true;
}
// 绑定地址
bool EasyTcpServer::Bind()
{
    if (!isRun_)
    {
#ifdef TEST_DEBUG
        std::cout << "isRun_ :value is false,Stop Bind" << std::endl;
#endif
        return false;
    }
    int ret = bind(server_sock_, (sockaddr *)&server_addr_, sizeof(server_addr_));
    if (SOCKET_ERROR == ret)
    {
        std::cerr << "bind() Error" << std::endl;

        return false;
    }
#ifdef TEST_DEBUG
    std::cout << "Bind success" << std::endl;
#endif
    return true;
}
// 监听套接字
bool EasyTcpServer::Listen()
{
    if (!isRun_)
    {
#ifdef TEST_DEBUG
        std::cout << "isRun_ :value is false,Stop Listen" << std::endl;
#endif
        return false;
    }

    int ret = listen(server_sock_, 5);
    if (SOCKET_ERROR == ret)
    {
        std::cerr << "listen() Error" << std::endl;
        return false;
    }
#ifdef TEST_DEBUG
    std::cout << "Listen success" << std::endl;
#endif
    return true;
}

// 接受socket 的发生情况
bool EasyTcpServer::Accept()
{
    if (!isRun_)
    {
#ifdef TEST_DEBUG
        std::cout << "isRun_ :value is false,Stop Accept " << std::endl;
#endif
        return false;
    }
    while (isRun_)
    {

        FD_ZERO(&fdRead_);

        // 将服务器套接字加入 fdRead_
        FD_SET(server_sock_, &fdRead_);

        // 将其他client套接字加入fdRead_
        for (auto iter : clientSockes_)
        {
            FD_SET(iter->getSocket(), &fdRead_);
            if (max_sock_ < iter->getSocket())
            {
                max_sock_ = iter->getSocket();
            }
        }

        // 开始select
        int ret = select(max_sock_ + 1, &fdRead_, nullptr, nullptr, 0);
        if (ret < 0)
        {
            std::cout << "select() error" << std::endl;
            isRun_ = false;
            return false;
        }
        if (FD_ISSET(server_sock_, &fdRead_))
        {

            FD_CLR(server_sock_, &fdRead_);

            struct sockaddr_in _clientAddr;
            memset(&_clientAddr, 0, sizeof(struct sockaddr_in));
            SOCKET _clientSock = INVALID_SOCKET;
            int _clientLen = sizeof(_clientAddr);
            _clientSock = accept(server_sock_, (sockaddr *)&_clientAddr, (socklen_t *)&_clientLen);
            if (INVALID_SOCKET == _clientSock)
            {
                std::cerr << "accept() Error" << std::endl;
            }
            else
            {

              //  std::cout << "client " << _clientSock << " ip is: " << inet_ntoa(_clientAddr.sin_addr) << "port :" << ntohs(_clientAddr.sin_port) << std::endl;
            }


            ClientSocket *newClient = new ClientSocket(_clientSock, _clientAddr);
            addClientToCellServer(newClient);
           
        }


    }

    return true;
}

// 接受网络消息
bool EasyTcpServer::Recv(ClientSocket *clientSock)
{
    if (!isRun_)
    {
#ifdef TEST_DEBUG
        std::cout << "isRun_ :value is false,Stop RECV" << std::endl;
#endif
        return false;
    }
    char recvBuff[RECV_BUFF] = {0};

    // 首先将缓冲区清零
    memset(recvBuff, 0, RECV_BUFF);
    int recvLen = recv(clientSock->getSocket(), recvBuff, RECV_BUFF, 0);
    if (recvLen < 0)
    {
        std::cerr << "socket" << clientSock->getSocket() << "recv() Error,recv Len = " << recvLen << std::endl;
        return false;
    }
    // 将内核接受缓冲区的数据移动至第二缓冲区
    memcpy(clientSock->secondBuff_ + clientSock->lastPos_, recvBuff, recvLen);
    clientSock->lastPos_ += recvLen;
    // 判断消息缓冲区的数据是否大于消息头
    if (clientSock->lastPos_ >= sizeof(DataHeader))
    {
        DataHeader *header = (DataHeader *)(clientSock->secondBuff_);
        if (clientSock->lastPos_ >= header->length_)
        {
            OnNetMsg(clientSock, header);
        }
        memcpy(clientSock->secondBuff_, clientSock->secondBuff_ + header->length_, header->length_);
        clientSock->lastPos_ -= header->length_;
    }
    return true;
}

// 处理网络消息 bool OnNetMsg(ClientSocket *clientSock, DataHeader *header);//处理网络消息
bool EasyTcpServer::OnNetMsg(ClientSocket *clientSock, DataHeader *header)
{
    if (!isRun_)
    {
#ifdef TEST_DEBUG
        std::cout << "isRun_ :value is false,Stop OnNetMsg" << std::endl;
#endif
        return false;
    }

    // 响应消息时计数器+1
    ++recvPackCount_;
    if (timeCount_.getElapsedSecond() >= 1.0)
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
        // std::cout << "client socket:" << clientSock->getSocket() << "client ip:" << inet_ntoa(clientSock->getAddr().sin_addr) << " port:" << ntohs(clientSock->getAddr().sin_port) << " LOGIN" << std::endl;

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
bool EasyTcpServer::Send(ClientSocket *clientSock, char *Msg, int n)
{
    if (nullptr == Msg)
    {
        std::cerr << "Send() Error,send message is a nullptr" << std::endl;
        return false;
    }
    int sendLen = send(clientSock->getSocket(), Msg, n, 0);
    if (sendLen <= 0) // client断开连接
    {
        if (sendLen == 0)
        {
            std::cout << "client socket :" << clientSock << " close " << std::endl;
        }
        else
        {
            std::cout << "server Send() message to client Socket " << clientSock << " Failed" << std::endl;
            perror("send() return -1");
        }

        auto iter = std::find(clientSockes_.begin(), clientSockes_.end(), clientSock);
        if (iter == clientSockes_.end())
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
            // memset(sendBuff_, 0, SEND_BUFF);
            return false;
        }
    }
    // std::cout << "server suceess send " << sendLen << " bytes to clien socket :" << clientSock << std::endl;
    return true;
}

void EasyTcpServer::addClientToCellServer(ClientSocket *pClient)
{
    //主线程将新加入的客户端添加到已连接的客户端队列中
    clientSockes_.push_back(pClient);
    auto minClientCellServer = cellServers_[0];

    //查找客户端数量最少的CellServer
    for (auto iter : cellServers_)
    {
        if(iter->getClientCount() < minClientCellServer->getClientCount())
        {
            minClientCellServer = iter;
        }
    }
    minClientCellServer->addClient(pClient);
   
}
#endif // EASYTCPCSERVER_HPP