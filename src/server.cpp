#include <iostream>
#include "../include/EasyTcpServer.hpp"

int main()
{
    EasyTcpServer server("192.168.1.8",4567);
    server.Start();
    server.Accept();
    return 0;
}