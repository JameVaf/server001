#include <winsock2.h>
#include <stdio.h>
#include <iostream>
#define _WINSOCK_DEPRECATED_NOWARNINGS

#pragma comment(lib, "ws2_32.lib")

using namespace std;
int main()
{
	char sendBuf[1024];
	char receiveBuf[1024];
	while (1)
	{
		// 创建套接字，socket前的一些检查工作.
		// 服务的启动
		WSADATA wsadata; // wsa 即windows socket async 异步套接字
		if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
		{
			cout << "套接字未打开" << endl;
			return 0;
		}
		else
		{
			cout << "已打开套接字" << endl;
		}
		SOCKET serSocket = socket(AF_INET, SOCK_STREAM, 0); // 创建可识别的套接字//parm1: af 地址协议族 ipv4 ipv6
															// parm2:type 传输协议类型 流式套接字（SOCK_STREAM)，数据包套接字（SOCK_DGRAM)
															// parm3:ptotoc1 使用具体的某个传输协议

		SOCKADDR_IN addr;							   // 需要绑定的参数，主要是本地的socket的一些信息。
		addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); // ip地址，htonl即host本机 to:to  n:net l:unsigned long 大端存储，低字节在高位
		addr.sin_family = AF_INET;
		addr.sin_port = htons(6000); // 端口 htons将无符号短整型转化为网络字节序

		bind(serSocket, (SOCKADDR *)&addr, sizeof(SOCKADDR)); // 绑定完成
		listen(serSocket, 5);								  // 监听窗口
		SOCKADDR_IN clientsocket;
		int len = sizeof(SOCKADDR);
		SOCKET serConn = accept(serSocket, (SOCKADDR *)&clientsocket, &len); // 于客户端建立链接

		cout << "发出:";
		gets_s(sendBuf, 1024);
		send(serConn, sendBuf, 1024, 0);

		recv(serConn, receiveBuf, 1024, 0);
		cout << "收到：" << receiveBuf << endl;

		closesocket(serConn); // 关闭
		WSACleanup();		  // 释放资源
	}
	return 0;
}
