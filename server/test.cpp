#define WIN32_LEAN_AND_MEAN
#include<iostream>
#include<WinSock2.h>
#include<Windows.h>

#pragma comment(lib,"ws2_32.lib")
int main()
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver,&dat);
	std::cout << "Test" << std::endl;

	WSACleanup();

	return 0;
}