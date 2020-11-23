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
			printf("bye\n");
			break;
		}
		else if (0 == strcmp(cmd_buf, "login")) 
		{
			Login login = { "cnj","1234" };
			DataHeader header = { sizeof(login), CMD_LOGIN };
			send(_sock, (const char*)&header, sizeof(header), 0);
			send(_sock, (const char*)&login, sizeof(login), 0);

			DataHeader recv_header = {};
			LoginResponse loginResponse = {};
			recv(_sock, (char*)&recv_header, sizeof(recv_header), 0);
			recv(_sock, (char*)&loginResponse, sizeof(loginResponse), 0);
			printf("login result is: %d\n", loginResponse.result);
		}
		else if (0 == strcmp(cmd_buf, "logout"))
		{
			Logout logout = { "cnj" };
			DataHeader header = { sizeof(logout), CMD_LOGOUT };
			send(_sock, (const char*)&header, sizeof(header), 0);
			send(_sock, (const char*)&logout, sizeof(logout), 0);

			DataHeader recv_header = {};
			LogoutResponse logoutResponse = {};
			recv(_sock, (char*)&recv_header, sizeof(recv_header), 0);
			recv(_sock, (char*)&logoutResponse, sizeof(logoutResponse), 0);
			printf("logout result is: %d\n", logoutResponse.result);
		}
		else
		{
			printf("this command was not supported.\n");
			continue;
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