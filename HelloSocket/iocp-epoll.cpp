//---------
//-- create sample tcp client use socket api
// 1 create socket
// 2 connect server
// 3 recv from server
// 4 close socket
//---------
//-- create sample tcp server use socket api
// 1 create socket
// 2 bind port, which is be connected by client
// 3 listen port
// 4 accept a client's connection
// 5 send message to client
// 6 close socket

#ifdef	_RUN_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <MSWSock.h>
//#pragma comment(lib, "Mswsock.lib")
#include <stdio.h>
#include <stdlib.h>
#include <thread>

int port = 12345;
#define IO_BUFFER_SIZE 1024
// 允许连接进来的客户端数量
const int permit_clients = 10;
BOOL g_run = TRUE;

enum IO_TYPE
{
	ACCEPT = 10,
	RECV,
	SEND
};

typedef struct _PER_IO_CONTEXT {
	OVERLAPPED	_Overlapped;				// 每一个重叠I/O网络操作都要有一个               
	SOCKET		_sockfd;					// 这个I/O操作所使用的Socket，每个连接的都是一样的 
	WSABUF		_wsaBuf;					// 存储数据的缓冲区，用来给重叠操作传递参数的，关于WSABUF后面还会讲 
	char		_szBuffer[IO_BUFFER_SIZE];	// 对应WSABUF里的缓冲区 
	int			_length;					// _szBuffer的实际长度
	IO_TYPE		_OpType;					// 标志这个重叠I/O操作是做什么的，例如Accept/Recv等 
} PER_IO_CONTEXT, *PPER_IO_CONTEXT;

//int delivery_accept(SOCKET sockServer, PPER_IO_CONTEXT pIoData = nullptr)
//{
//	if (!pIoData)
//	{
//		pIoData = new PER_IO_CONTEXT;
//		memset(pIoData, 0, sizeof(PER_IO_CONTEXT));
//	}
//
//	pIoData->_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	pIoData->_OpType = IO_TYPE::ACCEPT;
//
//	BOOL ret = AcceptEx(
//		sockServer,
//		pIoData->_sockfd,				// 用来接受连接的socket（Accept Socket）
//		pIoData->_szBuffer,
//		0,								// 如果不为0，则表示客户端连接到服务端之后，还必须再传送至少
//										// 一个字节大小的数据，服务端才会接受客户端的连接。
//										// 例如：设置此参数为 sizeof(buf) - ((sizeof (sockaddr_in) + 16) * 2)
//		sizeof(sockaddr_in) + 16,		// | 如果第三个参数不是0，这两个参数将从这个参数指定大小的位置开始存储，
//		sizeof(sockaddr_in) + 16,		// | 否则从buf的0位置开始存储
//		NULL,							// 接收到的字节数。不过只在同步（阻塞）模式下，这个参数才有意义，这里可以时0
//		&pIoData->_Overlapped			// 重叠体，供IOCP模式内部使用，不能NULL
//	);
//	if (FALSE == ret)
//	{
//		int err = WSAGetLastError();
//		if (ERROR_IO_PENDING != err)
//		{
//			printf("ERROR occur while AcceptEx. errno=%d\n", GetLastError());
//			return -1;
//		}
//	}
//	return 0;
//}

