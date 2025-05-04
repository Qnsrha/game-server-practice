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

// �̺�Ʈ �ڵ鷯 �Լ�
void HandleEvent(const std::string& data)
{
    std::cout << "recv complete :" << data << std::endl;
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

    std::cout << "Send to server: " << overlappedEx->buffer << std::endl;

    Sleep(1000);  //recv �������� ����

    // �����κ��� �߰� �����͸� �����ϱ� ���� WSARecv ȣ��
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
        // ���ŵ� �����͸� �̺�Ʈ �ڵ鷯�� ����
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
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // ���� IP �ּ�
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

    // �ݹ� �Լ� ������ ���� Alertable Wait ���� ����
    while (true)
    {
        SleepEx(INFINITE, TRUE);  // Alertable Wait
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}