#include<iostream>
#include<thread>
#include<chrono>

#include   "../include/Header.hpp"
#include "../include/EasyTcpClient.hpp"

void threadSendCmd(EasyTcpClient *client);

int main()
{
    
    EasyTcpClient client(std::string("192.168.1.12"), std::string("4567"));
    client.Start(); //初始化资源
    client.Connect();
    std::thread t(threadSendCmd, &client);
    client.Select();

    // std::this_thread::sleep_for(std::chrono::seconds(5));

    
    t.join();
    return 0;
}

void threadSendCmd(EasyTcpClient *client)
{
    std::cout << "Thread()" << std::endl;
    char cmdBuff[128] = {0};//128个字节足够缓冲命令
    char tempBuff[client->RECV_BUFF] = {0};
    while(client->getIsRun())
    {
        int sendLen = 0;
        memset(cmdBuff, 0, 128);
        memset(tempBuff, 0, client->RECV_BUFF);
        std::cout <<  "请输入cmd命令:";
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
        client->Send(tempBuff,sendLen);
    }
}