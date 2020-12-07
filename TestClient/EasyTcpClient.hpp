#ifndef _EASYTCPCLIENT_TCP_
#define _EASYTCPCLIENT_TCP_

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
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR			(-1)
#endif
#include<stdio.h>
#include"MessageHeader.hpp"

#ifndef RECV_BUFFER_SIZE
#define RECV_BUFFER_SIZE 1024*10
#endif

class EasyTcpClient
{
public:
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTcpClient()
	{
		Close();
	}

public:
	// create socket
	int InitSocket()
	{
		if (INVALID_SOCKET != _sock)
		{
			printf("close previous connection<socket=%d>\n", (int)_sock);
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
			return -1;
		}
		//printf("create socket success.\n");
		return 0;
	}

	// connect server
	int Connect(const char * ip, unsigned short port)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}

		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("connect server failure...\n");
		}
		//printf("connect server success...\n");
		return ret;
	}

	// close socket
	void Close()
	{
		if (_sock == INVALID_SOCKET) return;

#ifdef _WIN32
		closesocket(_sock);
		WSACleanup();
#else
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
	}

	//deal net message
	int OnRun()
	{
		if (!IsRunning()) return -1;

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
			Close();
			return ret;
		}

		if (FD_ISSET(_sock, &fdReads))
		{
			FD_CLR(_sock, &fdReads);

			int ret = RecvData();
			if (-1 == ret)
			{
				Close();
				return ret;
			}
		}

		return 0;
	}

	bool IsRunning()
	{
		return INVALID_SOCKET != _sock;
	}

	/*
	 *	1. receive data
	 *	2. process sticking package
	 *	3. receive splitting package
	*/
	int RecvData()
	{
		// receive data
		int nLen = recv(_sock, _szRecvBuffer, RECV_BUFFER_SIZE, 0);
		if (nLen <= 0)
		{
			printf("disconnected to server.\n");
			return -1;
		}

		// copy receive buffer data to message buffer
		memcpy(_szMsgBuffer + _lastPos, _szRecvBuffer, nLen);

		// update end position of message buffer
		_lastPos += nLen;

		// whether message buffer size greater than message header(DataHeader)'s size,
		// if yes, converting message buffer to struct DataHeader and clear message buffer
		// had dealed.
		while (_lastPos >= sizeof(DataHeader))
		{
			DataHeader * pheader = (DataHeader*)_szMsgBuffer;
			if (_lastPos >= pheader->data_length)
			{
				// processed message's length
				int nMsgSize = pheader->data_length;
				// length of message buffer which was untreated
				int nSize = _lastPos - nMsgSize;
				// process net message
				OnNetMessage(pheader);
				// earse processed message buffer
				memcpy(_szMsgBuffer, _szMsgBuffer + nMsgSize, nSize);
				// update end position of message buffer
				_lastPos = nSize;
			}
			else
			{
				// length of message buffer which was untreated less than 
				// length of message header(DataHeader)'s size
				break;
			}
		}

		return 0;
	}

	int SendData(DataHeader * header)
	{
		if (IsRunning() && header)
		{
			return send(_sock, (const char*)header, header->data_length, 0);
		}
		return SOCKET_ERROR;
	}

	void OnNetMessage(DataHeader* header)
	{

		switch (header->cmd)
		{
		case CMD_LOGIN_RESPONSE:
		{
			LoginResponse* ret = (LoginResponse*)header;
			//printf("receive server msg: CMD_LOGIN_RESPONSE, data length: %d, result: %d\n", header->data_length, ret->result);
		}
		break;
		case CMD_LOGOUT_RESPONSE:
		{
			LogoutResponse* ret = (LogoutResponse*)header;
			//printf("receive server msg: CMD_LOGOUT_RESPONSE, data length: %d, result: %d\n", header->data_length, ret->result);
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			NewUserJoin* ret = (NewUserJoin*)header;
			//printf("receive server msg: CMD_NEW_USER_JOIN, data length: %d, result: %d\n", header->data_length, ret->sock);
		}
		break;
		default:
		{
			printf("receive server msg: UNKNOWN, data length: %d\n", header->data_length);
		}
		break;
		}
	}
private:
	SOCKET _sock;
	// receive buffer
	char _szRecvBuffer[RECV_BUFFER_SIZE] = {};
	// message buffer
	char _szMsgBuffer[RECV_BUFFER_SIZE * 10] = {};
	// message buffer position
	int _lastPos = 0;
};

#endif