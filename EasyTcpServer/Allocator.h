#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

#include"Init.h"

class MemoryMgr;
extern MemoryMgr g_mem_mgr;

void*	operator new		(size_t size);
void	operator delete		(void* pv) throw();
void*	operator new[]		(size_t size);
void	operator delete[]	(void* pv) throw();
void*	mem_alloc			(size_t size);
void	mem_free			(void* pv);
#endif
