
#pragma warning(disable : 4996)

#ifdef  _WIN32

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Winsock2.h>
#include<Windows.h>

#elif  __linux__

#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(-0)
#define SOCKET_ERROR		   (-1)

#endif //  _WIN32





#include<iostream>
#include<vector>
#include<algorithm>

#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#endif // _WIN32



const int RECV_BUFF_LEN = 128;
const int SEND_BUFF_LEN = 128;

std::vector<SOCKET> g_cVector;

char _recvBuff[RECV_BUFF_LEN] = { 0 };
char _sendBuff[SEND_BUFF_LEN] = { 0 };

bool is_run = true;//��ѭ���Ƿ�ѭ��

enum class  CMD
{
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT_RESULT,
	CMD_QUIT,
	CMD_ERROR //����
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
	//1.����windows socket�ı�̻���

#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	if (0 != WSAStartup(ver, &dat))
	{
		std::cerr << "WSAStartup() error" << std::endl;
	}
#endif
	
	//2.����socket�׽���
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//3.bind �����ڽ��ܿͻ������ӵ�����˿�
	struct sockaddr_in _serverAddr = {};
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_port = htons(4567);
	_serverAddr.sin_addr.s_addr = inet_addr("192.168.1.12 ");
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


	//���ܿͻ��˵�����

	SOCKET _maxSock = _sock;

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

		//�������ӿͻ���Ҳ����fd����
		for (auto iter : g_cVector)
		{
			FD_SET(iter, &fdRead);
		}

		struct timeval _time;
		_time.tv_sec = 2;
		_time.tv_usec = 0;
		int ret = select(_maxSock + 1, &fdRead, &fdWrite, &fdExp,nullptr);
		if (ret < 0)
		{
			std::cout << "select �������" << std::endl;
			break;
		}

		//�ж��Ƿ����µ�����
		if (FD_ISSET(_sock, &fdRead))
		{
			//5.accept ���ܿͻ���

			//��server  sock�����fdRead
			FD_CLR(_sock, &fdRead);

			struct sockaddr_in _clientAddr;
			memset(&_clientAddr, 0, sizeof(struct sockaddr_in));
			SOCKET _clientSock = INVALID_SOCKET;
			socklen_t _clientLen = sizeof(_clientAddr);
			_clientSock = accept(_sock, (sockaddr*)&_clientAddr, &_clientLen);
			if (INVALID_SOCKET == _clientSock)
			{
				std::cerr << "accept() Error" << std::endl;
			}
			else
			{

				std::cout << "client "<<_clientSock<<" ip is: " << inet_ntoa(_clientAddr.sin_addr)<<"port :" <<ntohs(_clientAddr.sin_port)<< std::endl;

			}
			g_cVector.push_back(_clientSock);
			++_maxSock;
		}
#ifdef _WIN32
		else if(fdRead.fd_count > 0)   //���������ͻ��˵�����
		{
			for (int i = 0; i < fdRead.fd_count; ++i)
			{
				process(fdRead.fd_array[i]);
			}
		}
		#elif __linux__
		else{
			for (int i = 0; i < _maxSock;++i)
			{
				if(i!= _sock && FD_ISSET(i,&fdRead))
				{
					process(i);
				}
			}
		}

		#endif
		

	}

#ifdef  _WIN32
	//�ر������ӵ��׽��� 
	for (auto iter : g_cVector)
	{
		closesocket(iter);
	}

	closesocket(_sock);


	//6.�ر�socket ����
	WSACleanup();
#else

	for (auto iter : g_cVector)
	{
		close(iter);
	}
	close(_sock);
#endif //  _WIN32

	return 0;
};

//�����ͻ��˵�����
bool process(SOCKET sock)
{
	int _recvLen = (int)recv(sock, _recvBuff, sizeof(DataHeader), 0);//unsafe
	if (_recvLen < 0)
	{
		std::cout << "�׽���" << sock << " �Ͽ�����..." << std::endl;

		//���׽��ִ������ӵļ�����ɾ��
		auto iter = std::find(g_cVector.begin(), g_cVector.end(), sock);
		if (iter == g_cVector.end())
		{
			std::cerr << "���׽��ֲ��������ӵ��׽��ּ�����,��������߼�©��" << std::endl;
			return false;
		}
		g_cVector.erase(iter);

	}
	//��ʼ��ȡ��Ϣ
	switch (((DataHeader*)_recvBuff)->cmd_)
	{
	case CMD::CMD_LOGIN:
	{
	
		//��ȡLOGIN���������
		int _recvLen = (int)recv(sock, _recvBuff + sizeof(DataHeader), sizeof(LOGIN) - sizeof(DataHeader), 0);//unsafe
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
		
		//��ȡLOGOUT���������
		int _recvLen = (int)recv(sock, _recvBuff + sizeof(DataHeader), sizeof(LOGOUT) - sizeof(DataHeader), 0); //unsafe
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

		int _sendLen = (int)send(sock, _sendBuff, sizeof(LOGOUT_RESULT), 0);//unsafe
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
		int _sendLen = (int)send(sock, _sendBuff, sizeof(DataHeader), 0);
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