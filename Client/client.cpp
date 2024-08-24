#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#  include <WinSock2.h>
#  include <Ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
#else
// Do something else here for non windows
#endif
#include <iostream>
#include <stdio.h>
/* socket libraries */

#define PORT     8080 
#define ADDRESS  "127.0.0.1"
#define MAXLINE  1024 

int main(void)
{
    // initialize winsock
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != NO_ERROR) {
        printf("WSAStartup failed with error %d\n", res);
        return 1;
    }

    // create server socket
    SOCKET sendSocket;
    sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sendSocket == INVALID_SOCKET)
    {
        printf("socket failed with error %d\n", WSAGetLastError());
        return 1;
    }

    // setup send info
    char SendBuf[1025];
    int BufLen = (int)(sizeof(SendBuf) - 1);
    const char* toSend = "foobar";
    strcpy_s(SendBuf, toSend);

    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(sockaddr_in);

    clientAddr.sin_port = htons(PORT);
    clientAddr.sin_addr.s_addr = inet_addr(ADDRESS);
    clientAddr.sin_family = AF_INET;

    puts("Sending a datagram to the receiver...");
    int clientResult = sendto(sendSocket,
        SendBuf, BufLen, 0, (SOCKADDR*)&clientAddr, clientAddrSize);


    return 0;
}