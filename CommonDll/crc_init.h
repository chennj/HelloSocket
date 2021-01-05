#ifndef _CRC_INIT_H_
#define _CRC_INIT_H_

#include "../common/include/crc_common.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include <string.h>
#include <signal.h>

#define SOCKET int
#define INVALID_SOCKET	(SOCKET)(~0)
#define SOCKET_ERROR			(-1)
#endif

class CRCChannel;

typedef std::shared_ptr<CRCChannel> CRCChannelPtr;
typedef CRCChannelPtr& CRCChannelPtrRef;

typedef std::shared_ptr<CRCDataHeader> CRCDataHeaderPtr;
typedef CRCDataHeaderPtr& CRCDataHeaderPtrRef;

typedef std::shared_ptr<LoginResponse> LoginResponsePtr;
typedef LoginResponsePtr& LoginResponsePtrRef;

#include "../common/include/crc_timestamp.hpp"

// minimum buffer size
#ifndef RECV_BUFFER_SIZE
#define RECV_BUFFER_SIZE 1024*4
#endif

#ifndef SEND_BUFFER_SIZE
#define SEND_BUFFER_SIZE 1024*10
#endif

#ifndef DEFAULT_BUFFER_SIZE
#define DEFAULT_BUFFER_SIZE 1024*4
#endif

#endif
