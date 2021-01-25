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

#endif
