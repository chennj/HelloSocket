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
	// ���������߳�
	std::thread t1(cmdThread);
	t1.detach();

	signal(SIGPIPE, SIG_IGN);

	// ������־
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
	* epoll_create �Ĳ�����linux�ں�2.6.8֮���Ѿ�������
	* ��epoll��̬������ֵΪ�����ļ���file-max
	*/
	// create epoll object       
	int epfd = epoll_create(1024/*�Ѿ�û�����壬ֻ��Ϊ�˼��ݸ����д��*/);
	// �������˵�socket�����������EPOLLIN���ɶ����¼�
	epoll_event event;
	//�¼�����(�ɶ�)
	event.events = EPOLLIN;
	//���¼�����������������������
	event.data.fd = _sock;
	// epoll controll
	// ��epoll����ע����Ҫ��������������socket
	// ����˵�����ĵ��¼���EPOLLIN��
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, _sock, &event);
	if (ret < 0)
	{
		CRCLogger::info("error epoll_ctl %d \n", ret);
		return ret;
	}
	// ����һ���յ� epoll event ���飬���������� _sock �ϼ�⵽���¼�
	epoll_event events[1024] = {};
	// ѭ���ȴ����ǹ��ĵ��¼������� _sock �� events �ϵ�
	while (g_run)
	{
		// �ȴ������¼��ĵ�����������������
		int ret_events = epoll_wait(
			epfd,       // �� epoll_create �����Ķ���
			events,     // ������ _sock �ϼ�⵽���¼�
			1024,       // �������� events �Ĵ�С�����ܳ��������������Խ��
			0           // �ȴ���ʱ��
						// >0   �����û���¼����ȴ�ָ���������󷵻�
						// 0    �����¼����Ͱ��¼�װ�� events ��û�о��������ء�
						// -1   ��һֱ�ȴ���ֱ�����¼�������
		);
		// ��鷵�ص��¼���
		// < 0����ʾ����
		// = 0����ʾû���¼�����
		if (ret_events < 0)
		{
			CRCLogger::info("error epoll_wait %d \n", ret);
			break;
		}
		// �������ص��¼�
		// ��� ret_events = 0������������forѭ��
		for (int n = 0; n < ret_events; n++)
		{
			// �ж��¼���socket�Ƿ��Ƿ���˵�socket
			// �������˵�socket�ɶ�����ʾ���µĿͻ������ӽ�����
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