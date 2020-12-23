#ifndef _INIT_H_
#define _INIT_H_

#include<assert.h>

#ifdef _DEBUG
#include<stdio.h>
#define xPrintf(...) printf(__VA_ARGS__)
#else
#define xPrintf(...)
#endif

#ifdef _WIN32
#ifndef FD_SETSIZE
#define FD_SETSIZE      2506			//windows default FD_SETSIZE equals 64, too small
#endif
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

#define SOCKET int
#define INVALID_SOCKET	(SOCKET)(~0)
#define SOCKET_ERROR			(-1)
#endif

#include <stdio.h>
#include <memory>

class WorkServer;
class WorkServerSend2ClientTask;
class Channel;

typedef std::shared_ptr<Channel> ChannelPtr;
typedef ChannelPtr& ChannelPtrRef;

typedef std::shared_ptr<WorkServer> WorkServerPtr;
typedef WorkServerPtr& WorkServerPtrRef;

#include "MessageHeader.hpp"

typedef std::shared_ptr<DataHeader> DataHeaderPtr;
typedef DataHeaderPtr& DataHeaderPtrRef;

typedef std::shared_ptr<LoginResponse> LoginResponsePtr;
typedef LoginResponsePtr& LoginResponsePtrRef;

#include "Timestamp.hpp"
#include "Task.hpp"

// minimum buffer size
#ifndef RECV_BUFFER_SIZE
#define RECV_BUFFER_SIZE 1024*10
#endif

#ifndef SEND_BUFFER_SIZE
#define SEND_BUFFER_SIZE 1024*10
#endif

#endif
