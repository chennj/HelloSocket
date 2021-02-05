#ifndef _CRC_IOCP_HPP_
#define _CRC_IOCP_HPP_

#ifdef _WIN32

#include "crc_common.h"
#include "crc_logger.hpp"
#include <MSWSock.h>

//#define IO_BUFFER_SIZE 1024

class CRCIOCP
{
private:
	LPFN_ACCEPTEX _lpfnAcceptEx = NULL;
	HANDLE _IoCompletionPort = NULL;
	SOCKET _sockServer = INVALID_SOCKET;

protected:

public:
	CRCIOCP()
	{

	}

	~CRCIOCP()
	{
		CRCLogger_Info("~CRCIOCP()\n");
		destory();
	}

public:
	// preloading accept
	int load_acceptex(SOCKET listenSocket)
	{
		if (INVALID_SOCKET != _sockServer)
		{
			CRCLogger_Warn("CRCIOCP::load_acceptex INVALID_SOCKET != _sockServer\n");
			return 1;
		}
		if (_lpfnAcceptEx)
		{
			CRCLogger_Warn("CRCIOCP::load_acceptex _lpfnAcceptEx != NULL\n");
			return 1;
		}

		_sockServer			= listenSocket;
		GUID GuidAcceptEx	= WSAID_ACCEPTEX;
		DWORD dwBytes		= 0;

		int iResult = WSAIoctl(_sockServer, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidAcceptEx, sizeof(GuidAcceptEx),
			&_lpfnAcceptEx, sizeof(_lpfnAcceptEx),
			&dwBytes, NULL, NULL);
		if (iResult == SOCKET_ERROR) {
			CRCLogger_Error("WSAIoctl failed with error");
			return -1;
		}
		return 0;
	}

	// create io completion port object
	bool create()
	{
		// 创建完成端口IOCP
		_IoCompletionPort = CreateIoCompletionPort(
			INVALID_HANDLE_VALUE,
			NULL,
			0,
			0
		);
		if (NULL == _IoCompletionPort)
		{
			printf("ERROR occur while create io completion port. errno=%d\n", GetLastError());
			return false;
		}

		return true;
	}

	// release resource
	void destory()
	{
		if (_IoCompletionPort)
		{
			CloseHandle(_IoCompletionPort);
			_IoCompletionPort = NULL;
		}
	}

	// server socket is associated with io completion port
	bool register_sock(SOCKET sockfd)
	{
		// 关联IOCP与SERVER SOCKET
		HANDLE ret = CreateIoCompletionPort(
			(HANDLE)sockfd,
			_IoCompletionPort,
			(ULONG_PTR)sockfd,		// 可以是自定义结构体、对象、数组、等的指针，
									// 也可以是一个基础类型，这里简单传一个socket值
			0
		);
		if (NULL == ret)
		{
			CRCLogger_Error("CRCIOCP::register_sock ");
			return false;
		}
		return true;
	}

	// server socket is associated with io completion port
	bool register_sock(void* pChannel, SOCKET socket)
	{
		// 关联IOCP与SERVER SOCKET
		HANDLE ret = CreateIoCompletionPort(
			(HANDLE)socket,
			_IoCompletionPort,
			(ULONG_PTR)pChannel,		// 可以是自定义结构体、对象、数组、等的指针，
										// 也可以是一个基础类型，这里简单传一个socket值
			0
		);
		if (NULL == ret)
		{
			CRCLogger_Error("CRCIOCP::register_sock failed");
			return false;
		}
		return true;
	}

