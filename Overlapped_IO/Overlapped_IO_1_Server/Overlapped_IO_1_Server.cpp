#include <winsock2.h>
#include <windows.h>
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

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
    {
        ErrorHandling("socket() failed");
    }

    SOCKADDR_IN serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        ErrorHandling("bind() failed");
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        ErrorHandling("listen() failed");
    }

    std::cout << "Server is listening on port " << PORT << std::endl;

    SOCKET clientSocket;
    SOCKADDR_IN clientAddr{};
    int addrSize = sizeof(clientAddr);

    clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrSize);
    if (clientSocket == INVALID_SOCKET)
    {
        ErrorHandling("accept() failed");
    }

    std::cout << "Client connected!" << std::endl;

    OverlappedEx overlappedEx{};
    overlappedEx.socket = clientSocket;
    overlappedEx.wsaBuf.buf = overlappedEx.buffer;
    overlappedEx.wsaBuf.len = BUFFER_SIZE;
    overlappedEx.hEvent = WSACreateEvent();  // 이벤트 객체 생성
    overlappedEx.overlapped.hEvent = overlappedEx.hEvent;

    DWORD bytesReceived = 0;
    DWORD flags = 0;

    if (WSARecv(clientSocket, &overlappedEx.wsaBuf, 1, &bytesReceived, &flags, (LPWSAOVERLAPPED)&overlappedEx, NULL) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            ErrorHandling("WSARecv() failed");
        }
    }

    // 이벤트 대기
    if (WaitForSingleObject(overlappedEx.hEvent, INFINITE) == WAIT_OBJECT_0)
    {
        if (!WSAGetOverlappedResult(clientSocket, (LPWSAOVERLAPPED)&overlappedEx, &bytesReceived, FALSE, &flags))
        {
            ErrorHandling("WSAGetOverlappedResult() failed");
        }

        std::cout << "Received: " << overlappedEx.buffer << std::endl;

        // 클라이언트에게 데이터 전송
        DWORD bytesSent;
        if (WSASend(clientSocket, &overlappedEx.wsaBuf, 1, &bytesSent, 0, (LPWSAOVERLAPPED)&overlappedEx, NULL) == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                ErrorHandling("WSASend() failed");
            }
        }

        WaitForSingleObject(overlappedEx.hEvent, INFINITE);
        std::cout << "Echoed back: " << overlappedEx.buffer << std::endl;
    }

    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}
