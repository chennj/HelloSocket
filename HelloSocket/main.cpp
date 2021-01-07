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

#ifdef _WIN32
int main()
{
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