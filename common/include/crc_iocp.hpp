#ifndef _CRC_IOCP_HPP_
#define _CRC_IOCP_HPP_

#ifdef _WIN32

#include "crc_common.h"
#include "crc_logger.hpp"
#include <MSWSock.h>

#define IO_BUFFER_SIZE 1024

enum IO_TYPE
{
	ACCEPT = 10,
	RECV,
	SEND
};

typedef struct _IO_CONTEXT {
	OVERLAPPED	_Overlapped;				// 每一个重叠I/O网络操作都要有一个               
	SOCKET		_sockfd;					// 这个I/O操作所使用的Socket，每个连接的都是一样的 
	WSABUF		_wsaBuf;					// 存储数据的缓冲区，用来给重叠操作传递参数的，关于WSABUF后面还会讲 
	char		_szBuffer[IO_BUFFER_SIZE];	// 对应WSABUF里的缓冲区 
	int			_length;					// _szBuffer的实际长度
	IO_TYPE		_OpType;					// 标志这个重叠I/O操作是做什么的，例如Accept/Recv等 
} IO_CONTEXT, *PIO_CONTEXT;

typedef struct _IO_EVENT
{
	PIO_CONTEXT pIoData;
	DWORD bytesTrans = 0;
	SOCKET sock = INVALID_SOCKET;
}IO_EVENT,*PIO_EVENT;

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
		CRCLogger::info("CRCIOCP destory...\n");
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
			printf("WSAIoctl failed with error: %u\n", WSAGetLastError());
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
			CRCLogger_Error("CRCIOCP::register_sock errno=%d\n", GetLastError());
			return false;
		}
		return true;
	}

	// delivery accept to iocp
	int delivery_accept(PIO_CONTEXT pIoData = nullptr)
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

		if (!pIoData)
		{
			CRCLogger_Warn("CRCIOCP::load_acceptex pIoData == nullptr\n");
			pIoData = new IO_CONTEXT;
			memset(pIoData, 0, sizeof(IO_CONTEXT));
		}

		pIoData->_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		pIoData->_OpType = IO_TYPE::ACCEPT;

		BOOL ret = _lpfnAcceptEx(
			_sockServer,
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
				CRCLogger_Error("CRCIOCP::load_acceptex errno=%d\n", GetLastError());
				return -1;
			}
		}
		return 0;
	}

	// delivery receive to iocp
	int delivery_receive(PIO_CONTEXT pIoData)
	{
		if (!pIoData)
		{
			CRCLogger_Error("CRCIOCP::delivery_receive pIoData == nullptr\n");
			return -1;
		}

		pIoData->_OpType = IO_TYPE::RECV;
		WSABUF wsabuf = {};
		wsabuf.buf = pIoData->_szBuffer;
		wsabuf.len = IO_BUFFER_SIZE;
		DWORD flags = 0;
		ZeroMemory(&pIoData->_Overlapped, sizeof(OVERLAPPED));

		if (SOCKET_ERROR == WSARecv(pIoData->_sockfd, &wsabuf, 1, NULL, &flags, &pIoData->_Overlapped, NULL))
		{
			int err = WSAGetLastError();
			if (WSA_IO_PENDING != err)
			{
				printf("ERROR occur while WSARecv. errno=%d\n", GetLastError());
				return -1;
			}
		}
		return 0;
	}

	// delivery send to iocp
	int delivery_send(PIO_CONTEXT pIoData)
	{
		if (!pIoData)
		{
			CRCLogger_Error("CRCIOCP::delivery_send pIoData == nullptr\n");
			return -1;
		}

		pIoData->_OpType = IO_TYPE::SEND;
		WSABUF wsabuf = {};
		wsabuf.buf = pIoData->_szBuffer;
		wsabuf.len = pIoData->_length;
		DWORD flags = 0;
		if (SOCKET_ERROR == WSASend(pIoData->_sockfd, &wsabuf, 1, NULL, flags, &pIoData->_Overlapped, NULL))
		{
			int err = WSAGetLastError();
			if (WSA_IO_PENDING != err)
			{
				printf("ERROR occur while WSASend. errno=%d\n", GetLastError());
				return -1;
			}
		}
		return 0;
	}

	// wait net event ocuur
	int wait(IO_EVENT& ioEvent,int timeout)
	{
		ioEvent.bytesTrans = 0;
		ioEvent.pIoData = nullptr;
		ioEvent.sock = INVALID_SOCKET; 

		// 获取完成端口状态
		BOOL ret = GetQueuedCompletionStatus(
			_IoCompletionPort,
			&ioEvent.bytesTrans,
			(PULONG_PTR)&ioEvent.sock,
			(LPOVERLAPPED*)&ioEvent.pIoData,
			timeout
		);
		// 检查是否有事件发生，和select，epoll_wait类似
		if (!ret)
		{
			int err = GetLastError();
			if (WAIT_TIMEOUT == err)
			{
				return 0;
			}
			if (ERROR_NETNAME_DELETED == err)
			{
				CRCLogger_Warn("CRCIOCP::wait ERROR_NETNAME_DELETED. socket=%d\n", ioEvent.pIoData->_sockfd);
				return 1;
			}
			CRCLogger_Error("CRCIOCP::wait errno=%d\n", GetLastError());
			return -1;
		}

		return 1;
	}

};

#endif

#endif