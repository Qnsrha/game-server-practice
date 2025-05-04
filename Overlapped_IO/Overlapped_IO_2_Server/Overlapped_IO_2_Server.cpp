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

// �̺�Ʈ �ڵ鷯 �Լ�
void HandleEvent(const std::string& data)
{
    std::cout << "Echoed back: " << data << std::endl;
    // ���⼭ �߰����� �̺�Ʈ ��� ó���� ������ �� �ֽ��ϴ�.
}

// �ݹ� �Լ� (Completion Routine)
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

    // Ŭ���̾�Ʈ���� �ٽ� ������ (Echo)
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
            // ������ ���� �� �̺�Ʈ �ڵ鷯 ȣ��
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

    // �ݹ� �Լ� ������ ���� Alertable Wait ���� ����
    while (true)
    {
        SleepEx(INFINITE, TRUE);  // Alertable Wait
    }

    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}