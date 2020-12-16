#ifndef _MEMORYMGR_HPP_
#define _MEMORYMGR_HPP_

#include<stdlib.h>
#include<mutex>

/**
*	memory block which is minimum unit of memory
*/
class MemoryBlock
{
private:

public:
	MemoryBlock()
	{

	}
	~MemoryBlock()
	{

	}
};

/**
*	memory alloctor
*/
class MemoryAlloc
{
private:

public:
	MemoryAlloc()
	{

	}
	~MemoryAlloc()
	{

	}
};

/**
*	memory manager tool, thread safe
*/
class MemoryMgr
{
private:
	static MemoryMgr* _instance;
	static std::mutex _mutex;

private:
	MemoryMgr()
	{

	}
	~MemoryMgr()
	{
	}

	MemoryMgr(const MemoryMgr& signal) = delete;
	const MemoryMgr& operator=(const MemoryMgr &signal) = delete;

public:
	// apply for memory
	void* alloc_mem(size_t size)
	{
		return malloc(size);
	}

	// release memory
	void free_mem(void* pv)
	{
		free(pv);
	}

public:
	// single instance mode
	static MemoryMgr& instance();
};

std::mutex MemoryMgr::_mutex;

MemoryMgr* MemoryMgr::_instance = nullptr;

MemoryMgr& MemoryMgr::instance()
{
	if (nullptr == _instance)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (nullptr == _instance)
		{
			_instance = &MemoryMgr();
		}
	}
	return *_instance;
}

#endif
