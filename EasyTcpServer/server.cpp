
#pragma warning(disable : 4996)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Winsock2.h>
#include<Windows.h>
#include<iostream>
#include<vector>
#pragma comment(lib,"ws2_32.lib")

const int RECV_BUFF_LEN = 128;
const int SEND_BUFF_LEN = 128;

std::vector<SOCKET> g_cVector;

char _recvBuff[RECV_BUFF_LEN] = { 0 };
char _sendBuff[SEND_BUFF_LEN] = { 0 };

bool is_run = true;//主循环是否循环

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

bool process(SOCKET sock);
 
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


	//接受客户端的数据



	while (is_run)
	{
		memset(_recvBuff, 0, RECV_BUFF_LEN);
		memset(_sendBuff, 0, SEND_BUFF_LEN);

		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;

		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);

		//将已连接客户端也加入fd集合
		for (auto iter : g_cVector)
		{
			FD_SET(iter, &fdRead);
		}

		struct timeval _time;
		_time.tv_sec = 2;
		_time.tv_usec = 0;
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp,nullptr);
		if (ret < 0)
		{
			std::cout << "select 任务结束" << std::endl;
			break;
		}

		//判断是否有新的连接
		if (FD_ISSET(_sock, &fdRead))
		{
			//5.accept 接受客户端

			//将server  sock清除于fdRead
			FD_CLR(_sock, &fdRead);

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

				std::cout << "client "<<_clientSock<<" ip is: " << inet_ntoa(_clientAddr.sin_addr)<<"port :" <<ntohs(_clientAddr.sin_port)<< std::endl;

			}
			g_cVector.push_back(_clientSock);
		}
		else if(fdRead.fd_count > 0)   //处理其他客户端的数据
		{
			for (int i = 0; i < fdRead.fd_count; ++i)
			{
				process(fdRead.fd_array[i]);
			}
		}

		





		
		
		
	
		  

	}
	//关闭已连接的套接字 
	for (auto iter : g_cVector)
	{
		closesocket(iter);
	}

	closesocket(_sock);


	//6.关闭socket 环境
	WSACleanup();
	return 0;
}

//处理客户端的数据
bool process(SOCKET sock)
{
	int _recvLen = recv(sock, _recvBuff, sizeof(DataHeader), 0);
	if (_recvLen < 0)
	{
		std::cout << "套接字" << sock << " 断开连接..." << std::endl;

		//将套接字从已连接的集合中删除
		auto iter = std::find(g_cVector.begin(), g_cVector.end(), sock);
		if (iter == g_cVector.end())
		{
			std::cerr << "该套接字不在已连接的套截字集合中,程序存在逻辑漏洞" << std::endl;
			return false;
		}
		g_cVector.erase(iter);

	}
	//开始读取信息
	switch (((DataHeader*)_recvBuff)->cmd_)
	{
	case CMD::CMD_LOGIN:
	{
	
		//读取LOGIN的相关数据
		int _recvLen = recv(sock, _recvBuff + sizeof(DataHeader), sizeof(LOGIN) - sizeof(DataHeader), 0);
		if (_recvLen <= 0)
		{
			std::cout << "client error" << std::endl;
		}
		std::cout << "LOGIN  Name:" << ((LOGIN*)_recvBuff)->name_ << " PassWord:" << ((LOGIN*)_recvBuff)->name_ << std::endl;
		std::cout << "recv data Len" << ((LOGIN*)_recvBuff)->length_ << std::endl;

	
	
		((LOGIN_RESULT*)_sendBuff)->cmd_ = CMD::CMD_LOGIN_RESULT;
		((LOGIN_RESULT*)_sendBuff)->length_= sizeof(LOGIN_RESULT);
		((LOGIN_RESULT*)_sendBuff)->result_ = true;

		int _sendLen = send(sock, _sendBuff, sizeof(LOGIN_RESULT), 0);
		if (_sendLen <= 0)
		{
			std::cerr << "send LOGIN_RESULT ERROR" << std::endl;
		}
		else {
			std::cout << "sucess send " << _sendLen << " bytes " << std::endl;
		}



	}
	break;
	case CMD::CMD_LOGOUT:
	{
		
		//读取LOGOUT的相关数据
		int _recvLen = recv(sock, _recvBuff + sizeof(DataHeader), sizeof(LOGOUT) - sizeof(DataHeader), 0);
		if (_recvLen <= 0)
		{
			std::cout << "client error" << std::endl;
		}
		std::cout << "LOGOUT  Name:" << ((LOGOUT*)_recvBuff)->name_ << std::endl;
		std::cout << "recv data len " << ((LOGOUT*)_recvBuff)->length_ << std::endl;
		LOGOUT_RESULT _result;
		
		((LOGOUT_RESULT*)_sendBuff)->cmd_ = CMD::CMD_LOGOUT_RESULT;
		((LOGOUT_RESULT*)_sendBuff)->length_ = sizeof(LOGOUT_RESULT);
		((LOGOUT_RESULT*)_sendBuff)->result_ = true;

		int _sendLen = send(sock, _sendBuff, sizeof(LOGOUT_RESULT), 0);
		if (_sendLen <= 0)
		{
			std::cerr << "send LOGOUT_RESULT ERROR" << std::endl;
		}
		else {
			std::cout << "sucess send " << _sendLen << " bytes " << std::endl;
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

		((DataHeader*)_sendBuff)->cmd_ = CMD::CMD_ERROR;
		((DataHeader*)_sendBuff)->length_ = sizeof(DataHeader);
		int _sendLen = send(sock, _sendBuff, sizeof(DataHeader), 0);
		if (_sendLen <= 0)
		{
			std::cerr << "send LOGIN_ERROR ERROR" << std::endl;
		}
		else {
			std::cout << "sucess send " << _sendLen << " bytes " << std::endl;
		}

		return false;	
	}
	break;
	}

	return true;
}