#define WIN32_LEAN_AND_MEAN	//try import small def loog ago

#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>

enum CMD
{
	CMD_LOGIN, CMD_LOGOUT, CMD_UNKNOWN
};

// msg header
struct DataHeader
{
	short data_length;
	short cmd;
};

struct UnknownResponse {
	char msg[32];
};

struct Login
{
	char user_name[32];
	char pass_word[32];
};

struct LoginResponse
{
	int result;
};

struct Logout
{
	char user_name[32];
};

struct LogoutResponse
{
	int result;
};

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
	printf("a new client enter, socket: %d,ip:%s \n", (int)_sock, inet_ntoa(client_addr.sin_addr));

	// 5 recive client request and deal
	while (true)
	{
		DataHeader header = {};

		// 5.1 receive client request
		int nlen = recv(_sock_client, (char *)&header, sizeof(header), 0);
		if (nlen <= 0)
		{
			printf("error occurs while receive client request and mission finish.\n");
			break;
		}
		printf("receive command is: %d, data length:%d\n", header.cmd, header.data_length);

		// 5.2 deal client command
		switch (header.cmd)
		{
		case CMD_LOGIN:
		{
			Login login = {};
			nlen = recv(_sock_client, (char*)&login, sizeof(Login), 0);
			// ignore to check user name and password
			// ...
			LoginResponse loginResponse = {0};
			send(_sock_client, (char*)&header, sizeof(DataHeader), 0);
			send(_sock_client, (char*)&loginResponse, sizeof(LoginResponse), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout logout = {};
			nlen = recv(_sock_client, (char*)&logout, sizeof(logout), 0);

			LogoutResponse logoutResponse = { 1 };
			send(_sock_client, (char*)&header, sizeof(DataHeader), 0);
			send(_sock_client, (char*)&logoutResponse, sizeof(logoutResponse), 0);
		}
		break;
		default:
		{
			header = { CMD_UNKNOWN, 0 };
			UnknownResponse unknownResponse = { "unknown command." };
			send(_sock_client, (char*)&header, sizeof(header), 0);
			send(_sock_client, (char*)&unknownResponse, sizeof(unknownResponse), 0);
		}
		break;
		}

	}

	// 6 close socket
	closesocket(_sock);

	// clean windows socket environment
	WSACleanup();

	return 0;
}