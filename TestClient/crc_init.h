#ifndef _CRC_INIT_H_
#define _CRC_INIT_H_

#include "../common/include/crc_common.h"

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
