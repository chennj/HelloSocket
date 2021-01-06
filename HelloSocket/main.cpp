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
#define WIN32_LEAN_AND_MEAN	//try import small def loog ago
#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>
#include<stdlib.h>
int main()
{
	return 0;
}
#elif __linux__
// linux compile command
// g++ server.cpp -std=c++11 -pthread -o server
// ----------------------------------
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include <string.h>
#include <signal.h>
#include <sys/epoll.h>

#include "../common/include/crc_logger.hpp"

#define SOCKET int
#define INVALID_SOCKET	(SOCKET)(~0)
#define SOCKET_ERROR			(-1)

#include <stdio.h>
#include <vector>
#include <thread>

bool g_run = true;
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

int main()
{
	// 启动命令线程
	std::thread t1(cmdThread);
	t1.detach();

	signal(SIGPIPE, SIG_IGN);

	// 启动日志
	CRCLogger::instance().set_log_path("server.log", "w");
	CRCLogger::instance().start();

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
	CRCLogger::info("socket<%d> create success.\n", (int)_sock);

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
		CRCLogger::info("socket<%d> bind failure because port used by other.\n", (int)_sock);
		return ret;
	}
	CRCLogger::info("socket<%d> bind port<%d> success.\n", (int)_sock, 12345);

	/**
	* listen port
	*/
	ret = listen(_sock, 5);
	if (SOCKET_ERROR == ret)
	{
		CRCLogger::info("socket<%d> listen failure.\n", (int)_sock);
		return ret;
	}
	CRCLogger::info("socket<%d> listenning ... \n", (int)_sock);

	/**
	* accept loop and process
	* epoll_create 的参数在linux内核2.6.8之后已经无意义
	* 由epoll动态管理，其值为最大打开文件数file-max
	*/
	// create epoll object       
	int epfd = epoll_create(1024/*已经没有意义，只是为了兼容更早的写法*/);
	// 定义服务端的socket并将其关联到EPOLLIN（可读）事件
	epoll_event event;
	//事件类型(可读)
	event.events = EPOLLIN;
	//此事件类型所关联的描述符对象
	event.data.fd = _sock;
	// epoll controll
	// 向epoll对象注册需要管理、监听、检测的socket
	// 并且说明关心的事件（EPOLLIN）
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, _sock, &event);
	if (ret < 0)
	{
		CRCLogger::info("error epoll_ctl %d \n", ret);
		return ret;
	}
	// 创建一个空的 epoll event 数组，用来接收在 _sock 上检测到的事件
	epoll_event events[1024] = {};
	// 循环等待我们关心的事件，包括 _sock 和 events 上的
	while (g_run)
	{
		// 等待网络事件的到来，或者立即返回
		int ret_events = epoll_wait(
			epfd,       // 由 epoll_create 建立的对象
			events,     // 保存在 _sock 上检测到的事件
			1024,       // 接收数组 events 的大小，不能超过，否则会数组越界
			0           // 等待超时。
						// >0   ：如果没有事件，等待指定毫秒数后返回
						// 0    ：有事件，就把事件装入 events ，没有就立即返回。
						// -1   ：一直等待，直到有事件发生。
		);
		// 检查返回的事件数
		// < 0，表示出错
		// = 0，表示没有事件发生
		if (ret_events < 0)
		{
			CRCLogger::info("error epoll_wait %d \n", ret);
			break;
		}
		// 遍历返回的事件
		// 如果 ret_events = 0，则进不了这个for循环
		for (int n = 0; n < ret_events; n++)
		{
			// 判断事件的socket是否是服务端的socket
			// 如果服务端的socket可读，表示有新的客户端连接进来了
			if (events[n].data.fd == _sock)
			{
				sockaddr_in client_addr = {};
				int client_addr_len = sizeof(sockaddr_in);	// note: accept will fall without sizeof(sockaddr_in)
				SOCKET sock_client = INVALID_SOCKET;
				sock_client = accept(_sock, (sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
				if (INVALID_SOCKET == sock_client)
				{
					CRCLogger::info("error socket<%d> accept a invalid client socket.\n", (int)_sock);
				}
				else
				{
					g_clients.push_back(sock_client);
					CRCLogger::info("socket<%d> accept a client ip<%s>.\n", (int)_sock, inet_ntoa(client_addr.sin_addr));
				}
			}
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