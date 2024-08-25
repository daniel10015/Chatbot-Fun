
#ifdef _WIN32
//#include <Windows.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#  include <WinSock2.h>
#  include <Ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
#else
// Do something else here for non windows
#endif
#include <iostream>
#include <stdio.h>
#include <threads.h>
#include <fstream>
/* socket libraries */

#include <string>
#include <mutex>

#define PORT     8080 
#define ADDRESS  "127.0.0.1"
#define MAXLINE  1024 

std::mutex newMessageMutex;
bool newMessage = false;
std::string username; // read-only

std::string unique_identifier;



void GetUserMessage(std::string& input)
{
    std::getline(std::cin, input);
}

void HandleUserInput(std::string& userInput)
{
    std::string temp;
    while (true)
    {
        GetUserMessage(temp);
        newMessageMutex.lock();
        userInput = temp;
        newMessage = true;
        newMessageMutex.unlock();
    }
}

void ListenForServer(SOCKET sock, sockaddr* from, int addrLength)
{
    char serverBuf[1025];
    int bufferLength = 1024;
    while (true)
    {
        int bytesRecieved = recvfrom(sock, serverBuf, bufferLength, 0, from, &addrLength);
        if (bytesRecieved == SOCKET_ERROR)
        {
            printf("recvfrom failed with error %d\n", WSAGetLastError());
        }
        serverBuf[bytesRecieved] = '\0';
        
        // write to chat logs for the user:
        std::ofstream outfile;
        outfile.open(unique_identifier + "_" + username + "_chat_logs.txt", std::ios_base::app); // append instead of overwrite
        outfile << serverBuf << "\n";
        outfile.close();
    }
}

int main(void)
{
    std::cout << "input username: ";
    GetUserMessage(username);



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
    std::string toSend = "[" + username + "] connected!";
    strcpy_s(SendBuf, toSend.c_str());

    struct sockaddr_in serverAddr;
    int clientAddrSize = sizeof(sockaddr_in);

    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(ADDRESS);
    serverAddr.sin_family = AF_INET;

    puts("Sending a datagram to the receiver...");
    int clientResult = sendto(sendSocket,
        SendBuf, BufLen, 0, (SOCKADDR*)&serverAddr, clientAddrSize);
    // get unique identifier
    clientResult = recvfrom(sendSocket, SendBuf, BufLen, 0, (SOCKADDR*)&serverAddr, &clientAddrSize);
    SendBuf[clientResult] = '\0';
    unique_identifier = SendBuf;

    std::string userMessage;
    std::thread userInputThread(HandleUserInput, std::ref(userMessage));
    userInputThread.detach();
    std::thread serverListenThread(ListenForServer, sendSocket, (SOCKADDR*)&serverAddr, clientAddrSize);
    serverListenThread.detach();

    std::string sendMessage;

    // while most significant bit isn't set, keep running
    // ESC to leave loop
    GetKeyState(VK_ESCAPE);
    while (~GetKeyState(VK_ESCAPE) & 0x8000)
    {
        // check for user input, if so then send
        newMessageMutex.lock();
        if (newMessage)
        {
            newMessage = false;
            //std::cout << "DEBUG: " << userMessage << std::endl;
            // could make this faster with pointers but lazy rn
            sendMessage = unique_identifier + "[" + username + "] " + userMessage;
            sendto(sendSocket,
                sendMessage.c_str(), sendMessage.size(), 0, (SOCKADDR*)&serverAddr, clientAddrSize);
            userMessage.clear();
        }
        newMessageMutex.unlock();

        Sleep(10); // sleep for 10 ms
    }
    sendMessage = unique_identifier + "[" + username + "] disconnected!";
    sendto(sendSocket,
        sendMessage.c_str(), sendMessage.size(), 0, (SOCKADDR*)&serverAddr, clientAddrSize);
    std::cout << "Session ended for " << username << std::endl;

    return 0;
}