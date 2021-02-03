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
// �������ӽ����Ŀͻ�������
const int permit_clients = 10;
BOOL g_run = TRUE;

enum IO_TYPE
{
	ACCEPT = 10,
	RECV,
	SEND
};

typedef struct _PER_IO_CONTEXT {
	OVERLAPPED	_Overlapped;				// ÿһ���ص�I/O���������Ҫ��һ��               
	SOCKET		_sockfd;					// ���I/O������ʹ�õ�Socket��ÿ�����ӵĶ���һ���� 
	WSABUF		_wsaBuf;					// �洢���ݵĻ��������������ص��������ݲ����ģ�����WSABUF���滹�ὲ 
	char		_szBuffer[IO_BUFFER_SIZE];	// ��ӦWSABUF��Ļ����� 
	int			_length;					// _szBuffer��ʵ�ʳ���
	IO_TYPE		_OpType;					// ��־����ص�I/O��������ʲô�ģ�����Accept/Recv�� 
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
//		pIoData->_sockfd,				// �����������ӵ�socket��Accept Socket��
//		pIoData->_szBuffer,
//		0,								// �����Ϊ0�����ʾ�ͻ������ӵ������֮�󣬻������ٴ�������
//										// һ���ֽڴ�С�����ݣ�����˲Ż���ܿͻ��˵����ӡ�
//										// ���磺���ô˲���Ϊ sizeof(buf) - ((sizeof (sockaddr_in) + 16) * 2)
//		sizeof(sockaddr_in) + 16,		// | �����������������0�����������������������ָ����С��λ�ÿ�ʼ�洢��
//		sizeof(sockaddr_in) + 16,		// | �����buf��0λ�ÿ�ʼ�洢
//		NULL,							// ���յ����ֽ���������ֻ��ͬ����������ģʽ�£���������������壬�������ʱ0
//		&pIoData->_Overlapped			// �ص��壬��IOCPģʽ�ڲ�ʹ�ã�����NULL
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
// ʹ��Ԥ����AcceptEx�Ͳ���ҪԤ������ Mswsock.lib ���ˡ�
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
		pIoData->_sockfd,				// �����������ӵ�socket��Accept Socket��
		pIoData->_szBuffer,
		0,								// �����Ϊ0�����ʾ�ͻ������ӵ������֮�󣬻������ٴ�������
										// һ���ֽڴ�С�����ݣ�����˲Ż���ܿͻ��˵����ӡ�
										// ���磺���ô˲���Ϊ sizeof(buf) - ((sizeof (sockaddr_in) + 16) * 2)
		sizeof(sockaddr_in) + 16,		// | �����������������0�����������������������ָ����С��λ�ÿ�ʼ�洢��
		sizeof(sockaddr_in) + 16,		// | �����buf��0λ�ÿ�ʼ�洢
		NULL,							// ���յ����ֽ���������ֻ��ͬ����������ģʽ�£���������������壬�������ʱ0
		&pIoData->_Overlapped			// �ص��壬��IOCPģʽ�ڲ�ʹ�ã�����NULL
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

// -- IOCP��������
int main()
{
	// ���������߳�
	std::thread t1(cmdThread);
	t1.detach();

	// ����Sock 2.X����
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);

	// -------------//
	// 1. ����һ��Socket
	// ��ʹ��Socket API�����׽��ֵ�ʱ�򣬻�Ĭ������WSA_FLAG_OVERLAPPED��־
	// ����Ҳ����ʹ��WSASocket��������SOCKET
	SOCKET sock_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 2.1 ���ö���IP�Ͷ˿���Ϣ
	sockaddr_in sin = {};
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = INADDR_ANY;
	// 2.2 ��sockaddr��sock_server
	if (SOCKET_ERROR == bind(sock_server, (sockaddr*)&sin, sizeof(sockaddr)))
	{
		printf("ERROR occur while binding local port<%d>\n", port);
		return -1;
	}

	printf("SUCCESS for binding local port<%d>\n", port);

	// 3. ����sock_server
	if (SOCKET_ERROR == listen(sock_server, 64))
	{
		printf("ERROR occur while listening local port<%d>\n", port);
		return -1;
	}

	printf("listening port<%d>\n", port);

	// ----- IOCP BEGIN -----//

	// 4. ������ɶ˿�IOCP
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

	// 5. ����IOCP��SERVER SOCKET
	HANDLE iocp_socket_relate_result = CreateIoCompletionPort(
		(HANDLE)sock_server,
		iocp_obj,
		(ULONG_PTR)sock_server,	// �������Զ���ṹ�塢�������顢�ȵ�ָ�룬
								// Ҳ������һ���������ͣ�����򵥴�һ��socketֵ
		0
	);
	if (NULL == iocp_socket_relate_result)
	{
		printf("ERROR occur while server socket is associated with io completion port. errno=%d\n", GetLastError());
		return -1;
	}

	// 6. ��IOCPͶ�ݽ��ܿͻ������ӵ�����
	// Ԥ����AcceptEx
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
		// ��ȡ��ɶ˿�״̬
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
		// ����Ƿ����¼���������select��epoll_wait����
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
		// 7.1 ���������Ѿ����
		if (IO_TYPE::ACCEPT == pIoData->_OpType)
		{
			printf("new client enter. socket=%d\n", pIoData->_sockfd);
			// 7.2 ����IOCP��Client Socket
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
				// �ر�һ������Ͷ��һ��ACCEPT���ȴ��µĿͻ����ӣ����ֿ������ӵ���������
				delivery_accept_ex(sock_server, pIoData);
				continue;
			}
			// 7.3 ��IOCPͶ�ݽ������ݵ�����
			if (delivery_receive(pIoData) < 0)
			{
				printf("CLOSE socket=%d\n", pIoData->_sockfd);
				closesocket(pIoData->_sockfd);
				// �ر�һ������Ͷ��һ��ACCEPT���ȴ��µĿͻ����ӣ����ֿ������ӵ���������
				delivery_accept_ex(sock_server, pIoData);
			}
			continue;
		}
		// 8.1 ���������Ѿ���� Completion
		if (IO_TYPE::RECV == pIoData->_OpType)
		{
			// �ͻ��˶Ͽ�����
			if (bytes_trans <= 0)
			{
				printf("CLOSE socket=%d,bytes_trans=%d\n", pIoData->_sockfd, bytes_trans);
				closesocket(pIoData->_sockfd);
				// �ر�һ��������һ��PER_IO_CONTEXT����Ͷ��һ��ACCEPT���ȴ��µĿͻ����ӣ����ֿ������ӵ���������
				delivery_accept_ex(sock_server, pIoData);
				continue;
			}

			printf("RECEIVE data socket=%d,bytes_trans=%d, msg_count=%d\n", pIoData->_sockfd, bytes_trans, ++msg_count);

			//// 8.2 �������Ҫ�������ݣ���Ҫ��Ͷ�ݽ�����������
			//if (delivery_receive(pIoData) < 0)
			//{
			//	printf("CLOSE socket=%d\n", pIoData->_sockfd);
			//	closesocket(pIoData->_sockfd);
			//}
			// 8.2 ��IOCPͶ�ݷ�����������
			pIoData->_length = bytes_trans;
			if (delivery_send(pIoData) < 0)
			{
				printf("CLOSE socket=%d\n", pIoData->_sockfd);
				closesocket(pIoData->_sockfd);
			}
			continue;
		}
		// 9.1 ���������Ѿ����
		if (IO_TYPE::SEND == pIoData->_OpType)
		{
			// �ͻ��˶Ͽ�����
			if (bytes_trans <= 0)
			{
				printf("CLOSE socket=%d,bytes_trans=%d\n", pIoData->_sockfd, bytes_trans);
				closesocket(pIoData->_sockfd);
				// �ر�һ��������һ��PER_IO_CONTEXT����Ͷ��һ��ACCEPT���ȴ��µĿͻ����ӣ����ֿ������ӵ���������
				delivery_accept_ex(sock_server, pIoData);
				continue;
			}

			printf("SEND data socket=%d,bytes_trans=%d, msg_count=%d\n", pIoData->_sockfd, bytes_trans, msg_count);

			//// 9.2 �������Ҫ�������ݣ���IOCPͶ�ݷ�����������
			//pIoData->_length = bytes_trans;
			//if (delivery_send(pIoData) < 0)
			//{
			//	printf("CLOSE socket=%d\n", pIoData->_sockfd);
			//	closesocket(pIoData->_sockfd);
			//}

			// 9.2 ��IOCPͶ�ݽ�����������
			if (delivery_receive(pIoData) < 0)
			{
				printf("CLOSE socket=%d\n", pIoData->_sockfd);
				closesocket(pIoData->_sockfd);
				// �ر�һ��������һ��PER_IO_CONTEXT����Ͷ��һ��ACCEPT���ȴ��µĿͻ����ӣ����ֿ������ӵ���������
				delivery_accept_ex(sock_server, pIoData);
			}
			continue;
		}

		printf("undefine action.\n");
		break;
	
	}

	// ----- IOCP END -----//

	// 10.1 �ر�Client Socket
	for (PER_IO_CONTEXT client : ioData)
	{
		closesocket(client._sockfd);
	}
	// 10.2 �ر�Server Socket
	closesocket(sock_server);
	// 10.3 �ر���ɶ˿�
	CloseHandle(iocp_obj);

	// -------------------//
	// ���Windows Socket����
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

#endif