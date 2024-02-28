
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Winsock2.h>
#include<Windows.h>
#include<iostream>
#pragma comment(lib,"ws2_32.lib")


int main()
{
	//1.����windows socket�ı�̻���
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	if (0 != WSAStartup(ver, &dat))
	{
		std::cerr << "WSAStartup() error" << std::endl;
	}
	
	//2.����socket�׽���
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//3.bind �����ڽ��ܿͻ������ӵ�����˿�
	struct sockaddr_in _serverAddr = {};
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_port = htons(4567);
	_serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); 
	size_t _len = sizeof(_serverAddr);
	if (bind(_sock, (sockaddr*)&_serverAddr, _len) == SOCKET_ERROR)
	{
		std::cerr << "bind() Error" << std::endl;
	}
	else
	{
		std::cout << "bind success" << std::endl;
	}

	//4.listen �����˿�

	if (SOCKET_ERROR == listen(_sock, 5))
	{
		std::cerr << "listen() error" << std::endl;
	}
	else
	{
		std::cout << "listen success" << std::endl;
	}

	//5.accept ���ܿͻ���

	struct sockaddr_in _clientAddr;
	
	char message[] = "I'am server";
	while (true)
	{
		memset(&_clientAddr, 0, sizeof(struct sockaddr_in));
		  
		SOCKET _clientSock = INVALID_SOCKET;
		int _clientLen = sizeof(_clientAddr);
		_clientSock = accept(_sock, (sockaddr*)&_clientAddr, &_clientLen);
		if (INVALID_SOCKET == _clientSock)
		{
			std::cerr << "accept() Error,Error Code: "<<WSAGetLastError() << std::endl;
		}
		else
		{
			int _sendSize = send(_clientSock, message, strlen(message) + 1, 0);
			std::cout << "client ip is: " << inet_ntoa(_clientAddr.sin_addr) << std::endl;
			closesocket(_clientSock);
		}
	}
	//�ر��׽��� 
	closesocket(_sock);


	//6.�ر�socket ����
	WSACleanup();
	return 0;
}