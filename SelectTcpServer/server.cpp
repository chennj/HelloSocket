// linux compile command
// g++ server.cpp -std=c++11 -pthread -o server
// ----------------------------------
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN	//try import small def loog ago
#include<Windows.h>
#include<WinSock2.h>
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<string.h>

#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR		    (-1)
#endif

#include<stdio.h>
#include<vector>

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
		data_length = sizeof(UnknownResponse);
		cmd = CMD_UNKNOWN;
		strcpy(msg, "unknown command");
	}
	char msg[32];
};

struct Login : public DataHeader
{
	Login()
	{
	}

	char username[32];
	char password[32];
};

struct LoginResponse : public DataHeader
{
	LoginResponse()
	{
		data_length = sizeof(LoginResponse);
		cmd = CMD_LOGIN_RESPONSE;
		result = 1;
	}
	int result;
};

struct Logout : public DataHeader
{
	Logout()
	{
	}

	char username[32];
};

struct LogoutResponse : public DataHeader
{
	LogoutResponse()
	{
		data_length = sizeof(LogoutResponse);
		cmd = CMD_LOGOUT_RESPONSE;
		result = 1;
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

std::vector<SOCKET> g_clients;

int proc(SOCKET sock_client)
{

	char szRecv[1024] = {};

	// 5.1 receive client request
	int nlen = recv(sock_client, szRecv, sizeof(DataHeader), 0);
	if (nlen <= 0)
	{
		printf("client socket %d offline\n", sock_client);
		return -1;
	}

	DataHeader * header = (DataHeader*)szRecv;

	// 5.2 deal client command
	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		recv(sock_client, szRecv + sizeof(DataHeader), header->data_length - sizeof(DataHeader), 0);
		Login* ret = (Login*)szRecv;
		// ignore to check user name and password
		// ...
		printf("receive command is: CMD_LOGIN , data length: %d, user name: %s, pwd: %s\n", header->data_length, ret->username, ret->password);

		LoginResponse loginResponse;
		send(sock_client, (char*)&loginResponse, sizeof(LoginResponse), 0);
	}
	break;
	case CMD_LOGOUT:
	{
		Logout logout;
		recv(sock_client, (char*)&logout + sizeof(DataHeader), header->data_length - sizeof(DataHeader), 0);
		printf("receive command is: CMD_LOGOUT , data length: %d, user name: %s\n", header->data_length, logout.username);

		LogoutResponse logoutResponse;
		send(sock_client, (char*)&logoutResponse, sizeof(logoutResponse), 0);
	}
	break;
	default:
	{
		UnknownResponse unknownResponse;
		send(sock_client, (char*)&unknownResponse, sizeof(unknownResponse), 0);
	}
	break;
	}
	return 0;
}

int main()
{
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
#endif

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
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");
#else
	_sin.sin_addr.s_addr = INADDR_ANY;
#endif
	if (bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		printf("bind failure because port used by other\n");
#ifdef _WIN32
		closesocket(_sock);
#else
		close(_sock);
#endif
		return 0;
	}
	printf("bind net port success.\n");

	// 3 listen port
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("listen failure;");
#ifdef _WIN32
		closesocket(_sock);
#else
		close(_sock);
#endif
		return 0;
	}
	printf("listenning... \n");

	// 5 recive client request and deal
	while (true)
	{
		//伯克利 socket
		fd_set fd_read;
		fd_set fd_write;
		fd_set fd_exception;

		FD_ZERO(&fd_read);
		FD_ZERO(&fd_write);
		FD_ZERO(&fd_exception);

		// put server's socket in all the fd_set
		FD_SET(_sock, &fd_read);
		FD_SET(_sock, &fd_write);
		FD_SET(_sock, &fd_exception);

		SOCKET max_socket = _sock;
		for (int n = (int)g_clients.size() - 1; n >= 0; n--)
		{
			max_socket = g_clients[n]>max_socket ? g_clients[n] : max_socket;
			FD_SET(g_clients[n], &fd_read);
		}

		//nfds is range of fd_set, not fd_set's count.
		//nfds is also max value+1 of all the file descriptor(socket).
		//nfds can be 0 in the windows.
		//that timeval was setted null means blocking, not null means nonblocking.
		timeval t = { 0,100 };
		int ret = select(max_socket + 1/*nfds*/, &fd_read, &fd_write, &fd_exception, &t);
		if (ret < 0)
		{
			printf("error occurs while select and mission finish.\n");
			break;
		}

		if (FD_ISSET(_sock, &fd_read))
		{
			FD_CLR(_sock, &fd_read);

			// 4 waitting for accept a client's connection
			sockaddr_in client_addr = {};
			int client_addr_len = sizeof(sockaddr_in);	// note: accept will fall without sizeof(sockaddr_in)
			SOCKET _sock_client = INVALID_SOCKET;
#ifdef _WIN32
			_sock_client = accept(_sock, (sockaddr*)&client_addr, &client_addr_len);
#else
			_sock_client = accept(_sock, (sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
#endif
			if (INVALID_SOCKET == _sock_client)
			{
				printf("accept a invalid client socket\n");
			}

			// boardcast
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				NewUserJoin user_join;
				send(g_clients[n], (const char*)&user_join, sizeof(NewUserJoin), 0);
			}

			g_clients.push_back(_sock_client);

			printf("a new client enter, socket: %d,ip:%s \n", (int)_sock, inet_ntoa(client_addr.sin_addr));
		}

#ifdef _WIN32
		for (size_t n = 0; n < fd_read.fd_count; n++)
		{
			if (-1 == proc(fd_read.fd_array[n]))
			{
				auto iter = find(g_clients.begin(), g_clients.end(), fd_read.fd_array[n]);
				if (iter != g_clients.end())
				{
					g_clients.erase(iter);
				}
			}
		}
#else
		for (int n = (int)g_clients.size() - 1; n >= 0; n--)
		{
			if (FD_ISSET(g_clients[n], &fd_read))
			{
				if (-1 == proc(g_clients[n]))
				{
					//auto equals std::vector<SOCKET>::iterator
					//auto iter = g_clients.begin();
					//while (iter != g_clients.end())
					//{
					//	if (iter[n] == g_clients[n])
					//	{
					//		g_clients.erase(iter);
					//		break;
					//	}
					//	iter++;
					//}
					//优化
					auto iter = g_clients.begin() + n;
					if (iter != g_clients.end())
					{
						g_clients.erase(iter);
					}
				}
			}
		}
#endif

		// select(...) must be select(...,CAN'T BE NULL);
		//printf("process other task while idle.\n");
	}

	// 6 close socket
#ifdef _WIN32
	for (size_t n = g_clients.size() - 1; n >= 0; n--)
	{
		closesocket(g_clients[n]);
	}
	// clean windows socket environment
	WSACleanup();
#else
	for (int n = (int)g_clients.size() - 1; n >= 0; n--)
	{
		close(g_clients[n]);
	}
#endif

	printf("server is shutdown\n");

	return 0;
}