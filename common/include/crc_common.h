#ifndef _CRC_COMMON_H_
#define _CRC_COMMON_H_

#include<assert.h>

#ifdef _DEBUG
#include<stdio.h>
#define xPrintf(...) printf(__VA_ARGS__)
#else
#define xPrintf(...)
#endif

#include <stdio.h>
#include <memory>

#include "crc_stream_message.hpp"
#include "crc_message_header.hpp"
#endif
