#ifndef _CRC_FDSET_HPP_
#define _CRC_FDSET_HPP_

#include "crc_common.h"

class CRCFdSet
{
private:
	fd_set * _pFdSet;
	int _nFdSize;

public:
	CRCFdSet()
	{
		int nSockNum = FD_SETSIZE;
#ifdef _WIN32
		_nFdSize = sizeof(u_int) + sizeof(SOCKET) * nSockNum;
#else
		_nFdSize = nSockNum  / (8 * sizeof(char));
#endif
		_pFdSet = (fd_set*)new char[_nFdSize];
		memset(_pFdSet, 0, _nFdSize);
	}
	~CRCFdSet()
	{
		if (_pFdSet)
		{
			delete[] _pFdSet;
			_pFdSet = nullptr;
		}
	}

public:
	inline void add(SOCKET s)
	{
		FD_SET(s, _pFdSet);
	}

	inline void del(SOCKET s)
	{
		FD_CLR(s, _pFdSet);
	}

	inline void zero()
	{
#ifdef _WIN32
		FD_ZERO(_pFdSet);
#else
		memset(_pFdSet, 0, _nFdSize);
#endif
	}

	inline bool has(SOCKET s)
	{
		return FD_ISSET(s, _pFdSet);
	}

	inline fd_set* get_fd_set()
	{
		return _pFdSet;
	}

	inline int get_fd_set_size()
	{
		return _nFdSize;
	}

	void copy(CRCFdSet & src)
	{
		memcpy(_pFdSet, src.get_fd_set(), src.get_fd_set_size());
	}
};

#endif