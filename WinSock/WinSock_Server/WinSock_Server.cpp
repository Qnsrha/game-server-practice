#include <iostream>
#include <WinSock2.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#define SERVER_PORT 9000

using namespace std;

void initializeWinsock()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        throw runtime_error("WSAStartup failed: " + to_string(result));
    }
    cout << "Winsock initialized successfully!" << endl;
}

SOCKET createSocket()
{
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        throw runtime_error("Socket creation failed: " + to_string(WSAGetLastError()));
    }
    cout << "Socket created successfully!" << endl;
    return serverSocket;
}

void bindSocket(SOCKET serverSocket, SOCKADDR_IN& serverAddr)
{
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        throw runtime_error("Bind failed: " + to_string(WSAGetLastError()));
    }
    cout << "Socket binded successfully!" << endl;
}

void listenForClients(SOCKET serverSocket)
{
    if (listen(serverSocket, 5) == SOCKET_ERROR)
    {
        throw runtime_error("Listen failed: " + to_string(WSAGetLastError()));
    }
    cout << "Server is listening on port " << SERVER_PORT << " ..." << endl;
}

SOCKET acceptClient(SOCKET serverSocket, SOCKADDR_IN& clientAddr)
{
    int clientAddrSize = sizeof(clientAddr);
    SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET)
    {
        throw runtime_error("Accept failed: " + to_string(WSAGetLastError()));
    }
    cout << "Client connection success" << endl;
    return clientSocket;
}

void handleClient(SOCKET clientSocket)
{
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0)
    {
        buffer[bytesReceived] = '\0';
        cout << "Client says: " << buffer << endl;
    }
    else if (bytesReceived == 0)
    {
		throw runtime_error("Client disconnected!");
    }
    else
    {
		throw runtime_error("Receive failed: " + to_string(WSAGetLastError()));
    }

    const char* message = "Hello from server!";
    int bytesSent = send(clientSocket, message, strlen(message), 0);
    if (bytesSent == SOCKET_ERROR)
    {
        cerr << "Send failed: " << WSAGetLastError() << endl;
    }
    else
    {
        cout << "Message sent to client!" << endl;
    }

    closesocket(clientSocket);
}

int main()
{
    SOCKET clientSocket = INVALID_SOCKET;
    SOCKET serverSocket = INVALID_SOCKET;
    try
    {
        initializeWinsock();

        serverSocket = createSocket();

        SOCKADDR_IN serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(SERVER_PORT);

        bindSocket(serverSocket, serverAddr);
        listenForClients(serverSocket);

        SOCKADDR_IN clientAddr;
        clientSocket = acceptClient(serverSocket, clientAddr);
        handleClient(clientSocket);

        closesocket(serverSocket);
        WSACleanup();
    }
    catch (const exception& ex)
    {
        cerr << ex.what() << endl;
        if (clientSocket != INVALID_SOCKET)
        {
            closesocket(clientSocket);
        }
        if (serverSocket != INVALID_SOCKET)
        {
            closesocket(serverSocket);
        }

        WSACleanup();
        return -1;
    }

    return 0;
}