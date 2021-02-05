#define _RUN_
#ifdef	_RUN_

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include "../common/include/crc_allocator.h"
#include "../common/include/crc_memory_pool.hpp"
#include "../common/include/crc_init.h"
#include "../common/include/crc_iocp.hpp"

bool g_run = true;
int port = 12345;
const int clients = 10;

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

// -- IOCP基础流程简单封装
int main()
{
	char path[] = { "/log/server.log" };
	CRCLogger::instance().set_log_path(path, "w");
	CRCLogger::instance().start();

	// 启动命令线程
	std::thread t1(cmdThread);
	t1.detach();

	char * test = new char[123];
	delete[] test;

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
		CRCLogger_Error("ERROR occur while binding local port<%d>", port);
		return -1;
	}

	CRCLogger_Info("SUCCESS for binding local port<%d>\n", port);

	// 3. 监听sock_server
	if (SOCKET_ERROR == listen(sock_server, 64))
	{
		CRCLogger_Error("ERROR occur while listening local port<%d>", port);
		return -1;
	}

	CRCLogger_Info("listening port<%d>\n", port);

	// ----- IOCP BEGIN -----//

	CRCIOCP iocp;
	iocp.create();
	iocp.register_sock(sock_server);
	iocp.load_acceptex(sock_server);

	IO_CONTEXT ioData[clients] = {};
	for (int i = 0; i < clients; i++)
	{
		char buffer[1024] = {};
		ioData[i]._wsabuf.len = 1024;
		ioData[i]._wsabuf.buf = buffer;
		if (iocp.delivery_accept(&ioData[i]) < 0) {
			return -1;
		}
	}

	IO_EVENT ioEvent = {};
	while (g_run)
	{
		int ret = iocp.wait(ioEvent, 1);
		// 检查是否有事件发生，和select，epoll_wait类似
		if (0 == ret)continue;
		if (0 > ret)
		{
			CRCLogger_Error("ERROR occur while GetQueuedCompletionStatus. errno=%d", GetLastError());
			break;
		}
		//if (1 == ret)
		//{
		//	int err = GetLastError();
		//	if (ERROR_NETNAME_DELETED == err)
		//	{
		//		CRCLogger_Error("1. close client. socket=%d", ioEvent.pIoCtx->_sockfd);
		//		closesocket(ioEvent.pIoCtx->_sockfd);
		//		continue;
		//	}
		//}
		// 接受连接已经完成
		if (IO_TYPE::ACCEPT == ioEvent.pIoCtx->_OpType)
		{
			CRCLogger_Info("2. new client enter. socket=%d\n", ioEvent.pIoCtx->_sockfd);
			if (FALSE == iocp.register_sock(ioEvent.pIoCtx->_sockfd))
			{
				CRCLogger_Error("3. ERROR occur while client socket is associated with io completion port. errno=%d", GetLastError());
				closesocket(ioEvent.pIoCtx->_sockfd);
				// 关闭一个，再投递一个ACCEPT，等待新的客户连接
				iocp.delivery_accept(ioEvent.pIoCtx);
				continue;
			}
			iocp.delivery_receive(ioEvent.pIoCtx);
			continue;
		}
		// 接收数据已经完成 Completion
		if (IO_TYPE::RECV == ioEvent.pIoCtx->_OpType)
		{
			// 客户端断开处理
			if (ioEvent.bytesTrans <= 0)
			{
				CRCLogger_Error("4. CLOSE socket=%d,bytesTrans=%d", ioEvent.pIoCtx->_sockfd, ioEvent.bytesTrans);
				closesocket(ioEvent.pIoCtx->_sockfd);
				// 关闭一个，复用一次PER_IO_CONTEXT，再投递一个ACCEPT，等待新的客户连接
				iocp.delivery_accept(ioEvent.pIoCtx);
				continue;
			}

			CRCLogger_Info("5. RECEIVE data socket=%d,bytesTrans=%d\n", ioEvent.pIoCtx->_sockfd, ioEvent.bytesTrans);
			ioEvent.pIoCtx->_wsabuf.len = ioEvent.bytesTrans;
			// 向iocp投递发送数据任务
			iocp.delivery_send(ioEvent.pIoCtx);
			continue;
		}
		// 发送数据任务已完成
		if (IO_TYPE::SEND == ioEvent.pIoCtx->_OpType)
		{
			// 客户端断开处理
			if (ioEvent.bytesTrans <= 0)
			{
				CRCLogger_Error("6. CLOSE socket=%d,bytesTrans=%d", ioEvent.pIoCtx->_sockfd, ioEvent.bytesTrans);
				closesocket(ioEvent.pIoCtx->_sockfd);
				// 关闭一个，复用一次PER_IO_CONTEXT，再投递一个ACCEPT，等待新的客户连接
				iocp.delivery_accept(ioEvent.pIoCtx);
				continue;
			}
			// 向iocp投递接收数据任务
			iocp.delivery_receive(ioEvent.pIoCtx);
			continue;
		}

		printf("7. undefine action.\n");
		break;

	}

	// ----- IOCP END -----//

	// 关闭Server Socket
	closesocket(sock_server);
	// 关闭完成端口
	iocp.destory();

	// -------------------//
	// 清除Windows Socket环境
	WSACleanup();
	system("PAUSE");
	return 0;
}

#endif