	// delivery accept to iocp
	int delivery_accept(PIO_CONTEXT pIoCtx = nullptr)
	{
		if (INVALID_SOCKET == _sockServer)
		{
			CRCLogger_Error("CRCIOCP::load_acceptex INVALID_SOCKET == _sockServer\n");
			return -1;
		}

		if (_lpfnAcceptEx == nullptr)
		{
			CRCLogger_Error("CRCIOCP::load_acceptex _lpfnAcceptEx == nullptr\n");
			return -1;
		}

		if (!pIoCtx)
		{
			CRCLogger_Error("CRCIOCP::load_acceptex pIoData == nullptr");
			//pIoCtx = new IO_CONTEXT;
			//memset(pIoCtx, 0, sizeof(IO_CONTEXT));
			return -1;
		}

		pIoCtx->_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		pIoCtx->_OpType = IO_TYPE::ACCEPT;

		BOOL ret = _lpfnAcceptEx(
			_sockServer,
			pIoCtx->_sockfd,				// 用来接受连接的socket（Accept Socket）
			pIoCtx->_wsabuf.buf,
			0,								// 如果不为0，则表示客户端连接到服务端之后，还必须再传送至少
											// 一个字节大小的数据，服务端才会接受客户端的连接。
											// 例如：设置此参数为 sizeof(buf) - ((sizeof (sockaddr_in) + 16) * 2)
			sizeof(sockaddr_in) + 16,		// | 如果第三个参数不是0，这两个参数将从这个参数指定大小的位置开始存储，
			sizeof(sockaddr_in) + 16,		// | 否则从buf的0位置开始存储
			NULL,							// 接收到的字节数。不过只在同步（阻塞）模式下，这个参数才有意义，这里可以时0
			&pIoCtx->_Overlapped			// 重叠体，供IOCP模式内部使用，不能NULL
		);
		if (FALSE == ret)
		{
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err)
			{
				CRCLogger_Error("CRCIOCP::delivery_accept FAIL");
				return -1;
			}
		}
		return 0;
	}

	// delivery receive to iocp
	int delivery_receive(PIO_CONTEXT pIoCtx)
	{
		if (!pIoCtx)
		{
			CRCLogger_Error("CRCIOCP::delivery_receive pIoData == nullptr\n");
			return -1;
		}

		pIoCtx->_OpType = IO_TYPE::RECV;
		DWORD flags = 0;
		ZeroMemory(&pIoCtx->_Overlapped, sizeof(OVERLAPPED));

		if (SOCKET_ERROR == WSARecv(pIoCtx->_sockfd, &pIoCtx->_wsabuf, 1, NULL, &flags, &pIoCtx->_Overlapped, NULL))
		{
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err)
			{
				if (WSAECONNRESET == err)
				{
					CRCLogger_Warn("CRCIOCP::delivery_receive An existing connection was forcibly closed by the remote host.\n");
					return -1;
				} 
				CRCLogger_Error("CRCIOCP::delivery_receive error");
				return -1;
			}
		}
		return 0;
	}

	// delivery send to iocp
	int delivery_send(PIO_CONTEXT pIoCtx)
	{
		if (!pIoCtx)
		{
			CRCLogger_Error("CRCIOCP::delivery_send pIoData == nullptr");
			return -1;
		}

		pIoCtx->_OpType = IO_TYPE::SEND;
		DWORD flags = 0;
		if (SOCKET_ERROR == WSASend(pIoCtx->_sockfd, &pIoCtx->_wsabuf, 1, NULL, flags, &pIoCtx->_Overlapped, NULL))
		{
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err)
			{
				if (WSAECONNRESET == err)
				{
					CRCLogger_Warn("CRCIOCP::delivery_send An existing connection was forcibly closed by the remote host.\n");
					return -1;
				}
				CRCLogger_Error("CRCIOCP::delivery_send error");
				return -1;
			}
		}
		return 0;
	}

	// wait net event ocuur
	int wait(IO_EVENT& ioEvent,int timeout)
	{
		ioEvent.bytesTrans = 0;
		ioEvent.pIoCtx = NULL;
		ioEvent.data.ptr = NULL;

		// 获取完成端口状态
		BOOL ret = GetQueuedCompletionStatus(
			_IoCompletionPort,
			&ioEvent.bytesTrans,
			(PULONG_PTR)&ioEvent.data,
			(LPOVERLAPPED*)&ioEvent.pIoCtx,
			timeout
		);
		// 检查是否有事件发生，和select，epoll_wait类似
		if (FALSE == ret)
		{
			int err = GetLastError();
			if (WAIT_TIMEOUT == err)
			{
				return 0;
			}
			if (ERROR_NETNAME_DELETED == err)
			{
				CRCLogger_Warn("CRCIOCP::wait ERROR_NETNAME_DELETED. socket=%d;\n", ioEvent.pIoCtx->_sockfd);
				return 1;
			}
			if (ERROR_CONNECTION_ABORTED == err)
			{
				CRCLogger_Warn("CRCIOCP::wait ERROR_CONNECTION_ABORTED. socket=%d;\n", ioEvent.pIoCtx->_sockfd);
				return 2;
			}
			CRCLogger_Error("CRCIOCP::wait failed.");
			return -1;
		}

		return 1;
	}

};

#endif

#endif