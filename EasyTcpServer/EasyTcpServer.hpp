#ifndef _EASYTCPSERVER_HPP_
#define _EASYTCPSERVER_HPP_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include <string.h>

#define SOCKET int
#define INVALID_SOCKET	(SOCKET)(~0)
#define SOCKET_ERROR			(-1)
#endif
#include<stdio.h>
#include<vector>
#include"MessageHeader.hpp"

class EasyTcpServer
{
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTcpServer()
	{
		Close();
	}

public:
	// initialize socket
	int InitSocket()
	{
		if (INVALID_SOCKET != _sock)
		{
			printf("close previous connection socket<%d>\n", (int)_sock);
			Close();
		}

#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(ver, &data);
#endif
		// 1 create socket
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (SOCKET_ERROR == _sock)
		{
			printf("create socket failure.\n");
			return _sock;
		}
		printf("socket<%d> create success.\n", (int)_sock);
		return _sock;
	}

	// bind ip and port
	int Bind(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}

		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		if (ip)
		{
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip)
		{
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (ret == SOCKET_ERROR)
		{
			printf("socket<%d> bind failure because port used by other.\n", (int)_sock);
			Close();
			return ret;
		}
		printf("socket<%d> bind port<%d> success.\n", (int)_sock, port);
		return ret;
	}

	// listen port
	int Listen(int n=5)
	{
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("socket<%d> listen failure.\n", (int)_sock);
			Close();
			return ret;
		}
		printf("socket<%d> listenning ... \n", (int)_sock);
		return ret;
	}

	// accept client's connectin
	SOCKET Accept()
	{
		sockaddr_in client_addr = {};
		int client_addr_len = sizeof(sockaddr_in);	// note: accept will fall without sizeof(sockaddr_in)
		SOCKET sock_client = INVALID_SOCKET;

#ifdef _WIN32
		sock_client = accept(_sock, (sockaddr*)&client_addr, &client_addr_len);
#else
		sock_client = accept(_sock, (sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
#endif
		if (INVALID_SOCKET == sock_client)
		{
			printf("socket<%d> accept a invalid client request.\n", (int)_sock);
			return sock_client;
		}

		// boardcast
		NewUserJoin user_join;
		SendDataToAll(&user_join);

		g_clients.push_back(sock_client);

		printf("socket<%d>a new client enter: socket<%d>,ip<%s> \n", (int)_sock, (int)sock_client, inet_ntoa(client_addr.sin_addr));

		return sock_client;
	}

	// close socket
	void Close()
	{
		if (_sock == INVALID_SOCKET) return;

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

		_sock = INVALID_SOCKET;

		printf("server is shutdown\n");
	}

	// deal net message
	int OnRun()
	{
		if (!IsRunning())
		{
			return -1;
		}

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
		timeval t = { 0,10 };
		int ret = select(max_socket + 1/*nfds*/, &fd_read, &fd_write, &fd_exception, &t);
		if (ret < 0)
		{
			printf("socket<%d> error occurs while select and mission finish.\n", (int)_sock);
			Close();
			return ret;
		}

		if (FD_ISSET(_sock, &fd_read))
		{
			FD_CLR(_sock, &fd_read);

			Accept();
		}

#ifdef _WIN32
		for (size_t n = 0; n < fd_read.fd_count; n++)
		{
			if (-1 == RecvData(fd_read.fd_array[n]))
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
				if (-1 == RecvData(g_clients[n]))
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
					//”≈ªØ
					auto iter = g_clients.begin() + n;
					if (iter != g_clients.end())
					{
						g_clients.erase(iter);
					}
				}
			}
		}
#endif
	}

	// if is running
	bool IsRunning()
	{
		return _sock != INVALID_SOCKET;
	}

	// receive data, deal sticking package and splitting package
	int RecvData(SOCKET sock_client)
	{

		char szRecv[4096] = {};

		// 5.1 receive client request
		int nlen = recv(sock_client, szRecv, sizeof(DataHeader), 0);
		if (nlen <= 0)
		{
			printf("client socket <%d> offline\n", (int)sock_client);
			return -1;
		}

		DataHeader * header = (DataHeader*)szRecv;

		nlen = recv(sock_client, szRecv + sizeof(DataHeader), header->data_length - sizeof(DataHeader), 0);
		if (nlen <= 0)
		{
			printf("client socket<%d> offline\n", (int)sock_client);
			return -1;
		}

		OnNetMessage(header, sock_client);
		return 0;
	}

	// response net message
	virtual void OnNetMessage(DataHeader* pheader, SOCKET sock_client)
	{
		switch (pheader->cmd)
		{
		case CMD_LOGIN:
		{
			Login* ret = (Login*)pheader;
			printf("socket<%d> receive client socket<%d> request: CMD_LOGIN , data length<%d>, user name<%s>, pwd<%s>\n", (int)_sock, (int)sock_client, pheader->data_length, ret->username, ret->password);

			LoginResponse loginResponse;
			SendData(sock_client, &loginResponse);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* ret = (Logout*)pheader;
			printf("socket<%d> receive client socket<%d> request: CMD_LOGOUT , data length<%d>, user name<%s>\n", (int)_sock, (int)sock_client, pheader->data_length, ret->username);

			LogoutResponse logoutResponse;
			SendData(sock_client, &logoutResponse);
		}
		break;
		default:
		{
			UnknownResponse unknown;
			SendData(sock_client, &unknown);
		}
		break;
		}
	}

	// send data
	int SendData(SOCKET sock_client, DataHeader * pheader)
	{
		if (IsRunning() && pheader)
		{
			int ret = send(sock_client, (const char*)pheader, pheader->data_length, 0);
			printf("socket<%d> send client socket<%d> response: dataLen<%d>, sendResult<%d>\n", (int)_sock, (int)sock_client, pheader->data_length, ret);
			return ret;

		}
		return SOCKET_ERROR;
	}

	// boardcast
	void SendDataToAll(DataHeader * pheader)
	{
		for (int n = (int)g_clients.size() - 1; n >= 0; n--)
		{
			SendData(g_clients[n], pheader);
		}
	}

private:
	SOCKET _sock;
	std::vector<SOCKET> g_clients;
};


#endif