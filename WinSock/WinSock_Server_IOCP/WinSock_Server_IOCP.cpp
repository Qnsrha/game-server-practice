#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

struct PER_IO_DATA
{
    OVERLAPPED overlapped;
    WSABUF wsaBuf;
    char buffer[BUFFER_SIZE];
    int operationType; // 0: RECV, 1: SEND
};

HANDLE hIOCP;
SOCKET listenSocket;

void WorkerThread()
{
    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    PER_IO_DATA* perIoData;

    while (true)
    {
        BOOL result = GetQueuedCompletionStatus(
            hIOCP, &bytesTransferred, &completionKey, (LPOVERLAPPED*)&perIoData, INFINITE);

        if (!result || bytesTransferred == 0)
        {
            closesocket((SOCKET)completionKey);
            delete perIoData;
            continue;
        }

        SOCKET clientSocket = (SOCKET)completionKey;

        if (perIoData->operationType == 0) // RECV 완료
        {
            perIoData->buffer[bytesTransferred] = '\0'; // 받은 데이터의 끝에 NULL 추가
            std::cout << "Received: " << perIoData->buffer << std::endl;

            // Echo back to client
            perIoData->operationType = 1;
            DWORD flags = 0;

            perIoData->wsaBuf.buf = perIoData->buffer;
            perIoData->wsaBuf.len = bytesTransferred; // 실제 받은 데이터 길이로 설정

            int res = WSASend(clientSocket, &perIoData->wsaBuf, 1, NULL, flags, &perIoData->overlapped, NULL);
            if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
            {
                std::cerr << "WSASend failed: " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                delete perIoData;
            }
        }
        else // SEND 완료 -> 다시 RECV 요청
        {
            perIoData->operationType = 0;
            DWORD flags = 0;
            ZeroMemory(&perIoData->overlapped, sizeof(OVERLAPPED));

            perIoData->wsaBuf.buf = perIoData->buffer;
            perIoData->wsaBuf.len = BUFFER_SIZE;

            int res = WSARecv(clientSocket, &perIoData->wsaBuf, 1, NULL, &flags, &perIoData->overlapped, NULL);
            if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
            {
                std::cerr << "WSARecv failed: " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                delete perIoData;
            }
        }
    }
}

void AcceptThread()
{
    while (true)
    {
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

        if (clientSocket == INVALID_SOCKET)
        {
            continue;
        }

        std::cout << "Client Connected!" << std::endl;
        CreateIoCompletionPort((HANDLE)clientSocket, hIOCP, (ULONG_PTR)clientSocket, 0);

        PER_IO_DATA* perIoData = new PER_IO_DATA();
        ZeroMemory(&perIoData->overlapped, sizeof(OVERLAPPED));

        perIoData->operationType = 0;
        perIoData->wsaBuf.buf = perIoData->buffer;
        perIoData->wsaBuf.len = BUFFER_SIZE;

        DWORD flags = 0;
        int res = WSARecv(clientSocket, &perIoData->wsaBuf, 1, NULL, &flags, &perIoData->overlapped, NULL);
        if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        {
            std::cerr << "WSARecv failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            delete perIoData;
        }
    }
}

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    listen(listenSocket, SOMAXCONN);

    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    CreateIoCompletionPort((HANDLE)listenSocket, hIOCP, (ULONG_PTR)listenSocket, 0);

    std::vector<HANDLE> threads;
    for (int i = 0; i < 4; ++i)
    {
        HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkerThread, NULL, 0, NULL);
        threads.push_back(hThread);
    }

    AcceptThread();

    for (HANDLE hThread : threads)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
