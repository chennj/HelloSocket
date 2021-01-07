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
	// �������˵�socket�����������EPOLLIN���ɶ����¼�
	epoll_event event;
	//�¼�����(����ɶ�-EPOLLIN)
	event.events = events;
	//���¼�����������������������
	event.data.fd = socket;
	// epoll controll
	// ��epoll����ע����Ҫ��������������socket
	// ����˵�����ĵ��¼���events��
	int ret = epoll_ctl(epfd, operation, socket, &event);
	if (ret < 0)
	{
		CRCLogger::info("error epoll_ctl(%d,%d,%d,%d)\n", epfd, operation, socket, events);
	}
	return ret;
}

int main()
{
	// ���������߳�
	std::thread t1(cmdThread);
	t1.detach();

	signal(SIGPIPE, SIG_IGN);

	// ������־
	CRCLogger::instance().set_log_path("server.log", "w");
	CRCLogger::instance().start();

	// ��ʱ��
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
	* epoll_create �Ĳ�����linux�ں�2.6.8֮���Ѿ�������
	* ��epoll��̬������ֵΪ�����ļ���file-maxexit
	*
	*
	*
	*/
	// create epoll object       
	int epfd = epoll_create(1024/*�Ѿ�û�����壬ֻ��Ϊ�˼��ݸ����д��*/);
	// ע������socket��epoll���������������ϵĿɶ��� EPOLLIN �����ͻ��˽����¼�
	// ------------------------------------------------------
	ret = cell_epoll_ctl(epfd, EPOLL_CTL_ADD, _sock, EPOLLIN);
	if (ret < 0)
	{
		CRCLogger::info("error epoll_ctl %d \n", ret);
		return ret;
	}
	// ------------------------------------------------------
	// ����һ���յ� epoll event ���飬�������ռ�⵽���¼����������socket
	epoll_event events[1024] = {};
	// ѭ���ȴ�����socket�ϵ������¼�
	while (g_run)
	{
		// �ȴ������¼��ĵ�����������������
		int ret_events = epoll_wait(
			epfd,       // �� epoll_create �����Ķ���
			events,     // ������ _sock �� events �ϼ�⵽���¼�
			1024,       // �������� events �Ĵ�С�����ܳ��������������Խ��
			1           // �ȴ���ʱ��
						// >0   �����û���¼����ȴ�ָ���������󷵻�
						// 0    �����¼����Ͱ��¼�װ�� events ��û�о��������ء�
						// -1   ��һֱ�ȴ���ֱ�����¼�������
		);
		// ��鷵�ص��¼���
		// < 0����ʾ����
		// = 0����ʾû���¼�����
		if (ret_events < 0)
		{
			CRCLogger::info("error epoll_wait %d \n", ret_events);
			break;
		}

		// �������ص��¼�
		// ��� ret_events = 0������������forѭ��
		for (int n = 0; n < ret_events; n++)
		{
			// �ж��¼���socket�Ƿ��Ƿ���˵�socket
			// �������˵�socket�ɶ�����ʾ���µĿͻ������ӽ�����
			// ֻ��_sock�Ƿ����socket�������Ķ��ǿͻ���socket
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
					// ע���¼���Ŀͻ���socket��epoll������
					// -------------------------------------------------
					// ��Ҫ��һ��ʼ��ע�� EPOLLOUT �¼�����Ϊ��LTģʽ�»�Ƶ�����յ� EPOLLOUT
					// ���¿ͻ��˶Ͽ��ˣ����ڲ�ͣ�ķ������ݡ���ĳ��socket��Ҫ�������ݵ�ʱ����ȥ
					// Ϊ���socketע�� EPOLLOUT �¼�
					// warning: cell_epoll_ctl(epfd, EPOLL_CTL_ADD, sock_client, EPOLLIN | EPOLLOUT);
					ret = cell_epoll_ctl(epfd, EPOLL_CTL_ADD, sock_client, EPOLLIN);
					if (ret < 0)
					{
						break;
					}
					// -------------------------------------------------
					// ����ͻ�socket
					g_clients.push_back(sock_client);
					CRCLogger::info("socket<%d> accept a client ip<%s>.\n", _sock, inet_ntoa(client_addr.sin_addr));
				}
				continue;
			}
			// ����ͻ���socket�¼�
			// -----------------------------------------
			// ����ͻ��˿ɶ��¼�����ʾ�ͻ���socket�����ݿɶ�
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
					// �յ���Ϣ���Ϊ�ڴ�socket�Ϲ�עд�¼�
					cell_epoll_ctl(epfd, EPOLL_CTL_MOD, clientSocket, EPOLLOUT);
					CRCLogger::info("server socket<%d> receive client<socket=%d> data, len=%d.\n", _sock, clientSocket, ret);
				}
			}
			// ����ͻ��˿�д�¼�����ʾ�ͻ���socket���ڿ��Է������ݣ�û�������ԭ���µĶ�����
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
					// ������ɺ��Ϊ�ڴ�socket�Ϲ�ע���¼�
					cell_epoll_ctl(epfd, EPOLL_CTL_MOD, clientSocket, EPOLLIN);
					// ��ʾ EPOLL_CTL_DEL ��ʹ��
					// ���ã����ر����ӵ�����£��رն�����socket�ļ�������Ҫ��ʱ������ 
					// EPOLL_CTL_ADD �ӽ���
					if (g_count > 50)
					{
						cell_epoll_ctl(epfd, EPOLL_CTL_DEL, clientSocket, 0);
					}
					CRCLogger::info("server socket<%d> send client<socket=%d> data len=%d\n", _sock, clientSocket, ret);
				}
			}
			// �Ѿ���EPOLLIN������쳣����������������쳣�����⣬�˷�ʱ��
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