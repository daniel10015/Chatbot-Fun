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
#include <set>
#include <unordered_map>
#include <string>
/* socket libraries */

#define PORT     8080 
#define ADDRESS  "127.0.0.1"
#define MAXLINE  1024 

const std::string connected = "connected!";
const std::string disconnected = "disconnected!";

unsigned long currentIdentifier = 0;

boolean IsNewClient(const std::string& input)
{
    if (input.size() > connected.size())
    {
        //std::cout << "DEBUG: " << input.substr(input.size() - 1 - connected.size()) << std::endl;
        if (input.substr(input.size() - connected.size(), connected.size()) == connected)
            return true;
    }
    return false;
}

void GetName(std::string& extracted, const std::string& input)
{
    size_t i = 1;
    while (input[i] != ']') { i++; }
    extracted = input.substr(1, i-1);
}

boolean IsDisconnectedClient(const std::string& input)
{
    if (input.size() > disconnected.size())
    {
        //std::cout << "DEBUG: " << input.substr(input.size() - disconnected.size()) << std::endl;
        if (input.substr(input.size() - disconnected.size(), disconnected.size()) == disconnected)
        {
            return true;
        }
    }
    return false;
}

// pair{ unique_ID, idx to start of msg }
std::pair<unsigned long, unsigned long> GetStartOfMessage(const std::string& input)
{
    unsigned long i = 1;
    while (input[i] != '[') { i++; }
    return { std::stoul(input.substr(0, i)), i };
}

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
    SOCKET serverSocket;
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET)
    {
        printf("socket failed with error %d\n", WSAGetLastError());
        return 1;
    }

    // create socket information
    struct sockaddr_in serverAddr;
    // Bind the socket to any address and the specified port.
    serverAddr.sin_family = AF_INET;
    // ensure that integers are read in the proper endian format used on the network
    // always big endian for winsock
    serverAddr.sin_port = htons(PORT); 
    // converts a IPv4 literal string into an unsigned 32 bit integer
    serverAddr.sin_addr.s_addr = inet_addr(ADDRESS);
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
    {
        printf("bind failed with error %d\n", WSAGetLastError());
        return 1;
    }

    // server specifications
    int bytesRecieved;
    char serverBuf[1025];
    int bufferLength = 1024;
    
    // separately store sender information
    struct sockaddr_in senderAddr;
    int sendAddrSize = sizeof(sockaddr_in);

    // recieve data
    printf("Receiving datagrams on %s\n", ADDRESS);
    std::unordered_map<unsigned long, sockaddr_in> clients;
    std::string serverInput;
    std::string clientName;
    size_t clientID;
    // blocking function
    while (true)
    {
        bytesRecieved = recvfrom(serverSocket, serverBuf, bufferLength, 0, (SOCKADDR*)&senderAddr, &sendAddrSize);
        if (bytesRecieved == SOCKET_ERROR)
        {
            printf("recvfrom failed with error %d\n", WSAGetLastError());
        }
        else
        {
            serverBuf[bytesRecieved] = '\0';
            serverInput = serverBuf;
            if (IsNewClient(serverInput))
            {
                // generate unique identifier
                clientID = currentIdentifier++;
                // send unqiue identifier to client
                std::string ID = std::to_string(clientID);
                sendto(serverSocket, ID.c_str(), ID.size(), 0, (SOCKADDR*)&senderAddr, sendAddrSize);
                // store client info
                clients[clientID] = senderAddr;
            }
            else if (IsDisconnectedClient(serverInput))
            {
                std::pair<unsigned long, unsigned long> ret = GetStartOfMessage(serverInput);
                clients.erase(ret.first);
                serverInput = serverInput.substr(ret.second, serverInput.size() - ret.second); // update msg
            }
        }

        //print out entire input message
        std::cout << serverBuf << std::endl;

        // relay message to all clients (including the one that sent it, unless they disconnected)
        for (const auto& client : clients)
        {
               sendto(serverSocket, serverBuf, bytesRecieved, 0, (SOCKADDR*)&(client.second), sendAddrSize);
        }
    }

    char sendBuf[] = { 'h', 'e', 'l', 'l', 'o', '\0' };
    int sendBufLen = (int)(sizeof(sendBuf) - 1);
    int sendResult = sendto(serverSocket,
        sendBuf, sendBufLen, 0, (SOCKADDR*)&senderAddr, sendAddrSize);
    if (sendResult == SOCKET_ERROR) 
    {
        printf("Sending back response got an error: %d\n", WSAGetLastError());
    }

    std::cout << "data recieved: " << serverBuf << std::endl;

    return 0;
}