#ifndef _MEMORYMGR_HPP_
#define _MEMORYMGR_HPP_

#include<stdlib.h>
#include<mutex>

#include"Init.h"
#define ALLOC_MAX_MEM_SIZE 128
#define ALLOC_MAX_BLOCK_QUANTITY 100000

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
	std::mutex _mutex;
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
		xPrintf("create memory allocator instance\n");

		_pBufPool		= nullptr;
		_pHeader		= nullptr;
		_nUnitSize		= 0;
		_nUnitQuantity	= 0;
	}
	~MemoryAlloc()
	{
		xPrintf("release memory pool\n");
		if (_pBufPool)
			free(_pBufPool);
	}

public:
	// apply for memory
	void* alloc_mem(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (!_pBufPool)
		{
			init_pool();
		}

		MemoryBlock* pRet = nullptr;
		if (nullptr == _pHeader)
		{
			pRet = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pRet->_bPool = false;
			pRet->_nID = -1;
			pRet->_nRef = 1;
			pRet->_pAlloc = this;
			pRet->_pNext = nullptr;
			printf("MemoryAlloc\t::alloc_mem:\t%llx, id=%d, size=%d\n", pRet, pRet->_nID, nSize);
		}
		else
		{
			pRet = _pHeader;
			_pHeader = _pHeader->_pNext;
			assert(0 == pRet->_nRef);
			pRet->_nRef = 1;

		}
		xPrintf("MemoryAlloc\t::alloc_mem:\t%llx, id=%d, size=%d\n", pRet, pRet->_nID, nSize);
		return ( (char*)pRet + sizeof(MemoryBlock) );
	}

	// release memory
	void free_mem(void* pv)
	{
		MemoryBlock* pBlock = (MemoryBlock*)( (char*)pv - sizeof(MemoryBlock) );

		assert(1 <= pBlock->_nRef);

		xPrintf("MemoryAlloc\t::free_mem:\t%llx, id=%d\n", pBlock, pBlock->_nID);

		if (pBlock->_bPool)
		{
			std::lock_guard<std::mutex> lg(_mutex);
			if (0 != --pBlock->_nRef)
			{
				return;
			}

			pBlock->_pNext = _pHeader;
			_pHeader = pBlock;
		}
		else
		{
			if (0 != --pBlock->_nRef)
			{
				return;
			}

			free(pBlock);
		}
	}

	// initialize 
	void init_pool()
	{
		xPrintf("initialize memory pool,size of type=%d,unit quantity=%d\n", _nUnitSize - sizeof(MemoryBlock), _nUnitQuantity);

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
			pTemp->_nID = n;
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

		/**
		*	calculate size per unit
		*/
		// n is different value in different system, such as n equal 4 byte in 32 bit system 
		// and 8 byte in 64 bit system
		int n = sizeof(void*);
		// check again to ensure incoming values to being a power of 2,
		// such as if nUnitSize=63, then as a result _nUnitSize=64 after
		// calculated from fllowing below sentence.
		while ((n <<= 1) < nUnitSize);
		// calculate unit's size
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

	MemoryTplOfAlloc<64,	ALLOC_MAX_BLOCK_QUANTITY * 2> _mem64_allocator;
	MemoryTplOfAlloc<128,	ALLOC_MAX_BLOCK_QUANTITY> _mem128_allocator;
	//MemoryTplOfAlloc<256,	ALLOC_MAX_BLOCK_QUANTITY> _mem256_allocator;
	//MemoryTplOfAlloc<512,	ALLOC_MAX_BLOCK_QUANTITY> _mem512_allocator;
	//MemoryTplOfAlloc<1024,	ALLOC_MAX_BLOCK_QUANTITY> _mem1024_allocator;

	MemoryAlloc* _szAlloc[ALLOC_MAX_MEM_SIZE + 1];

private:
	MemoryMgr()
	{
		init_allocator(0,	64,		&_mem64_allocator);
		init_allocator(65,	128,	&_mem128_allocator);
		//init_allocator(129, 256,	&_mem256_allocator);
		//init_allocator(257, 512,	&_mem512_allocator);
		//init_allocator(513, 1024,	&_mem1024_allocator);
		xPrintf("create memory manager instance\n");
	}
	~MemoryMgr()
	{
		xPrintf("destory memory manager instance\n");
	}

	MemoryMgr(const MemoryMgr& signal) = delete;
	const MemoryMgr& operator=(const MemoryMgr &signal) = delete;

private:
	// initialize buffer pool mapping array
	void init_allocator(int nBegin, int nEnd, MemoryAlloc* pAlloc)
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
			xPrintf("MemoryMgr\t::alloc_mem:\t%llx, id=%d, size=%d\n", pRet, pRet->_nID, nSize);
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
				xPrintf("MemoryMgr\t::free_mem:\t%llx, id=%d\n", pBlock, pBlock->_nID);
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
