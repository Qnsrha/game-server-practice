#include <winsock2.h>
#include <windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

#define PORT 9000
#define BUFFER_SIZE 1024

struct OverlappedEx
{
    WSAOVERLAPPED overlapped;
    WSABUF wsaBuf;
    SOCKET socket;
    char buffer[BUFFER_SIZE];
    
};

// 이벤트 핸들러 함수
void HandleEvent(const std::string& data)
{
    std::cout << "Echoed back: " << data << std::endl;
    // 여기서 추가적인 이벤트 기반 처리를 수행할 수 있습니다.
}

// 콜백 함수 (Completion Routine)
void CALLBACK CompletionRoutine(DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
{
    if (error != 0 || bytesTransferred == 0)
    {
        std::cerr << "CompletionRoutine error: " << error << std::endl;
        return;
    }

    OverlappedEx* overlappedEx = (OverlappedEx*)overlapped;
    overlappedEx->buffer[bytesTransferred] = '\0';

    std::cout << "Received: " << overlappedEx->buffer << std::endl;

    // 클라이언트에게 다시 보내기 (Echo)
    DWORD bytesSent;
    if (overlappedEx->socket != INVALID_SOCKET)
    {
        if (WSASend(overlappedEx->socket, &overlappedEx->wsaBuf, 1, &bytesSent, 0, overlapped, NULL) == SOCKET_ERROR)
        {
            int errorCode = WSAGetLastError();
            if (errorCode != WSA_IO_PENDING)
            {
                std::cerr << "WSASend failed with error: " << errorCode << std::endl;
                return;
            }
        }
        else
        {
            // 데이터 전송 후 이벤트 핸들러 호출
            HandleEvent(overlappedEx->buffer);
        }
    }
    else
    {
        std::cerr << "Invalid socket in CompletionRoutine" << std::endl;
    }
}

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create listen socket" << std::endl;
        return 1;
    }

    SOCKADDR_IN serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening on port " << PORT << std::endl;

    SOCKET clientSocket;
    SOCKADDR_IN clientAddr{};
    int addrSize = sizeof(clientAddr);
    clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrSize);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Accept failed" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected!" << std::endl;

    OverlappedEx* overlappedEx = new OverlappedEx();
    overlappedEx->socket = clientSocket;
    overlappedEx->wsaBuf.buf = overlappedEx->buffer;
    overlappedEx->wsaBuf.len = sizeof(overlappedEx->buffer);
    ZeroMemory(&(overlappedEx->overlapped), sizeof(WSAOVERLAPPED));

    DWORD flags = 0;
    DWORD bytesReceived;
    int result = WSARecv(clientSocket, &(overlappedEx->wsaBuf), 1, &bytesReceived, &flags, (LPWSAOVERLAPPED)overlappedEx, CompletionRoutine);

    if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
    {
        std::cerr << "WSARecv failed with error: " << WSAGetLastError() << std::endl;
        delete overlappedEx;
    }

    // 콜백 함수 실행을 위해 Alertable Wait 상태 유지
    while (true)
    {
        SleepEx(INFINITE, TRUE);  // Alertable Wait
    }

    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}