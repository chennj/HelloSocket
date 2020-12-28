#ifndef _CRC_ALLOCATOR_H_
#define _CRC_ALLOCATOR_H_

#include"crc_init.h"

class CRCMemoryPool;
extern CRCMemoryPool g_mem_mgr;

void*	operator new		(size_t size);
void	operator delete		(void* pv) throw();
void*	operator new[]		(size_t size);
void	operator delete[]	(void* pv) throw();
void*	mem_alloc			(size_t size);
void	mem_free			(void* pv);
#endif
