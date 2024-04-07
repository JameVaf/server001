#include <iostream>
#include "../include/EasyTcpServer.hpp"

int main()
{
    
    EasyTcpServer server("127.0.0.1", 4567);
    
    server.Start();
    server.Accept();
    return 0;
}   