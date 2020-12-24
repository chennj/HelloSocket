#include "Allocator.h"
#include "MemoryMgr.hpp"

void* operator new(size_t size)
{
	return MemoryMgr::instance().alloc_mem(size);
}

void operator delete(void * pv)
{
	MemoryMgr::instance().free_mem(pv);
}

void* operator new[](size_t size)
{
	return MemoryMgr::instance().alloc_mem(size);
}

void operator delete[](void * pv)
{
	MemoryMgr::instance().free_mem(pv);
}

void* mem_alloc(size_t size)
{
	return MemoryMgr::instance().alloc_mem(size);
}

void mem_free(void* pv)
{
	MemoryMgr::instance().free_mem(pv);
}