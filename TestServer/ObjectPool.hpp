#ifndef _OBJECTPOOL_HPP_
#define _OBJECTPOOL_HPP_

#include"Init.h"
#include<mutex>

template<class Type, size_t nPoolQuantity>
class ObjectPool
{
protected:
	struct NodeHeader
	{
		NodeHeader* _pNext;
		int		_nID;
		char	_nRef;
		bool	_bPool;
		// reserve, not use
		char c1;
		char c2;
	};

private:
	NodeHeader* _pHeader;
	// object pool address
	char* _pObjPool;
	// lock
	std::mutex _mutex;

public:
	ObjectPool()
	{
		init_pool();
	}
	~ObjectPool()
	{
		delete _pObjPool;
	}

private:
	// initialize obj pool
	void init_pool()
	{
		// assert _pBufPool must be null
		assert(nullptr == _pObjPool);

		if (_pObjPool)
			return;

		/**
		*	calculate object pool size
		*/
		size_t unitSize = sizeof(NodeHeader) + sizeof(Type);
		size_t poolSize = nPoolQuantity * unitSize;
		_pObjPool = new char[poolSize];

		/**
		*	init object pool
		*/
		// convert _pObjPool to NodeHeader
		_pHeader = (NodeHeader*)_pObjPool;
		// init first NodeHeader
		_pHeader->_bPool = true;
		_pHeader->_nID = 0;
		_pHeader->_nRef = 0;
		_pHeader->_pNext = nullptr;
		// calculate address of next unit and link all of units to chain
		NodeHeader* pCurr = _pHeader;
		for (size_t n = 1; n < nPoolQuantity; n++)
		{
			NodeHeader* pTemp = (NodeHeader*)(_pObjPool + (n * unitSize));

			pTemp->_bPool = true;
			pTemp->_nID = n;
			pTemp->_nRef = 0;
			pTemp->_pNext = nullptr;

			pCurr->_pNext = pTemp;
			pCurr = pTemp;
		}
	}

public:
	// apply object
	void* alloc_obj(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);

		NodeHeader* pRet = nullptr;
		if (nullptr == _pHeader)
		{
			// first to apply from my memory pool
			pRet = (NodeHeader*)new char[sizeof(NodeHeader) + sizeof(Type)];
			pRet->_bPool = false;
			pRet->_nID = -1;
			pRet->_nRef = 1;
			pRet->_pNext = nullptr;
		}
		else
		{
			pRet = _pHeader;
			_pHeader = _pHeader->_pNext;
			assert(0 == pRet->_nRef);
			pRet->_nRef = 1;

		}
		xPrintf("ObjectAlloc\t::alloc_obj:\t%llx, id=%d, size=%d\n", pRet, pRet->_nID, nSize);
		return ((char*)pRet + sizeof(NodeHeader));
	}

	// release obj pool
	void free_obj(void* pv)
	{
		NodeHeader* pNode = (NodeHeader*)((char*)pv - sizeof(NodeHeader));

		assert(1 <= pNode->_nRef);

		if (0 != --pNode->_nRef)
		{
			return;
		}

		xPrintf("ObjectAlloc\t::free_obj:\t%llx, id=%d\n", pNode, pNode->_nID);

		if (pNode->_bPool)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			pNode->_pNext = _pHeader;
			_pHeader = pNode;
		}
		else
		{
			delete[] pNode;
		}
	}
};

template<class Type, size_t nPoolQuantity>
class ObjectPoolBase
{
private:
	typedef ObjectPool<Type, nPoolQuantity> ClassTypePool;
	static ClassTypePool& object_pool()
	{
		// single object pool
		static ClassTypePool sPool;
		return sPool;
	}

public:
	ObjectPoolBase()
	{

	}

	~ObjectPoolBase()
	{

	}

public:
	void* operator new(size_t nSize)
	{
		xPrintf("pool new\n");
		return object_pool().alloc_obj(nSize);
	}

	void operator delete(void * pv)
	{
		xPrintf("pool delete\n");
		object_pool().free_obj(pv);
	}

	template<typename ... Args>
	static Type* create_object(Args ... args)
	{
		Type* objs = new Type(args...);
		return objs;
	}

	static void destory_object(Type* pobj)
	{
		delete pobj;
	}

	template<typename ... Args>
	static std::shared_ptr<Type> create_shared_object(Args ... args)
	{
		return std::make_shared<Type>(args...);
	}
};

#endif
