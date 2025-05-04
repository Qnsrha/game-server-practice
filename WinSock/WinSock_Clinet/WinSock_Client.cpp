#include <WinSock2.h>
#include <WS2tcpip.h>  // inet_pton()
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 9000
#define SERVER_IP "127.0.0.1"

using namespace std;

void InitializeWinsock()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        throw runtime_error("WSAStartup failed: " + to_string(WSAGetLastError()));
    }
}

SOCKET CreateSocket()
{
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        throw runtime_error("Socket creation failed: " + to_string(WSAGetLastError()));
    }
    return clientSocket;
}

void ConnectToServer(SOCKET clientSocket, const char* serverIp, int serverPort)
{
    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) <= 0)
    {
        throw runtime_error("Invalid address / Address not supported");
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
    {
        throw runtime_error("Connection failed: " + to_string(WSAGetLastError()));
    }
}

void SendMessage(SOCKET clientSocket, const char* message)
{
    int bytesSent = send(clientSocket, message, strlen(message), 0);
    if (bytesSent == SOCKET_ERROR)
    {
        throw runtime_error("Send failed: " + to_string(WSAGetLastError()));
    }
}

void ReceiveMessage(SOCKET clientSocket)
{
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR)
    {
        throw runtime_error("Receive failed: " + to_string(WSAGetLastError()));
    }
    else if (bytesReceived > 0)
    {
        buffer[bytesReceived] = '\0';
        cout << "Server says: " << buffer << endl;
    }
}

int main()
{
    SOCKET clientSocket = INVALID_SOCKET;
    try
    {
        InitializeWinsock();
        clientSocket = CreateSocket();
        ConnectToServer(clientSocket, SERVER_IP, SERVER_PORT);
        cout << "Connected to server!" << endl;


        const char* message = "Hello from client!";
        SendMessage(clientSocket, message);


        ReceiveMessage(clientSocket);

        
        closesocket(clientSocket);
        WSACleanup();
    }
    catch (const exception& ex)
    {
        cerr << ex.what() << endl;
        if (clientSocket != INVALID_SOCKET)
        {
            closesocket(clientSocket);
        }
        WSACleanup();
        return -1;
    }

    return 0;
}
