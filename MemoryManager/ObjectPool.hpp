#ifndef _OBJECTPOOL_HPP_
#define _OBJECTPOOL_HPP_

template<class Type, size_t nPoolQuantity>
class ObjectPool
{
protected:
	struct NodeHeader
	{
		NodeHeader* pNext;
		int nID;
		char nRef;
		bool bPool;
		// reserve, not use
		char c1;
		char c2;
	};

private:
	NodeHeader* _pNodeHeader;
	// object pool address
	char* _pObjPool;
	// lock
	std::mutex _mutex;

public:
	ObjectPool()
	{

	}
	~ObjectPool()
	{

	}

public:
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
		size_t nSize = nPoolQuantity * (sizeof(Type) + sizeof(NodeHeader));
		_pObjPool = new char[nSize];

		/**
		*	init object pool
		*/
		// convert _pBufPool to MemoryBlock
		_pNodeHeader = (NodeHeader*)_pObjPool;
		// init first MemoryBlock
		_pNodeHeader->_bPool = true;
		_pNodeHeader->_nID = 0;
		_pNodeHeader->_nRef = 0;
		_pNodeHeader->_pNext = nullptr;
		// calculate address of next unit and link all of units to chain
		NodeHeader* pCurr = _pNodeHeader;
		for (size_t n = 1; n < nPoolQuantity; n++)
		{
			NodeHeader* pTemp = (NodeHeader*)((char*)_pNodeHeader + (n * sizeof(NodeHeader)));

			pTemp->_bPool = true;
			pTemp->_nID = n;
			pTemp->_nRef = 0;
			pTemp->_pNext = nullptr;

			pCurr->_pNext = pTemp;
			pCurr = pTemp;
		}
	}
	// release obj pool
	// apply object
};

template<class Type>
class ObjectPoolBase
{
private:

public:
	ObjectPoolBase()
	{

	}

	~ObjectPoolBase()
	{

	}

public:
	void* operator new(size_t size)
	{
		printf("pool new\n");
		return malloc(size);
	}

	void operator delete(void * pv)
	{
		printf("pool delete\n");
		free(pv);
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
};

#endif
