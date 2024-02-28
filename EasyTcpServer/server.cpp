
#pragma warning(disable : 4996)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Winsock2.h>
#include<Windows.h>
#include<iostream>
#pragma comment(lib,"ws2_32.lib")

const int RECV_BUFF_LEN = 128;
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
	memset(&_clientAddr, 0, sizeof(struct sockaddr_in));
	SOCKET _clientSock = INVALID_SOCKET;
	int _clientLen = sizeof(_clientAddr);
	_clientSock = accept(_sock, (sockaddr*)&_clientAddr, &_clientLen);
	if (INVALID_SOCKET == _clientSock)
	{
		std::cerr << "accept() Error,Error Code: " << WSAGetLastError() << std::endl;
	}
	else
	{

		std::cout << "client ip is: " << inet_ntoa(_clientAddr.sin_addr) << std::endl;
		 
	}
	
	char _recvBuff[RECV_BUFF_LEN] = { 0 };
	while (true)
	{
		int _recvLen = recv(_clientSock, _recvBuff, RECV_BUFF_LEN, 0);
		if (_recvLen <= 0)
		{
			std::cout << "client error" << std::endl;
		}

		char _sendBuff[128] = {0};
		//��ʼ��ȡ��Ϣ
		if (0 == strcmp(_recvBuff, "getName"))
		{
			char tempstr[] = "xiangqiang";
			strncpy(_sendBuff, tempstr, strlen(tempstr) + 1);
		}
		else if (0 == strcmp(_recvBuff, "getAge"))
		{
			char tempstr[] = "18";
			strncpy(_sendBuff, tempstr, strlen(tempstr) + 1);
		}
		else if (0 == strcmp(_recvBuff, "quit"))
		{
			std::cout << "quit server" << std::endl;
			break;
		}
		else
		{
			char tempstr[] = "don't know cmd,\nplease enter in agein";
			strncpy(_sendBuff, tempstr, strlen(tempstr) + 1);
		}
		int _sendLen = send(_clientSock, _sendBuff, strlen(_sendBuff) + 1, 0);
		if (_sendLen <= 0)
		{
			std::cerr << "send() error" << std::endl;
		}
		  

	}
	//�ر��׽��� 
	closesocket(_clientSock);
	closesocket(_sock);


	//6.�ر�socket ����
	WSACleanup();
	return 0;
}