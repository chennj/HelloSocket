#include "crc_allocator.h"
#include "crc_memory_pool.hpp"

void* operator new(size_t size)
{
	return CRCMemoryPool::instance().alloc_mem(size);
}

void operator delete(void * pv)
{
	CRCMemoryPool::instance().free_mem(pv);
}

void* operator new[](size_t size)
{
	return CRCMemoryPool::instance().alloc_mem(size);
}

void operator delete[](void * pv)
{
	CRCMemoryPool::instance().free_mem(pv);
}

void* mem_alloc(size_t size)
{
	return CRCMemoryPool::instance().alloc_mem(size);
}

void mem_free(void* pv)
{
	CRCMemoryPool::instance().free_mem(pv);
}