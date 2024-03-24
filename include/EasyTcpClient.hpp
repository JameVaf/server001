#ifndef EASYTCPCLIENT_HPP
#define EASYTCPCLIENT_HPP

#pragma warning(disable : 4996)
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Winsock2.h>
#include <Windows.h>

#elif __linux__
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(-0)
#define SOCKET_ERROR (-1)
#endif //  _WIN32

#include <iostream>
#include <thread>

#include "NoAbleCopy.hpp"
#include "Header.hpp"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif // _WIN32



#define TEST_DEBUG



// 客户端
class EasyTcpClient : public NoAbleCopy
{
public:
    EasyTcpClient(const std::string ip, const std::string port);
    ~EasyTcpClient();
    bool Start();   // 开启服务器
    bool Stop();    // 关闭服务器
    bool Send(char* sendMsg,int n);//发送消息
    void Recv();   //接受消息
    SOCKET getSock();   //得到服务器的套接字
    bool getIsRun();    //得到服务器是否运行
    bool Init();        // 资源初始化
    bool Connect();     // 连接目标服务器
    bool Select();         //select等待客户端接受数据

private:


    bool OnNet(DataHeader *header); // 处理网络消息

public:
    const int SEND_BUFF = 1024; // 发送缓冲区大小
    const int RECV_BUFF = 1024; // 接收缓冲区大小

private:
    char *recvBuff_ = nullptr;         // 接收缓冲区
    char *sendBuff_ = nullptr;         // 发送缓冲区
    SOCKET serverSock_ = INVALID_SOCKET;      // 服务器的套接字
    sockaddr_in serverAddr_; // 服务器的地址
    bool isRun_ = true;            // 判断客户端是否继续运行
    char *secondRecvBuff_ = nullptr;  //第二接收缓冲区
    int lastPost_ = 0;              //接收缓冲区的标志位

};


//构造函数
EasyTcpClient::EasyTcpClient(const std::string ip, const std::string port) : 
recvBuff_(nullptr),
sendBuff_(nullptr),
secondRecvBuff_(nullptr),
serverSock_(INVALID_SOCKET),
lastPost_(0),
isRun_(true)
{
    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr_.sin_port = htons((unsigned short)(atoi(port.c_str())));
};


//初始化资源
bool EasyTcpClient::Init()
{
#ifdef _WIN32
    // 1.启动windows socket编程环境
    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    if (0 != WSAStartup(ver, &dat))
    {
        std::cerr << "WSAStartup() error" << std::endl;
    }
#endif // _WIN32

    if(sendBuff_)
    {
        free(sendBuff_);
        sendBuff_ = nullptr;
    }
    sendBuff_ = (char*)malloc(SEND_BUFF);

    if(recvBuff_)
    {
        free(recvBuff_);
        recvBuff_ = nullptr;
    }
    recvBuff_ = (char *)malloc(RECV_BUFF);

    if(secondRecvBuff_)
    {
        free(secondRecvBuff_);
        secondRecvBuff_ = nullptr;
    }
    secondRecvBuff_ = (char *)malloc(RECV_BUFF * 10);

    serverSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(INVALID_SOCKET == serverSock_)
    {
        std::cerr << "socket() Error" << std::endl;
        std::cout << serverSock_ << std::endl;
        return false;
    }
    return true;
}

//析构函数
EasyTcpClient::~EasyTcpClient()
{
    if(sendBuff_)
    {
        free(sendBuff_);
    }
    if(recvBuff_)
    {
        free(recvBuff_);
    }
    if(secondRecvBuff_)
    {
        free(secondRecvBuff_);
    }
#ifdef _WIN32
    closesocket(serverSock_);
#elif __linux__
    close(serverSock_);
#endif // _WIN32
    serverSock_ = INVALID_SOCKET;
#ifdef _WIN32
    // 
    WSACleanup();
#endif // _WIN32
}

bool EasyTcpClient::Connect()
{
    int ret = connect(serverSock_, (sockaddr *)&serverAddr_, sizeof(serverAddr_));
    if (SOCKET_ERROR == ret)
    {
        std::cerr << "connect Error()" << std::endl;
        return false;
    }
    else
    {
#ifdef TESTDEBUG
        std::cout << "connct() sucess" << std::endl;
#endif
    }

   
    return true;
}

bool EasyTcpClient:: Select()
{
    while (isRun_)
    {
        fd_set fdRead;
        FD_ZERO(&fdRead);
        FD_SET(serverSock_, &fdRead);

        timeval _time = {2, 0};
        int ret = select(serverSock_ + 1, &fdRead, nullptr, nullptr, &_time);
        if (ret < 0)
        {
            std::cout << "select error" << std::endl;
        }
        if (FD_ISSET(serverSock_, &fdRead)) // 服务器发送数据到
        {
            Recv();
        }
    }
}
bool EasyTcpClient::Start()
{
     Init();
    //Connect();
    return true;
};
bool EasyTcpClient::Stop()
{
    this->isRun_ = false;
    return true;
}

bool EasyTcpClient::Send(char *sendMsg,int n)
{
    memcmp(sendBuff_, sendMsg, n);
    int sendLen = send(serverSock_, sendBuff_, n, 0);
    if(sendLen <= 0)
    {
        std::cerr << "Send Server Error from client " << std::endl;
        return false;
    }
    return true;
}
void EasyTcpClient::Recv()
{
    //memset(recvBuff_, 0, RECV_BUFF);
    int _recvLen = recv(serverSock_, recvBuff_, RECV_BUFF, 0);
    if(_recvLen <= 0)
    {
        std::cerr << "recv() Error,recv Len = " <<_recvLen<< std::endl;
    }
    //将内核接受缓冲区的数据移动至第二缓冲区
    memcpy(secondRecvBuff_+lastPost_, recvBuff_, _recvLen);
    lastPost_ += _recvLen;
    //判断消息缓冲区的数据是否大于消息头
    if(lastPost_ >= sizeof(DataHeader))
    {
        DataHeader *header = (DataHeader *)secondRecvBuff_;
        if (lastPost_ >= header->length_)
        {
            OnNet(header);
        }
        memcpy(secondRecvBuff_,secondRecvBuff_+header->length_,header->length_);
        lastPost_ -= header->length_;
    }

};

bool EasyTcpClient::OnNet(DataHeader *header)
{
    //将第二缓冲区的数据处理
    //先读取头部信息
    switch (header->cmd_)
    {
    case CMD::CMD_LOGIN_RESULT:
    {
   
 
        if (((LOGIN_RESULT*)header)->result_)
        {
            std::cout << "登录成功" << std::endl;
        }
        else
        {
            std::cout << "登录失败" << std::endl;
        }
    }
    break;
    case CMD::CMD_LOGOUT_RESULT:
    {

        if (((LOGOUT_RESULT *)header)->result_)
        {
            std::cout << "登出成功" << std::endl;
        }
        else
        {
            std::cout << "登出失败" << std::endl;
        }
    }
    break;
    case CMD::CMD_NEWJOIN :{
        std::cout << "new sock:" << ((NEW_JOIN *)header)->newUserSock_ << " join ip:" << ((NEW_JOIN *)header)->newUserAddr_ << " port:" << ((NEW_JOIN *)header)->newUserPort << std::endl;
    } break;
        default:
    {
        std::cout << "from server Error" << std::endl;
        return false;
    }
    break;

    }
    return true;
}


//返回服务器的socket
SOCKET EasyTcpClient::getSock()
{
    return this->serverSock_;
}

//得到服务器是否继续运行
bool EasyTcpClient::getIsRun()
{
    return this->isRun_;
}
#endif // EASYTCPCLIENT_HPP