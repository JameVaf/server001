
#pragma warning(disable : 4996)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Winsock2.h>
#include<Windows.h>
#include<iostream>
#pragma comment(lib,"ws2_32.lib")

const int RECV_BUFF_LEN = 128;
const int SEND_BUFF_LEN = 128;

enum class  CMD
{
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT_RESULT,
	CMD_QUIT,
	CMD_ERROR //待定
};





typedef struct DataHeader
{
	CMD cmd_;
	int length_;

}DataHeader;


typedef struct LOGIN:public DataHeader
{
	char name_[30] = { 0 };
	char password_[30] = { 0 };
}LOGIN;

typedef struct LOGOUT :public DataHeader
{
	char name_[30] = { 0 };
}LOGOUT;

typedef struct LOGIN_RESULT :public DataHeader
{
	bool result_ = false;
}LOGIN_RESULT;

typedef struct LOGOUT_RESULT :public DataHeader
{
	bool result_ = false;
}LOGOUT_RESULT;


 
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

	//3.bind 绑定用于接受客户端连接的网络端口
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

	//4.listen 监听端口

	if (SOCKET_ERROR == listen(_sock, 5))
	{
		std::cerr << "listen() error" << std::endl;
	}
	else
	{
		std::cout << "listen success" << std::endl;
	}

	//5.accept 接受客户端

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
	//接受客户端的数据
	char _recvBuff[RECV_BUFF_LEN] = { 0 };
	char _sendBuff[SEND_BUFF_LEN] = { 0 };

	bool is_run = true;
	while (is_run)
	{
		memset(_recvBuff, 0, RECV_BUFF_LEN);
		memset(_sendBuff, 0, SEND_BUFF_LEN);

		DataHeader _header;
		//读取客户端的数据
		int _recvLen = recv(_clientSock,(char*)&_header,sizeof(DataHeader), 0);
		if (_recvLen <= 0)
		{
			std::cout << "client error" << std::endl;
		}


		
		//开始读取信息
		switch(_header.cmd_)
		{
		case CMD::CMD_LOGIN :
		{
			LOGIN temp ;
			memset(&temp, 0, sizeof(LOGIN));
			//读取LOGIN的相关数据
			int _recvLen = recv(_clientSock, (char*)&temp + sizeof(DataHeader), sizeof(LOGIN)-sizeof(DataHeader), 0);
			if (_recvLen <= 0)
			{
				std::cout << "client error" << std::endl;
			}
			std::cout << "LOGIN  Name:" << temp.name_ << " PassWord:" << temp.password_ << std::endl;

			LOGIN_RESULT _result;
			memset(&_result, 0, sizeof(LOGIN_RESULT));
			_result.cmd_ = CMD::CMD_LOGIN_RESULT;
			_result.length_ = sizeof(LOGIN_RESULT);
			_result.result_ = true;

			int _sendLen = send(_clientSock, (char*)&_result, sizeof(LOGIN_RESULT), 0);
			if (_sendLen <= 0)
			{
				std::cerr << "send LOGIN_RESULT ERROR" << std::endl;
			}



		}
		break;
		case CMD::CMD_LOGOUT :	
		{
			LOGOUT temp;
			memset(&temp, 0, sizeof(temp));
			//读取LOGOUT的相关数据
			int _recvLen = recv(_clientSock, (char*)&temp + sizeof(DataHeader), sizeof(LOGOUT) - sizeof(DataHeader), 0);
			if (_recvLen <= 0)
			{
				std::cout << "client error" << std::endl;
			}
			std::cout << "LOGOUT  Name:" << temp.name_ << std::endl;

			LOGOUT_RESULT _result;
			memset(&_result, 0, sizeof(LOGOUT_RESULT));
			_result.cmd_ = CMD::CMD_LOGOUT_RESULT;
			_result.length_ = sizeof(LOGOUT_RESULT);
			_result.result_ = true;

			int _sendLen = send(_clientSock, (char*)&_result, sizeof(LOGOUT_RESULT), 0);
			if (_sendLen <= 0)
			{
				std::cerr << "send LOGOUT_RESULT ERROR" << std::endl;
			}
		}
			break;
		case CMD::CMD_QUIT:
		{
			std::cout << "server quit" << std::endl;
			is_run = false;
		}
			break;
		default:
		{
			std::cout << "I don't know CMD" << std::endl;
			DataHeader _result;
			memset(&_result, 0, sizeof(_result));
			_result.cmd_ = CMD::CMD_ERROR;
			_result.length_ = sizeof(DataHeader);
			int _sendLen = send(_clientSock, (char*)&_result, sizeof(DataHeader), 0);
			if (_sendLen <= 0)
			{
				std::cerr << "send LOGIN_ERROR ERROR" << std::endl;
			}
			
		}
			break;
		}
		
	
		  

	}
	//关闭套接字 
	closesocket(_clientSock);
	closesocket(_sock);


	//6.关闭socket 环境
	WSACleanup();
	return 0;
}