// -----------------------------------------------------------
// 使用预加载AcceptEx就不需要预先引用 Mswsock.lib 库了。
// -----------------------------------------------------------
LPFN_ACCEPTEX lpfnAcceptEx = NULL;
int load_acceptex(SOCKET sockServer)
{
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes = 0;
	int iResult = WSAIoctl(sockServer, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx, sizeof(GuidAcceptEx),
		&lpfnAcceptEx, sizeof(lpfnAcceptEx),
		&dwBytes, NULL, NULL);
	if (iResult == SOCKET_ERROR) {
		printf("WSAIoctl failed with error: %u\n", WSAGetLastError());
		closesocket(sockServer);
		WSACleanup();
		return -1;
	}
	return 0;
}
int delivery_accept_ex(SOCKET sockServer, PPER_IO_CONTEXT pIoData = nullptr)
{
	if (!pIoData)
	{
		pIoData = new PER_IO_CONTEXT;
		memset(pIoData, 0, sizeof(PER_IO_CONTEXT));
	}

	pIoData->_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	pIoData->_OpType = IO_TYPE::ACCEPT;

	BOOL ret = lpfnAcceptEx(
		sockServer,
		pIoData->_sockfd,				// 用来接受连接的socket（Accept Socket）
		pIoData->_szBuffer,
		0,								// 如果不为0，则表示客户端连接到服务端之后，还必须再传送至少
										// 一个字节大小的数据，服务端才会接受客户端的连接。
										// 例如：设置此参数为 sizeof(buf) - ((sizeof (sockaddr_in) + 16) * 2)
		sizeof(sockaddr_in) + 16,		// | 如果第三个参数不是0，这两个参数将从这个参数指定大小的位置开始存储，
		sizeof(sockaddr_in) + 16,		// | 否则从buf的0位置开始存储
		NULL,							// 接收到的字节数。不过只在同步（阻塞）模式下，这个参数才有意义，这里可以时0
		&pIoData->_Overlapped			// 重叠体，供IOCP模式内部使用，不能NULL
	);
	if (FALSE == ret)
	{
		int err = WSAGetLastError();
		if (ERROR_IO_PENDING != err)
		{
			printf("ERROR occur while AcceptEx. errno=%d\n", GetLastError());
			return -1;
		}
	}
	return 0;
}
// -----------------------------------------------------------

int delivery_receive(PPER_IO_CONTEXT pIoData)
{
	pIoData->_OpType = IO_TYPE::RECV;
	WSABUF wsabuf = {};
	wsabuf.buf = pIoData->_szBuffer;
	wsabuf.len = IO_BUFFER_SIZE;
	DWORD flags = 0;
	ZeroMemory(&pIoData->_Overlapped, sizeof(OVERLAPPED));

	if (SOCKET_ERROR == WSARecv(pIoData->_sockfd, &wsabuf, 1, NULL, &flags, &pIoData->_Overlapped, NULL))
	{
		int err = WSAGetLastError();
		if (ERROR_IO_PENDING != err)
		{
			printf("ERROR occur while WSARecv. errno=%d\n", GetLastError());
			return -1;
		}
	}
	return 0;
}

int delivery_send(PPER_IO_CONTEXT pIoData)
{
	pIoData->_OpType = IO_TYPE::SEND;
	WSABUF wsabuf = {};
	wsabuf.buf = pIoData->_szBuffer;
	wsabuf.len = pIoData->_length;
	DWORD flags = 0;
	if (SOCKET_ERROR == WSASend(pIoData->_sockfd, &wsabuf, 1, NULL, flags, &pIoData->_Overlapped, NULL))
	{
		int err = WSAGetLastError();
		if (ERROR_IO_PENDING != err)
		{
			printf("ERROR occur while WSASend. errno=%d\n", GetLastError());
			return -1;
		}
	}
	return 0;
}

void cmdThread()
{
	while (true)
	{
		char cmd_buf[128] = {};
		scanf("%s", cmd_buf);
		if (0 == strcmp(cmd_buf, "exit"))
		{
			printf("bye\n");
			g_run = false;
			break;
		}
		else
		{
			printf("this command was not supported.\n");
		}
	}
}

