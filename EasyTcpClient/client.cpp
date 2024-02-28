
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Winsock2.h>
#include<Windows.h>
#include<iostream>
#pragma comment(lib,"ws2_32.lib")


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
		std::cout << "connetc success" << std::endl;
		char _buff[100] = { 0 };
		int _recvLen = recv(_sock, _buff, 100, 0);
		std::cout << "SERVER MESSAGE: " << _buff << std::endl;
	}

	closesocket(_sock);


	//6.关闭socket 环境
	WSACleanup();
	return 0;
}
