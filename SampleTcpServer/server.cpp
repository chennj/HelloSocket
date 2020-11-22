#define WIN32_LEAN_AND_MEAN	//try import small def loog ago

#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>


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

	_sock_client = accept(_sock, (sockaddr*)&client_addr, &client_addr_len);
	if (INVALID_SOCKET == _sock_client)
	{
		printf("accept a invalid client socket\n");
	}
	printf("a new client enter, ip:%s \n", inet_ntoa(client_addr.sin_addr));

	// 5 recive client request and deal
	char _recv_buf[128] = {};
	while (true)
	{
		// 5.1 receive client request
		int nlen = recv(_sock_client, _recv_buf, 128, 0);
		if (nlen <= 0)
		{
			printf("error occurs while receive client request and mission finish.\n");
			break;
		}
		// 5.2 deal client request
		if (0 == strcmp(_recv_buf, "cmd_get_name"))
		{
			char msg_buf[] = "nobody";
			send(_sock_client, msg_buf, strlen(msg_buf) + 1, 0);
		}
		else if (0 == strcmp(_recv_buf, "cmd_get_age"))
		{
			char msg_buf[] = "80";
			send(_sock_client, msg_buf, strlen(msg_buf) + 1, 0);
		}
		else
		{
			char msg_buf[] = "???.";
			send(_sock_client, msg_buf, strlen(msg_buf) + 1, 0);
		}

	}

	// 6 close socket
	closesocket(_sock);

	// clean windows socket environment
	WSACleanup();

	return 0;
}