// -- IOCP基础流程
int main()
{
	// 启动命令线程
	std::thread t1(cmdThread);
	t1.detach();

	// 启动Sock 2.X环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);

	// -------------//
	// 1. 建立一个Socket
	// 当使用Socket API创建套接字的时候，会默认设置WSA_FLAG_OVERLAPPED标志
	// 我们也可以使用WSASocket函数创建SOCKET
	SOCKET sock_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 2.1 设置对外IP和端口信息
	sockaddr_in sin = {};
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = INADDR_ANY;
	// 2.2 绑定sockaddr与sock_server
	if (SOCKET_ERROR == bind(sock_server, (sockaddr*)&sin, sizeof(sockaddr)))
	{
		printf("ERROR occur while binding local port<%d>\n", port);
		return -1;
	}

	printf("SUCCESS for binding local port<%d>\n", port);

	// 3. 监听sock_server
	if (SOCKET_ERROR == listen(sock_server, 64))
	{
		printf("ERROR occur while listening local port<%d>\n", port);
		return -1;
	}

	printf("listening port<%d>\n", port);

	// ----- IOCP BEGIN -----//

	// 4. 创建完成端口IOCP
	HANDLE iocp_obj = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE,
		NULL,
		0,
		0
	);
	if (NULL == iocp_obj)
	{
		printf("ERROR occur while create io completion port. errno=%d\n", GetLastError());
		return -1;
	}

	// 5. 关联IOCP与SERVER SOCKET
	HANDLE iocp_socket_relate_result = CreateIoCompletionPort(
		(HANDLE)sock_server,
		iocp_obj,
		(ULONG_PTR)sock_server,	// 可以是自定义结构体、对象、数组、等的指针，
								// 也可以是一个基础类型，这里简单传一个socket值
		0
	);
	if (NULL == iocp_socket_relate_result)
	{
		printf("ERROR occur while server socket is associated with io completion port. errno=%d\n", GetLastError());
		return -1;
	}

	// 6. 向IOCP投递接受客户端连接的任务
	// 预加载AcceptEx
	if (load_acceptex(sock_server) < 0)
	{
		return -1;
	}
	PER_IO_CONTEXT ioData[permit_clients] = {};
	for (int i = 0; i < permit_clients; i++)
	{
		//if (delivery_accept(sock_server, ioData+i) < 0) {
		//	return -1;
		//}
		if (delivery_accept_ex(sock_server, ioData + i) < 0) {
			return -1;
		}
	}

	int msg_count = 0;
	while (g_run)
	{
		// 获取完成端口状态
		DWORD bytes_trans = 0;
		SOCKET sock = INVALID_SOCKET;
		PPER_IO_CONTEXT pIoData;
		BOOL ret = GetQueuedCompletionStatus(
			iocp_obj,
			&bytes_trans,
			(PULONG_PTR)&sock,
			(LPOVERLAPPED*)&pIoData,
			1
		);
		// 检查是否有事件发生，和select，epoll_wait类似
		if (!ret)
		{
			int err = GetLastError();
			if (WAIT_TIMEOUT == err)
			{
				continue;
			}
			if (ERROR_NETNAME_DELETED == err)
			{
				printf("1. close client. socket=%d\n", pIoData->_sockfd);
				closesocket(pIoData->_sockfd);
				continue;
			}
			printf("ERROR occur while GetQueuedCompletionStatus. errno=%d\n", GetLastError());
			break;
		}
		// 7.1 接受连接已经完成
		if (IO_TYPE::ACCEPT == pIoData->_OpType)
		{
			printf("new client enter. socket=%d\n", pIoData->_sockfd);
			// 7.2 关联IOCP与Client Socket
			HANDLE iocp_socket_relate_result = CreateIoCompletionPort(
				(HANDLE)pIoData->_sockfd,
				iocp_obj,
				(ULONG_PTR)pIoData->_sockfd,
				0
			);
			if (NULL == iocp_socket_relate_result)
			{
				printf("ERROR occur while client socket is associated with io completion port. errno=%d\n", GetLastError());
				closesocket(pIoData->_sockfd);
				// 关闭一个，再投递一个ACCEPT，等待新的客户连接，保持可以连接的总数不变
				delivery_accept_ex(sock_server, pIoData);
				continue;
			}
			// 7.3 向IOCP投递接收数据的任务
			if (delivery_receive(pIoData) < 0)
			{
				printf("CLOSE socket=%d\n", pIoData->_sockfd);
				closesocket(pIoData->_sockfd);
				// 关闭一个，再投递一个ACCEPT，等待新的客户连接，保持可以连接的总数不变
				delivery_accept_ex(sock_server, pIoData);
			}
			continue;
		}
		// 8.1 接收数据已经完成 Completion
		if (IO_TYPE::RECV == pIoData->_OpType)
		{
			// 客户端断开处理
			if (bytes_trans <= 0)
			{
				printf("CLOSE socket=%d,bytes_trans=%d\n", pIoData->_sockfd, bytes_trans);
				closesocket(pIoData->_sockfd);
				// 关闭一个，复用一次PER_IO_CONTEXT，再投递一个ACCEPT，等待新的客户连接，保持可以连接的总数不变
				delivery_accept_ex(sock_server, pIoData);
				continue;
			}

			printf("RECEIVE data socket=%d,bytes_trans=%d, msg_count=%d\n", pIoData->_sockfd, bytes_trans, ++msg_count);

			//// 8.2 如果还需要接收数据，需要再投递接收数据任务
			//if (delivery_receive(pIoData) < 0)
			//{
			//	printf("CLOSE socket=%d\n", pIoData->_sockfd);
			//	closesocket(pIoData->_sockfd);
			//}
			// 8.2 向IOCP投递发送数据任务
			pIoData->_length = bytes_trans;
			if (delivery_send(pIoData) < 0)
			{
				printf("CLOSE socket=%d\n", pIoData->_sockfd);
				closesocket(pIoData->_sockfd);
			}
			continue;
		}
		// 9.1 发送数据已经完成
		if (IO_TYPE::SEND == pIoData->_OpType)
		{
			// 客户端断开处理
			if (bytes_trans <= 0)
			{
				printf("CLOSE socket=%d,bytes_trans=%d\n", pIoData->_sockfd, bytes_trans);
				closesocket(pIoData->_sockfd);
				// 关闭一个，复用一次PER_IO_CONTEXT，再投递一个ACCEPT，等待新的客户连接，保持可以连接的总数不变
				delivery_accept_ex(sock_server, pIoData);
				continue;
			}

			printf("SEND data socket=%d,bytes_trans=%d, msg_count=%d\n", pIoData->_sockfd, bytes_trans, msg_count);

			//// 9.2 如果还需要发送数据，向IOCP投递发送数据任务
			//pIoData->_length = bytes_trans;
			//if (delivery_send(pIoData) < 0)
			//{
			//	printf("CLOSE socket=%d\n", pIoData->_sockfd);
			//	closesocket(pIoData->_sockfd);
			//}

			// 9.2 向IOCP投递接收数据任务
			if (delivery_receive(pIoData) < 0)
			{
				printf("CLOSE socket=%d\n", pIoData->_sockfd);
				closesocket(pIoData->_sockfd);
				// 关闭一个，复用一次PER_IO_CONTEXT，再投递一个ACCEPT，等待新的客户连接，保持可以连接的总数不变
				delivery_accept_ex(sock_server, pIoData);
			}
			continue;
		}

		printf("undefine action.\n");
		break;
	
	}

	// ----- IOCP END -----//

	// 10.1 关闭Client Socket
	for (PER_IO_CONTEXT client : ioData)
	{
		closesocket(client._sockfd);
	}
	// 10.2 关闭Server Socket
	closesocket(sock_server);
	// 10.3 关闭完成端口
	CloseHandle(iocp_obj);

	// -------------------//
	// 清除Windows Socket环境
	WSACleanup();
	system("PAUSE");
	return 0;
}
#elif __linux__
// linux compile command
// g++ server.cpp -std=c++11 -pthread -o server
// doc www.man7.org/linux linux.die.net
// search key "man epoll" in google or enter "man epoll" in linux terminal
// ----------------------------------
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <sys/epoll.h>

