#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h> // Add this header for inet_pton
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

#define PORT 9000
#define BUFFER_SIZE 1024

struct OverlappedEx
{
    OVERLAPPED overlapped;
    WSABUF wsaBuf;
    char buffer[BUFFER_SIZE];
    SOCKET socket;
    WSAEVENT hEvent;
};

void ErrorHandling(const char* message)
{
    std::cerr << message << " Error Code: " << WSAGetLastError() << std::endl;
    exit(1);
}

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        ErrorHandling("WSAStartup() failed");
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        ErrorHandling("socket() failed");
    }

    SOCKADDR_IN serverAddr{};
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    serverAddr.sin_port = htons(PORT);

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        ErrorHandling("connect() failed");
    }

    std::cout << "Connected to server!" << std::endl;

    OverlappedEx sendOverlapped{};
    sendOverlapped.socket = clientSocket;
    sendOverlapped.wsaBuf.buf = sendOverlapped.buffer;
    sendOverlapped.wsaBuf.len = BUFFER_SIZE;
    sendOverlapped.hEvent = WSACreateEvent();
    sendOverlapped.overlapped.hEvent = sendOverlapped.hEvent;

    strcpy_s(sendOverlapped.buffer, "Hello, Server!");

    DWORD bytesSent;
    if (WSASend(clientSocket, &sendOverlapped.wsaBuf, 1, &bytesSent, 0, (LPWSAOVERLAPPED)&sendOverlapped, NULL) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            ErrorHandling("WSASend() failed");
        }
    }

    WaitForSingleObject(sendOverlapped.hEvent, INFINITE);
    std::cout << "Sent to server: " << sendOverlapped.buffer << std::endl;

    OverlappedEx recvOverlapped{};
    recvOverlapped.socket = clientSocket;
    recvOverlapped.wsaBuf.buf = recvOverlapped.buffer;
    recvOverlapped.wsaBuf.len = BUFFER_SIZE;
    recvOverlapped.hEvent = WSACreateEvent();
    recvOverlapped.overlapped.hEvent = recvOverlapped.hEvent;

    DWORD bytesReceived;
    DWORD flags = 0;
    if (WSARecv(clientSocket, &recvOverlapped.wsaBuf, 1, &bytesReceived, &flags, (LPWSAOVERLAPPED)&recvOverlapped, NULL) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            ErrorHandling("WSARecv() failed");
        }
    }

    WaitForSingleObject(recvOverlapped.hEvent, INFINITE);
    std::cout << "Server Response: " << recvOverlapped.buffer << std::endl;

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
