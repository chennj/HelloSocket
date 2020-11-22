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

	// 3 recv from server
	char buf[1024] = {0};
	int nlen = recv(_sock, buf, 1024, 0);
	if (nlen > 0)
	{
		printf("received msg from server is %s\n", buf);
	}

	printf("received msg from server is null\n");

	// 4 close socket
	closesocket(_sock);

	// clean windows socket environment
	WSACleanup();

	return 0;
}