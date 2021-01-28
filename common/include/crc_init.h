#ifndef _CRC_INIT_H_
#define _CRC_INIT_H_

#include "crc_common.h"

class CRCWorkServer;
class CRCWorkSelectServer;
class CRCWorkEpollServer;
class CRCWorkIOCPServer;
class WorkServerSend2ClientTask;
class CRCChannel;

typedef std::shared_ptr<CRCChannel> CRCChannelPtr;

typedef std::shared_ptr<CRCWorkServer> CRCWorkServerPtr;

typedef std::shared_ptr<CRCWorkSelectServer> CRCWorkSelectServerPtr;

typedef std::shared_ptr<CRCWorkEpollServer> CRCWorkEpollServerPtr;

typedef std::shared_ptr<CRCWorkIOCPServer> CRCWorkIOCPServerPtr;

typedef std::shared_ptr<CRCDataHeader> CRCDataHeaderPtr;

typedef std::shared_ptr<LoginResponse> LoginResponsePtr;

#include "crc_timestamp.hpp"

// minimum buffer size
#ifndef RECV_BUFFER_SIZE
#define RECV_BUFFER_SIZE 1024*10
#endif

#ifndef SEND_BUFFER_SIZE
#define SEND_BUFFER_SIZE 1024*10
#endif

#ifndef DEFAULT_BUFFER_SIZE
#define DEFAULT_BUFFER_SIZE 1024*4
#endif

enum IO_TYPE
{
	ACCEPT = 10,
	RECV,
	SEND
};

typedef struct _IO_CONTEXT {
	OVERLAPPED	_Overlapped;				// 每一个重叠I/O网络操作都要有一个               
	SOCKET		_sockfd;					// 这个I/O操作所使用的Socket，每个连接的都是一样的 
	WSABUF		_wsabuf;					// _szBuffer的实际长度
	IO_TYPE		_OpType;					// 标志这个重叠I/O操作是做什么的，例如Accept/Recv等 
} IO_CONTEXT, *PIO_CONTEXT;

typedef struct _IO_EVENT
{
	union {
		void*	ptr = nullptr;
		SOCKET	sock;
	}data;
	PIO_CONTEXT pIoData;
	DWORD		bytesTrans = 0;

}IO_EVENT, *PIO_EVENT;

#endif
