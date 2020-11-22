#define WIN32_LEAN_AND_MEAN	//try import small def loog ago

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>

//可以配置再工程-属性-链接器-输入中
//#pragma comment(lib, "ws2_32.lib")

int main()
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);

	//---------
	//-- create sample tcp server use socket api

	// 1 create socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == _sock)
	{
		printf("create socket failure\n");
		return 0;
	}
	printf("create socket success\n");

	// 2 bind port, which is be connected by client
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(12345);
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");
	if (bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		printf("bind failure because port used by other\n");
		closesocket(_sock);
		return 0;
	}
	printf("bind net port success.\n");

	// 3 listen port
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("listen failure;");
		closesocket(_sock);
		return 0;
	}
	printf("listenning... \n");

	// 4 waitting for accept a client's connection
	sockaddr_in client_addr = {};
	int client_addr_len = sizeof(sockaddr_in);	// note: accept will fall without sizeof(sockaddr_in)
	SOCKET _sock_client = INVALID_SOCKET;
ACCEPT:
	_sock_client = accept(_sock, (sockaddr*)&client_addr, &client_addr_len);
	if (INVALID_SOCKET == _sock_client)
	{
		printf("accept a invalid client socket\n");
	}
	printf("accept a client ip:%s \n" , inet_ntoa(client_addr.sin_addr));

	// 5 send message to client
	char msg_buf[] = "Hello, I'm server.";
	send(_sock_client, msg_buf, strlen(msg_buf)+1, 0);

	goto
		ACCEPT;

	// 6 close socket
	closesocket(_sock);

	// clean windows socket environment
	WSACleanup();

	return 0;
}