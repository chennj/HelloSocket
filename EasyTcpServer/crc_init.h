#ifndef _CRC_INIT_H_
#define _CRC_INIT_H_

#include "../common/include/crc_common.h"

class CRCWorkServer;
class CRCWorkSelectServer;
class CRCWorkEpollServer;
class WorkServerSend2ClientTask;
class CRCChannel;

typedef std::shared_ptr<CRCChannel> CRCChannelPtr;
typedef CRCChannelPtr& CRCChannelPtrRef;

typedef std::shared_ptr<CRCWorkServer> CRCWorkServerPtr;
typedef CRCWorkServerPtr& CRCWorkServerPtrRef;

typedef std::shared_ptr<CRCWorkSelectServer> CRCWorkSelectServerPtr;
typedef CRCWorkSelectServerPtr& CRCWorkServerSelectPtrRef;

typedef std::shared_ptr<CRCWorkEpollServer> CRCWorkEpollServerPtr;
typedef CRCWorkEpollServerPtr& CRCWorkEpollServerPtrRef;

typedef std::shared_ptr<CRCDataHeader> CRCDataHeaderPtr;
typedef CRCDataHeaderPtr& CRCDataHeaderPtrRef;

typedef std::shared_ptr<LoginResponse> LoginResponsePtr;
typedef LoginResponsePtr& LoginResponsePtrRef;

#include "../common/include/crc_timestamp.hpp"

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
