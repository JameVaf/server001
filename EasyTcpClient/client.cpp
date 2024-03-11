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

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif // _WIN32

const int CMD_BUFF = 128;
const int RECV_BUFF = 128;

char _cmdBuff[CMD_BUFF] = {0};
char _recvBuff[RECV_BUFF] = {0};

enum class CMD
{
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT_RESULT,
	CMD_QUIT,
	CMD_ERROR // ????
};

const int RECV_BUFF_LEN = 128;
const int SEND_BUFF_LEN = 128;

// ???????????????
void processCmd(SOCKET sock);
//
typedef struct DataHeader
{
	CMD cmd_;
	int length_;

} DataHeader;

typedef struct LOGIN : public DataHeader
{
	char name_[30] = {0};
	char password_[30] = {0};
} LOGIN;

typedef struct LOGOUT : public DataHeader
{
	char name_[30] = {0};
} LOGOUT;

typedef struct LOGIN_RESULT : public DataHeader
{
	bool result_ = false;
} LOGIN_RESULT;

typedef struct LOGOUT_RESULT : public DataHeader
{
	bool result_ = false;
} LOGOUT_RESULT;

int main()
{
#ifdef _WIN32
	// 1.????windows socket???????
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	if (0 != WSAStartup(ver, &dat))
	{
		std::cerr << "WSAStartup() error" << std::endl;
	}
#endif // _WIN32

	// 2.????socket?????
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 3.????connet
	struct sockaddr_in _serverAddr;
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_addr.s_addr = inet_addr("192.168.1.12");
	_serverAddr.sin_port = htons(4567);

	if (SOCKET_ERROR == connect(_sock, (sockaddr *)&_serverAddr, sizeof(_serverAddr)))
	{
		std::cerr << "connect() error" << std::endl;
	}
	else
	{
		std::cout << "connect server success" << std::endl;
	}

	// 3.??????????,????????????
	std::thread t(processCmd, _sock);

	// 3.????select
	while (true)
	{
		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);

		timeval _time = {2, 0};
		int ret = select(_sock + 1, &fdRead, nullptr, nullptr, nullptr);
		if (ret < 0)
		{
			std::cout << "select error" << std::endl;
		}

		if (FD_ISSET(_sock, &fdRead)) // ??????????????
		{
			// ????????????????
			DataHeader data;
			memset(&data, 0, sizeof(data));
			int _recvLen = (int)recv(_sock, (char *)&data, sizeof(data), 0);
			if (_recvLen <= 0)
			{
				std::cerr << "recv from server Error" << std::endl;
			}

			switch (data.cmd_)
			{
			case CMD::CMD_LOGIN_RESULT:
			{
				LOGIN_RESULT temp;
				memset(&temp, 0, sizeof(temp));
				int _recvLen = (int)recv(_sock, (char *)&temp + sizeof(DataHeader), sizeof(LOGIN_RESULT) - sizeof(DataHeader), 0);
				if (_recvLen <= 0)
				{
					std::cerr << "recv from server Login result Error" << std::endl;
				}
				if (temp.result_)
				{
					std::cout << "??????" << std::endl;
				}
				else
				{
					std::cout << "??????" << std::endl;
				}
			}
			break;
			case CMD::CMD_LOGOUT_RESULT:
			{
				LOGOUT_RESULT temp;
				memset(&temp, 0, sizeof(temp));
				int _recvLen = (int)recv(_sock, (char *)&temp + sizeof(DataHeader), sizeof(LOGOUT_RESULT) - sizeof(DataHeader), 0);
				if (_recvLen <= 0)
				{
					std::cerr << "recv from server Logout result Error" << std::endl;
				}
				if (temp.result_)
				{
					std::cout << "??????" << std::endl;
				}
				else
				{
					std::cout << "??????" << std::endl;
				}
			}
			break;
			default:
			{
				std::cout << "from server Error" << std::endl;
			}
			break;
			}
		}
	}

	t.join();

#ifdef _WIN32
	closesocket(_sock);
#elif __linux__
	close(_sock);
#endif // _WIN32

#ifdef _WIN32
	// 6.???socket ????
	WSACleanup();
#endif // _WIN32

	getchar();
	return 0;
}

// ???????????????
void processCmd(SOCKET sock)
{
	while (true)
	{
		memset(_cmdBuff, 0, CMD_BUFF);
		memset(_recvBuff, 0, RECV_BUFF);
		std::cout << "??????cmd????:";
		std::cin >> _cmdBuff;
		if (0 == strcmp(_cmdBuff, "quit"))
		{
			DataHeader head;
			memset(&head, 0, sizeof(head));
			head.cmd_ = CMD::CMD_QUIT;
			head.length_ = sizeof(head);
			int _sendLen = (int)send(sock, (char *)&head, sizeof(head), 0);
			if (_sendLen <= 0)
			{
				std::cerr << "send quit to server Error" << std::endl;
			}

			break;
		}
		else if (0 == strcmp(_cmdBuff, "login"))
		{
			LOGIN temp;
			memset(&temp, 0, sizeof(temp));
			temp.cmd_ = CMD::CMD_LOGIN;
			temp.length_ = sizeof(temp);
			std::cout << "??????????:";
			std::cin >> temp.name_;
			std::cout << std::endl;
			std::cout << "??????????:";
			std::cin >> temp.password_;
			std::cout << std::endl;
			int _sendLen = (int)send(sock, (char *)&temp, sizeof(temp), 0);
			if (_sendLen <= 0)
			{
				std::cerr << "send Login server Error" << std::endl;
			}
		}
		else if (0 == strcmp(_cmdBuff, "logout"))
		{
			LOGOUT temp;
			memset(&temp, 0, sizeof(temp));
			temp.cmd_ = CMD::CMD_LOGOUT;
			temp.length_ = sizeof(temp);
			std::cout << "??????????:";
			std::cin >> temp.name_;
			std::cout << std::endl;

			int _sendLen = (int)send(sock, (char *)&temp, sizeof(temp), 0);
			if (_sendLen <= 0)
			{
				std::cerr << "send Login server Error" << std::endl;
			}
		}
		else
		{
			std::cout << "input Error! please input login or logout or quit " << std::endl;
			continue;
		}
	}
};
