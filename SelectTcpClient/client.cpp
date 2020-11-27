// linux compile command
// g++ client.cpp -std=c++11 -pthread -o client
// ----------------------------------
#define WIN32_LEAN_AND_MEAN	//try import small def loog ago

#ifdef _WIN32
#include<Windows.h>
#include<WinSock2.h>
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include <string.h>

#define SOCKET int
#define INVALIDE_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR			(-1)
#endif
#include<stdio.h>
#include<thread>

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESPONSE,
	CMD_LOGOUT,
	CMD_LOGOUT_RESPONSE,
	CMD_NEW_USER_JOIN,
	CMD_UNKNOWN
};

// msg header
struct DataHeader
{
	short data_length;
	short cmd;
};

struct UnknownResponse : public DataHeader
{
	UnknownResponse()
	{
	}
	char msg[32];
};

struct Login : public DataHeader
{
public:
	Login()
	{
		data_length = sizeof(Login);
		cmd = CMD_LOGIN;
	}

	char username[32];
	char password[32];
};

struct LoginResponse : public DataHeader
{
	LoginResponse()
	{
	}
	int result;
};

struct Logout : public DataHeader
{
public:
	Logout()
	{
		data_length = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}

	char username[32];
};

struct LogoutResponse : public DataHeader
{
	LogoutResponse()
	{
	}
	int result;
};

struct NewUserJoin : public DataHeader
{
	NewUserJoin()
	{
		data_length = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

int proc(SOCKET sock_local)
{

	char szRecv[1024] = {};

	// 5.1 receive client request
	int nlen = recv(sock_local, szRecv, sizeof(DataHeader), 0);
	if (nlen <= 0)
	{
		printf("disconnected to server.\n");
		return -1;
	}

	DataHeader * header = (DataHeader*)szRecv;

	// 5.2 deal client command
	switch (header->cmd)
	{
	case CMD_LOGIN_RESPONSE:
	{
		recv(sock_local, szRecv + sizeof(DataHeader), header->data_length - sizeof(DataHeader), 0);
		LoginResponse* ret = (LoginResponse*)szRecv;
		printf("receive server msg: CMD_LOGIN_RESPONSE, data length: %d, result: %d\n", header->data_length, ret->result);
	}
	break;
	case CMD_LOGOUT_RESPONSE:
	{
		LogoutResponse logoutResponse;
		recv(sock_local, (char*)&logoutResponse + sizeof(DataHeader), header->data_length - sizeof(DataHeader), 0);
		printf("receive server msg: CMD_LOGOUT_RESPONSE, data length: %d, result: %d\n", header->data_length, logoutResponse.result);
	}
	break;
	case CMD_NEW_USER_JOIN:
	{
		recv(sock_local, szRecv + sizeof(DataHeader), header->data_length - sizeof(DataHeader), 0);
		NewUserJoin* ret = (NewUserJoin*)szRecv;
		printf("receive server msg: CMD_NEW_USER_JOIN, data length: %d, result: %d\n", header->data_length, ret->sock);
	}
	break;
	default:
	{
		recv(sock_local, szRecv + sizeof(DataHeader), header->data_length - sizeof(DataHeader), 0);
		printf("receive server msg: UNKNOWN, data length: %d, data: %s\n", header->data_length, szRecv);
	}
	break;
	}
	return 0;
}

bool g_bRun = true;

void cmd_thread(SOCKET _sock)
{
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
			Login login;
			strcpy(login.username, "cnj");
			strcpy(login.password, "cnj123");
			send(_sock, (const char*)&login, sizeof(login), 0);
		}
		else if (0 == strcmp(cmd_buf, "logout"))
		{
			Logout logout;
			strcpy(logout.username, "cnj");
			send(_sock, (const char*)&logout, sizeof(logout), 0);
		}
		else
		{
			printf("this command was not supported.\n");
		}
	}

	g_bRun = false;

}
int main()
{
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
#endif
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
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = inet_addr("192.168.137.1");
#else
	_sin.sin_addr.s_addr = inet_addr("192.168.137.1");
#endif
	if (SOCKET_ERROR == connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))
	{
		printf("connect server failure.\n");
		return 0;
	}
	printf("connect server success.\n");

	// 3 send request to server and receive resposne from server
	// start up cmd thread

	std::thread t1(cmd_thread, _sock);
	t1.detach();

	while (g_bRun)
	{
#ifdef _WIN32
		FD_SET fdReads;
#else
		fd_set fdReads;
#endif
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);

		timeval t = { 1,0 };
		int ret = select(_sock + 1, &fdReads, 0, 0, &t);
		if (ret < 0)
		{
			printf("error occurs while listen server\n");
			break;
		}

		if (FD_ISSET(_sock, &fdReads))
		{
			FD_CLR(_sock, &fdReads);

			int ret = proc(_sock);
			if (-1 == ret)
			{
				break;
			}
		}

	}

	printf("client is exit\n");

#ifdef _WIN32
	// 4 close socket
	closesocket(_sock);

	// clean windows socket environment
	WSACleanup();
#else
	close(_sock);
#endif
	return 0;
}