#ifndef _MEMORYMGR_HPP_
#define _MEMORYMGR_HPP_

#include<stdlib.h>
#include<mutex>
#include<assert.h>

#define ALLOC_MAX_MEM_SIZE 64

class MemoryAlloc;

/**
*	memory block which is minimum unit of memory
*	sizeof(MemoryBlock) = 32 byte in 64bit(x64) system, but = 20 byte in 32bit(x86) systerm
*/
class MemoryBlock
{
public:
	// which block it belong to
	MemoryAlloc* _pAlloc;
	// next block
	MemoryBlock* _pNext;
	// memory block number
	int _nID;
	// count reference
	int _nRef;
	// whether this block is in the pool
	bool _bPool;

private:
	// reserve
	char c1;
	char c2;
	char c3;

public:
	MemoryBlock()
	{

	}
	~MemoryBlock()
	{

	}
};

/**
*	memory allocator
*/
class MemoryAlloc
{
protected:
	// address of the memory pool
	char* _pBufPool;
	// header address point to MemoryBlock in memory pool
	MemoryBlock* _pHeader;
	// size per unit in buffer pool
	// unit  = MemoryBlock + body
	size_t _nUnitSize;
	// quantity of unit in buffer pool
	size_t _nUnitQuantity;

public:
	MemoryAlloc()
	{
		_pBufPool		= nullptr;
		_pHeader		= nullptr;
		_nUnitSize		= 0;
		_nUnitQuantity	= 0;
	}
	~MemoryAlloc()
	{
		printf("release buffer pool\n");
		if (_pBufPool)
			free(_pBufPool);
	}

public:
	// apply for memory
	void* alloc_mem(size_t size)
	{
		if (!_pBufPool)
		{
			init_pool();
		}

		MemoryBlock* pRet = nullptr;
		if (nullptr == _pHeader)
		{
			pRet = (MemoryBlock*)malloc(size + sizeof(MemoryBlock));
			pRet->_bPool = false;
			pRet->_nID = -1;
			pRet->_nRef = 1;
			pRet->_pAlloc = this;
			pRet->_pNext = nullptr;
		}
		else
		{
			pRet = _pHeader;
			assert(0 == pRet->_nRef);
			pRet->_nRef = 1;

			_pHeader = _pHeader->_pNext;
		}

		return ( (char*)pRet + sizeof(MemoryBlock) );
	}

	// release memory
	void free_mem(void* pv)
	{
		MemoryBlock* pBlock = (MemoryBlock*)( (char*)pv - sizeof(MemoryBlock) );

		assert(1 <= pBlock->_nRef);

		if (0 != --pBlock->_nRef)
		{
			return;
		}

		if (pBlock->_bPool)
		{
			pBlock->_pNext = _pHeader;
			_pHeader->_pNext = pBlock;
		}
		else
		{
			free(pBlock);
		}
	}

	// initialize 
	void init_pool()
	{
		// assert _pBufPool must be null
		assert(nullptr == _pBufPool);
		if (_pBufPool)
			return;

		/**
		*	request memory of the pool from system
		*/
		// calculate pool capacity
		size_t poolCapacity = _nUnitSize * _nUnitQuantity;
		// allocate memory
		_pBufPool = (char*)malloc(poolCapacity);

		/**
		*	initialize the memory pool
		*/
		// convert _pBufPool to MemoryBlock
		_pHeader = (MemoryBlock*)_pBufPool;

		// init first MemoryBlock
		_pHeader->_bPool	= true;
		_pHeader->_nID		= 0;
		_pHeader->_nRef		= 0;
		_pHeader->_pAlloc	= this;
		_pHeader->_pNext	= nullptr;

		// calculate address of next unit and link all of units to chain
		MemoryBlock* pCurr = _pHeader;
		for (size_t n = 1; n < _nUnitQuantity; n++)
		{
			MemoryBlock* pTemp = (MemoryBlock*)( (char*)_pHeader + (n * _nUnitSize) );

			pTemp->_bPool = true;
			pTemp->_nID = 0;
			pTemp->_nRef = 0;
			pTemp->_pAlloc = this;
			pTemp->_pNext = nullptr;

			pCurr->_pNext = pTemp;
			pCurr = pTemp;
		}

	}
};

/**
*	allocator template of memory allocator which easy to used to initialize MemoryAlloc
*/
template<size_t nUnitSize, size_t nUnitQuantity>
class MemoryTplOfAlloc : public MemoryAlloc
{
public:
	MemoryTplOfAlloc()
	{
		// prevent incoming values from being not a power of 2
		int n = sizeof(void*);
		while ((n <<= 1) < nUnitSize);
		_nUnitSize = n + sizeof(MemoryBlock);

		_nUnitQuantity = nUnitQuantity;
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

	MemoryTplOfAlloc<64, 10> _mem64_allocator;
	MemoryAlloc* _szAlloc[ALLOC_MAX_MEM_SIZE + 1];
private:
	MemoryMgr()
	{
		init(0, 64, &_mem64_allocator);
	}
	~MemoryMgr()
	{
		printf("destory memory manager instance\n");
	}

	MemoryMgr(const MemoryMgr& signal) = delete;
	const MemoryMgr& operator=(const MemoryMgr &signal) = delete;

private:
	// initialize buffer pool mapping array
	void init(int nBegin, int nEnd, MemoryAlloc* pAlloc)
	{
		for (int n = nBegin; n <= nEnd; n++)
		{
			_szAlloc[n] = pAlloc;
		}
	}

public:
	// apply for memory
	void* alloc_mem(size_t nSize)
	{
		if (nSize <= ALLOC_MAX_MEM_SIZE)
		{
			return _szAlloc[nSize]->alloc_mem(nSize);
		}
		else
		{
			MemoryBlock* pRet = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pRet->_bPool = false;
			pRet->_nID = -1;
			pRet->_nRef = 1;
			pRet->_pAlloc = nullptr;
			pRet->_pNext = nullptr;
			return ((char*)pRet + sizeof(MemoryBlock));

		}
	}

	// release memory
	void free_mem(void* pv)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pv - sizeof(MemoryBlock));
		if (pBlock->_bPool)
		{
			pBlock->_pAlloc->free_mem(pv);
		}
		else
		{
			if (0 == --pBlock->_nRef)
			{
				free(pBlock);
			}
		}
	}

	// add ref count for shareing buffer block
	void add_ref(void* pv)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pv - sizeof(MemoryBlock));
		++pBlock->_nRef;
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
			printf("create memory manager instance\n");
			static MemoryMgr mgr;
			_instance = &mgr;
		}
	}
	return *_instance;
	//std::lock_guard<std::mutex> lock(_mutex);
	//static MemoryMgr mgr;
	//return mgr;
}

#endif
