#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h> // Add this header for inet_pton

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

using namespace std;

int main()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(PORT);

	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cerr << "Connection failed!" << "\n";
		return 1;
	}

	char buffer[BUFFER_SIZE];
	while (true)
	{
		cout << "Send: ";
		cin.getline(buffer, BUFFER_SIZE);

		send(clientSocket, buffer, strlen(buffer), 0);

		Sleep(1000);
		int recvLen = recv(clientSocket, buffer, strlen(buffer), 0);
		if (recvLen == SOCKET_ERROR || recvLen >= BUFFER_SIZE) {
			cerr << "Receive failed or buffer overflow!" << "\n";
			return 1;
		}

		if (recvLen == 0) // 연결 종료 확인
		{
			cout << "Connection closed by server.\n";
			break; // 연결 종료
		}
		else if (recvLen == SOCKET_ERROR) // 에러 처리
		{
			cerr << "Recv failed: " << WSAGetLastError() << "\n";
			break;
		}
		else // 수신된 데이터 출력
		{
			buffer[recvLen] = '\0'; // 받은 데이터의 끝에 NULL 추가
			cout << "Echo: " << buffer << "\n";
		}
	}

	closesocket(clientSocket);
	WSACleanup();
	return 0;
}
