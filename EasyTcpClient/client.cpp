
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Winsock2.h>
#include<Windows.h>
#include<iostream>
#pragma comment(lib,"ws2_32.lib")

const int CMD_BUFF = 128;
const int RECV_BUFF = 128;

typedef struct dataPack
{
	int age;
	char name[32];      
}dataPack;


int main()
{
	//1.启动windows socket的编程环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	if (0 != WSAStartup(ver, &dat))
	{
		std::cerr << "WSAStartup() error" << std::endl;
	}

	//2.创建socket套接字
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//3.进行connet
	struct sockaddr_in _serverAddr;
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	_serverAddr.sin_port = htons(4567);

	if (SOCKET_ERROR == connect(_sock, (sockaddr*)&_serverAddr, sizeof(_serverAddr)))
	{
		std::cerr << "connect() error" << std::endl;
		
	}
	else
	{

		std::cout << "connect server success" << std::endl;
		char _cmdBuff[CMD_BUFF] = { 0 };
		char _recvBuff[RECV_BUFF] = { 0 };
		while (true)
		{
			memset(_cmdBuff, 0, CMD_BUFF);
			memset(_recvBuff, 0, RECV_BUFF);
			std::cout << "请输入cmd命令:";
			std::cin >> _cmdBuff;
			if (0 == strcmp(_cmdBuff, "quit"))
			{
				std::cout << "client close" << std::endl;
				
				int _sendLen = send(_sock, _cmdBuff, CMD_BUFF, 0);
				if (_sendLen <= 0)
				{
					std::cerr << "send() error" << std::endl;
				}
				break;
			}
			int _sendLen = send(_sock, _cmdBuff, CMD_BUFF, 0);
			if (_sendLen <= 0)
			{
				std::cerr << "send() error" << std::endl;
			}
			//开始接受服务端的数据
			int _recvLen = recv(_sock, _recvBuff, RECV_BUFF, 0);
			if (_recvLen <= 0)
			{
				std::cerr << "recv() error" << std::endl;
			}
			std::cout << "SERVER MESSAGE: name " << ((dataPack*)_recvBuff)->name <<" age: "<< ((dataPack*)_recvBuff)->age << std::endl;



		}
	}
	


	closesocket(_sock);


	//6.关闭socket 环境
	WSACleanup();
	getchar();
	return 0;
}
