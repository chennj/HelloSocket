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
	//-- create sample tcp client use socket api
	// 1 create socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == _sock)
	{
		printf("create socket failure.\n");
		return 0;
	}
	printf("create socket success.\n");

	// 2 connect server
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(12345);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (SOCKET_ERROR == connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))
	{
		printf("connect server failure.\n");
		return 0;
	}
	printf("connect server success.\n");

	// 3 send request to server and receive resposne from server
	while (true)
	{
		char cmd_buf[128] = {};
		scanf("%s", cmd_buf);
		if (0 == strcmp(cmd_buf, "exit"))
		{
			break;
		}
		else {
			send(_sock, cmd_buf, strlen(cmd_buf) + 1, 0);
		}

		char buf[1024] = { 0 };
		int nlen = recv(_sock, buf, 1024, 0);
		if (nlen > 0)
		{
			printf("that received data from server is %s\n", buf);
		}

	}

	printf("client is exit\n");

	// 4 close socket
	closesocket(_sock);

	// clean windows socket environment
	WSACleanup();

	// exit
	printf("server is shut down \n");

	return 0;
}