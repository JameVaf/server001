#include<iostream>
#include<thread>
#include<chrono>
#include<atomic>

#include   "../include/Header.hpp"
#include "../include/EasyTcpClient.hpp"

void threadSendCmd(EasyTcpClient *client);
void threadInputCmd();
void sendThread(int id);
const int cCount = 1000;    //连接的客户端数量
const int tCount = 4;       //开启的线程数量
EasyTcpClient *clients[cCount];

bool g_isRun = true;
std::atomic_int g_threadCount= 0;
int main()
{
    

    //启动UI线程输入命令
    std::thread UiThread(threadInputCmd);
    UiThread.detach();

    //启动其他线程

    for (int i = 0; i < tCount;++i)
    {
        std::thread t1(sendThread, i + 1);
        t1.detach();
    }

    while (g_isRun)
    {
        /* code */
        Sleep(100);
    }


}

void threadSendCmd(EasyTcpClient *client)
{
    
    char cmdBuff[128] = {0};//128个字节足够缓冲命令
    char tempBuff[client->RECV_BUFF] = {0};
    while(client->getIsRun())
    {
        int sendLen = 0;
        memset(cmdBuff, 0, 128);
        memset(tempBuff, 0, client->RECV_BUFF);
        std::cout <<  "input cmd:";
        std::cin >> cmdBuff;
        if (0 == strcmp(cmdBuff, "quit"))
        {
            ((DataHeader *)tempBuff)->cmd_ =CMD:: CMD_QUIT;
            ((DataHeader *)tempBuff)->length_ = sizeof(DataHeader);
            sendLen = sizeof(DataHeader);
        }
        else if (0 == strcmp(tempBuff, "login"))
        {
            ((LOGIN *)tempBuff)->cmd_ =CMD:: CMD_LOGIN;
            ((LOGIN *)tempBuff)->length_ = sizeof(LOGIN);
            std::cout << "请输入登录的用户名:";
            std::cin >> ((LOGIN *)tempBuff)->name_;
            std::cout << "请输入登录的密码:";
            std::cin >> ((LOGIN *)tempBuff)->passWord_;
            sendLen = sizeof(LOGIN);
        }
        else if (0 == strcmp(tempBuff, "logout"))
        {
            ((LOGOUT *)tempBuff)->cmd_ = CMD::CMD_LOGOUT;
            ((LOGOUT *)tempBuff)->length_ = sizeof(LOGOUT);
            std::cout << "请输入登出的用户名:";
            std::cin >> ((LOGOUT *)tempBuff)->name_;
            sendLen = sizeof(LOGOUT);
        }
        else
        {
            std::cout << "input Error! please input login or logout or quit " << std::endl;
            continue;
        }
        for (int i = 0; i < 1000;++i)
            client->Send(tempBuff, sendLen);
    }
}

void sendThread(int id)
{
    std::cout << "thread id " << id << "start " << std::endl;
    //4个线程 ID:1~4
    int c = cCount / 4;
    int begin = (id - 1) *c;
    int end = id * c - 1;
    std::cout << "start for" << std::endl;
    for (int i = begin; i <= end; ++i)
    {
       
        if(!g_isRun) //判断程序是否继续,否则直接退出
        {
            return;
        }
        std::cout<<"start try"<<std::endl;
        // 开始初始化自己线程负责的客户端连接
        try
        {
            // 连接本地的服务器地址
            std::cout << "thread client count:" << g_threadCount << std::endl;
            clients[i] = new EasyTcpClient("127.0.0.1", "4567");
            ++g_threadCount;
        }
        catch (const std::bad_alloc &e)
        {
            std::cerr << "new EasyTcpClient Failed" << std::endl;
            std::cerr << e.what() << '\n';
            g_isRun = false;
            return;
        }

        //新建的客户端向服务器发送连接请求
        clients[i]->Init();
        clients[i]->Connect();
    }

    // 循环向服务器发送数据循环向服务器发送数据
    LOGIN login;
    login.cmd_ = CMD::CMD_LOGIN;
    memcpy(login.name_, "jame", g_strLen);
    memcpy(login.passWord_, "123456", g_strLen);
    login.length_ = sizeof(LOGIN);

    while (g_isRun)
    {
        for (int i = begin; i <= end; ++i)
        {
            if(g_isRun)
            clients[i]->Send((char*)&login,login.length_);
            else
                break;
        }
    }

    for (int i = begin; i <= end;++i)
    clients[i]->Stop();
}

/// @brief 
void threadInputCmd()
{

    while(g_isRun)
    {
        //由于threadSendCmd 会发生命令,所以UIThread只需显示结果
        char recvBuff[128] = {0};
        std::cout << "input CMD:";
        std::cin.getline(recvBuff, 128);
        if(0 == strcmp(recvBuff,"quit"))
        {
            g_isRun = false;
        }
        else
        {
            std::cout << "cmd dont know" << std::endl;
        }
    }
}