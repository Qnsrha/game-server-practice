#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h> // Add this header for inet_pton
#include <iostream>
#include <string>

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
    std::cout << "recv complete :" << data << std::endl;
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

    std::cout << "Send to server: " << overlappedEx->buffer << std::endl;

    Sleep(1000);  //recv 먼저실행 방지

    // 서버로부터 추가 데이터를 수신하기 위해 WSARecv 호출
    DWORD bytesReceived;
    DWORD recvFlags = 0;
    if (WSARecv(overlappedEx->socket, &overlappedEx->wsaBuf, 1, &bytesReceived, &recvFlags, (LPWSAOVERLAPPED)overlappedEx, NULL) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            std::cerr << "WSARecv failed" << std::endl;
        }
    }
    else
    {
        // 수신된 데이터를 이벤트 핸들러로 전달
        HandleEvent(overlappedEx->buffer);
    }
}

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN serverAddr{};
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // 서버 IP 주소
    serverAddr.sin_port = htons(PORT);

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Connect failed" << std::endl;
        return -1;
    }

    std::cout << "Connected to server!" << std::endl;

    OverlappedEx* sendOverlapped = new OverlappedEx();
    sendOverlapped->socket = clientSocket;
    sendOverlapped->wsaBuf.buf = sendOverlapped->buffer;
    sendOverlapped->wsaBuf.len = BUFFER_SIZE;

    strcpy_s(sendOverlapped->buffer, "Hello, Server!");

    DWORD bytesSent;
    if (WSASend(clientSocket, &sendOverlapped->wsaBuf, 1, &bytesSent, 0, (LPWSAOVERLAPPED)sendOverlapped, CompletionRoutine) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            std::cerr << "WSASend failed" << std::endl;
        }
    }

    // 콜백 함수 실행을 위해 Alertable Wait 상태 유지
    while (true)
    {
        SleepEx(INFINITE, TRUE);  // Alertable Wait
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}