#ifndef _ALLOCTOR_H_
#define _ALLOCTOR_H_

class MemoryMgr;
extern MemoryMgr g_mem_mgr;

void*	operator new		(size_t size);
void	operator delete		(void* pv);
void*	operator new[]		(size_t size);
void	operator delete[]	(void* pv);
void*	mem_alloc			(size_t size);
void	mem_free			(void* pv);
#endif