#include "crc_init.h"
#include "../common/include/crc_logger.hpp"

#define SOCKET int
#define INVALID_SOCKET	(SOCKET)(~0)
#define SOCKET_ERROR			(-1)

#include <stdio.h>
#include <vector>
#include <thread>
#include <algorithm>

bool g_run = true;
int g_count = 0;
int g_flag = 0;
std::vector<SOCKET> g_clients;

void cmdThread()
{
	while (true)
	{
		char cmd_buf[128] = {};
		scanf("%s", cmd_buf);
		if (0 == strcmp(cmd_buf, "exit"))
		{
			printf("bye\n");
			g_run = false;
			break;
		}
		else
		{
			printf("this command was not supported.\n");
		}
	}
}

int recvThread(SOCKET clientSocket)
{
	char szRecv[4096];
	return recv(clientSocket, szRecv, 4096, 0);
}

int sendThread(SOCKET clientSocket)
{
	HeartS2C heart;
	return send(clientSocket, (const char*)&heart, heart.data_length, 0);
}

int clientLeave(SOCKET clientSocket)
{
	close(clientSocket);
	CRCLogger::info("client <socket=%d> had exit\n", clientSocket);
	auto iter = std::find(g_clients.begin(), g_clients.end(), clientSocket);
	if (g_clients.end() != iter)
		g_clients.erase(iter);
}

