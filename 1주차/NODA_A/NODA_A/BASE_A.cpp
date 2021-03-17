#include <iostream>
#include <map>
using namespace std;
#include <WS2tcpip.h>
#include "conn.h"
#pragma comment(lib, "Ws2_32.lib")

#define MAX_BUFFER        1024
#define SERVER_PORT        3500

void do_node_a();

struct SOCKETINFO
{
	WSAOVERLAPPED overlapped;
	WSABUF dataBuffer;
	SOCKET socket;
	char messageBuffer[MAX_BUFFER];
};


SOCKETINFO node_B;
CONN g_conn;

void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);

void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	if (0 != Error) {
		wcout << L"연결이 끊어졌습니다. 종료합니다.";
		exit(0);
	}

	SOCKET client_s = node_B.socket;

	if (dataBytes == 0)
	{
		closesocket(node_B.socket);
		return;
	}  // 클라이언트가 closesocket을 했을 경우
	//cout << "From client received " << dataBytes << " bytes\n";
	node_B.dataBuffer.len = dataBytes;
	memset(&(node_B.overlapped), 0, sizeof(WSAOVERLAPPED));
	for (DWORD i = 0; i < dataBytes; ++i)
		g_conn.set_value(node_B.messageBuffer[i] == 1);
	

	node_B.dataBuffer.len = MAX_BUFFER;
	node_B.dataBuffer.buf = node_B.messageBuffer;
	memset(&node_B.overlapped, 0, sizeof(WSAOVERLAPPED));
	DWORD flags = 0;
	WSARecv(node_B.socket, &node_B.dataBuffer, 1, NULL, &flags, &(node_B.overlapped), recv_callback);
}

void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	SOCKET client_s = node_B.socket;
	if (0 != Error) {
		wcout << L"연결이 끊어졌습니다. 종료합니다.";
		exit(0);
	}

	if (dataBytes == 0) {
		closesocket(node_B.socket);
		wcout << L"Node B가 사라졌습니다.\n";
		return;
	}  // 클라이언트가 closesocket을 했을 경우

	delete overlapped;

}
void send_value(bool value)
{
	SOCKETINFO* over = new SOCKETINFO;
	over->dataBuffer.len = 1;
	over->dataBuffer.buf = over->messageBuffer;
	memset(&(over->overlapped), 0, sizeof(WSAOVERLAPPED));
	if (true == value) over->messageBuffer[0] = 1;
	else over->messageBuffer[0] = 0;
	WSASend(node_B.socket, &(over->dataBuffer), 1, NULL, 0, &(over->overlapped), send_callback);
}


CONN::CONN() : stat(false)
{
}
void CONN::set(bool value)
{
	stat = value;
	SleepEx(0, TRUE);
	send_value(value);
}

void CONN::set_value(bool value)
{
	stat = value;
}

bool CONN::get()
{
	SleepEx(0, TRUE);
	return stat;
}




int main()
{
	wcout.imbue(std::locale("korean"));

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	wcout << L"노드 A를 시작합니다.\n";


	while (SOCKET_ERROR == ::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN))) {
		wcout << L"시작할 수 없습니다. 노드 A가 이미 실행 중인 것으로 보입니다.\n";
		exit(-1);
	}


	listen(listenSocket, 5);
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	memset(&clientAddr, 0, addrLen);


	wcout << L"노드 B의 실행을 기다리고 있습니다.\n";
	SOCKET clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &addrLen);
	wcout << L"노드 B와 연결되었습니다.\n";
	node_B.socket = clientSocket;
	node_B.dataBuffer.len = MAX_BUFFER;
	node_B.dataBuffer.buf = node_B.messageBuffer;
	memset(&node_B.overlapped, 0, sizeof(WSAOVERLAPPED));
	DWORD flags = 0;
	WSARecv(node_B.socket, &node_B.dataBuffer, 1, NULL, &flags, &(node_B.overlapped), recv_callback);

	do_node_a();

	closesocket(listenSocket);
	WSACleanup();
}