int cell_epoll_ctl(int epfd, int operation, SOCKET socket, uint32_t events)
{
	// 定义服务端的socket并将其关联到EPOLLIN（可读）事件
	epoll_event event;
	//事件类型(比如可读-EPOLLIN)
	event.events = events;
	//此事件类型所关联的描述符对象
	event.data.fd = socket;
	// epoll controll
	// 向epoll对象注册需要管理、监听、检测的socket
	// 并且说明关心的事件（events）
	int ret = epoll_ctl(epfd, operation, socket, &event);
	if (ret < 0)
	{
		CRCLogger::info("error epoll_ctl(%d,%d,%d,%d)\n", epfd, operation, socket, events);
	}
	return ret;
}

int main()
{
	// 启动命令线程
	std::thread t1(cmdThread);
	t1.detach();

	signal(SIGPIPE, SIG_IGN);

	// 启动日志
	CRCLogger::instance().set_log_path("server.log", "w");
	CRCLogger::instance().start();

	// 计时器
	CRCTimestamp tTime;

	SOCKET _sock;
	/**
	* create socket
	*/
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (SOCKET_ERROR == _sock)
	{
		CRCLogger::info("create socket failure.\n");
		return _sock;
	}
	CRCLogger::info("socket<%d> create success.\n", _sock);

	/**
	* bind ip and port
	*/
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(12345);
	_sin.sin_addr.s_addr = INADDR_ANY;

	int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (ret == SOCKET_ERROR)
	{
		CRCLogger::info("socket<%d> bind failure because port used by other.\n", _sock);
		return ret;
	}
	CRCLogger::info("socket<%d> bind port<%d> success.\n", _sock, 12345);

	/**
	* listen port
	*/
	ret = listen(_sock, 5);
	if (SOCKET_ERROR == ret)
	{
		CRCLogger::info("socket<%d> listen failure.\n", _sock);
		return ret;
	}
	CRCLogger::info("socket<%d> listenning ... \n", _sock);

	/**
	* accept loop and process
	* epoll_create 的参数在linux内核2.6.8之后已经无意义
	* 由epoll动态管理，其值为最大打开文件数file-maxexit
	*
	*
	*
	*/
	// create epoll object       
	int epfd = epoll_create(1024/*已经没有意义，只是为了兼容更早的写法*/);
	// 注册服务端socket到epoll控制器，监听其上的可读（ EPOLLIN ）即客户端接入事件
	// ------------------------------------------------------
	ret = cell_epoll_ctl(epfd, EPOLL_CTL_ADD, _sock, EPOLLIN);
	if (ret < 0)
	{
		CRCLogger::info("error epoll_ctl %d \n", ret);
		return ret;
	}
	// ------------------------------------------------------
	// 创建一个空的 epoll event 数组，用来接收检测到的事件及其关联的socket
	epoll_event events[1024] = {};
	// 循环等待所有socket上的所有事件
	while (g_run)
	{
		// 等待网络事件的到来，或者立即返回
		int ret_events = epoll_wait(
			epfd,       // 由 epoll_create 建立的对象
			events,     // 保存在 _sock 和 events 上检测到的事件
			1024,       // 接收数组 events 的大小，不能超过，否则会数组越界
			1           // 等待超时。
						// >0   ：如果没有事件，等待指定毫秒数后返回
						// 0    ：有事件，就把事件装入 events ，没有就立即返回。
						// -1   ：一直等待，直到有事件发生。
		);
		// 检查返回的事件数
		// < 0，表示出错
		// = 0，表示没有事件发生
		if (ret_events < 0)
		{
			CRCLogger::info("error epoll_wait %d \n", ret_events);
			break;
		}

		// 遍历返回的事件
		// 如果 ret_events = 0，则进不了这个for循环
		for (int n = 0; n < ret_events; n++)
		{
			// 判断事件的socket是否是服务端的socket
			// 如果服务端的socket可读，表示有新的客户端连接进来了
			// 只有_sock是服务端socket，其他的都是客户端socket
			// ------------------------------------------
			if (events[n].data.fd == _sock)
			{
				sockaddr_in client_addr = {};
				int client_addr_len = sizeof(sockaddr_in);	// note: accept will fall without sizeof(sockaddr_in)
				SOCKET sock_client = INVALID_SOCKET;
				sock_client = accept(_sock, (sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
				if (INVALID_SOCKET == sock_client)
				{
					CRCLogger::info("error socket<%d> accept a invalid client socket.\n", _sock);
				}
				else
				{
					// 注册新加入的客户端socket到epoll控制器
					// -------------------------------------------------
					// 不要在一开始就注册 EPOLLOUT 事件，因为在LT模式下会频繁的收到 EPOLLOUT
					// 导致客户端断开了，开在不停的发送数据。在某个socket需要发送数据的时候，再去
					// 为这个socket注册 EPOLLOUT 事件
					// warning: cell_epoll_ctl(epfd, EPOLL_CTL_ADD, sock_client, EPOLLIN | EPOLLOUT);
					ret = cell_epoll_ctl(epfd, EPOLL_CTL_ADD, sock_client, EPOLLIN);
					if (ret < 0)
					{
						break;
					}
					// -------------------------------------------------
					// 缓存客户socket
					g_clients.push_back(sock_client);
					CRCLogger::info("socket<%d> accept a client ip<%s>.\n", _sock, inet_ntoa(client_addr.sin_addr));
				}
				continue;
			}
			// 处理客户端socket事件
			// -----------------------------------------
			// 处理客户端可读事件，表示客户端socket有数据可读
			if (events[n].events & EPOLLIN)
			{
				CRCLogger::info("EPOLLIN %d\n", ++g_count);
				SOCKET clientSocket = events[n].data.fd;
				int ret = recvThread(clientSocket);
				if (ret <= 0)
				{
					clientLeave(clientSocket);
				}
				else
				{
					// 收到消息后改为在此socket上关注写事件
					cell_epoll_ctl(epfd, EPOLL_CTL_MOD, clientSocket, EPOLLOUT);
					CRCLogger::info("server socket<%d> receive client<socket=%d> data, len=%d.\n", _sock, clientSocket, ret);
				}
			}
			// 处理客户端可写事件，表示客户端socket现在可以发送数据（没有因各种原因导致的堵塞）
			if (events[n].events & EPOLLOUT)
			{
				CRCLogger::info("EPOLLOUT %d\n", g_count);
				SOCKET clientSocket = events[n].data.fd;
				int ret = sendThread(clientSocket);
				if (ret <= 0)
				{
					clientLeave(clientSocket);
				}
				else
				{
					// 发送完成后改为在此socket上关注读事件
					cell_epoll_ctl(epfd, EPOLL_CTL_MOD, clientSocket, EPOLLIN);
					// 演示 EPOLL_CTL_DEL 的使用
					// 作用：不关闭连接的情况下，关闭对摸个socket的监听，需要的时候再用 
					// EPOLL_CTL_ADD 加进来
					if (g_count > 50)
					{
						cell_epoll_ctl(epfd, EPOLL_CTL_DEL, clientSocket, 0);
					}
					CRCLogger::info("server socket<%d> send client<socket=%d> data len=%d\n", _sock, clientSocket, ret);
				}
			}
			// 已经在EPOLLIN检测了异常，所以下面的两种异常无需检测，浪费时间
			/*
			if (events[n].events & EPOLLERR)
			{
			CRCLogger::info("error  EPOLLERR client<socket=%d>\n",events[n].data.fd);
			}

			if (events[n].events & EPOLLHUP)
			{
			CRCLogger::info("error  EPOLLHUP client<socket=%d>\n",events[n].data.fd);
			}
			*/
			// -----------------------------------------
		}
	}

	for (auto client : g_clients)
	{
		close(client);
	}
	g_clients.clear();

	close(epfd);
	close(_sock);
	CRCLogger::info("server had shut down.\n");
	return 0;
}
#endif

#endif