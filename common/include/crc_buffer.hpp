#ifndef _CRC_BUFFER_HPP_
#define _CRC_BUFFER_HPP_

#include "crc_logger.hpp"

class CRCBuffer
{
private:
	// buffer
	char* _pBuf;
	// last position of buffer with data
	int _lastPos;
	// buffer total size in byte
	int _nSize;
	// buffer full count
	int _fullCount;

public:
	CRCBuffer(int nSize = 8192)
	{
		_fullCount = 0;
		_lastPos = 0;
		_nSize = nSize;
		_pBuf = new char[_nSize];
	}

	~CRCBuffer()
	{
		if (_pBuf)
		{
			CRCLogger_Info("~CRCBuffer()\n");
			delete[] _pBuf;
			_pBuf = nullptr;
		}
	}

public:
	// add data
	int push(const char* pData, size_t nLen)
	{
		int ret = 0;

		// buffer is full
		// 动态扩展，保留节目
		//if (_lastPos + nLen > _nSize)
		//{
		//	int n = DEFAULT_BUFFER_SIZE;
		//	while ((n <<= 1) < (_nSize + nLen));
		//	_nSize = n;
		//	char* buffer = new char[_nSize];
		//	memcpy(buffer, _pBuf, _lastPos);
		//	delete[] _pBuf;
		//	_pBuf = buffer;
		//}

		if (_lastPos + nLen <= _nSize)
		{
			// copy data to the end of the buffer
			memcpy(_pBuf + _lastPos, pData, nLen);
			// calculate the tail position of the buffer
			_lastPos += nLen;

			if (_lastPos == _nSize)
			{
				_fullCount++;
			}

			// set return value
			ret = nLen;
		}
		else
		{
			_fullCount++;
		}

		return ret;
	}

	// remove data
	int pop(int nLen)
	{
		if (nLen <= 0)return -1;
		int n = _lastPos - nLen;
		if (n < 0)return -1;
		if (n > 0)
		{
			memcpy(_pBuf, _pBuf + nLen, n);
		}
		_lastPos = n;
		return nLen;
	}

	// write to socket
	int write2socket(SOCKET sockfd)
	{
		int ret = 0;

		if (_lastPos > 0 && INVALID_SOCKET != sockfd)
		{
			//std::lock_guard<std::mutex> lg(_mutex);
			ret = send(sockfd/*client socket*/, _pBuf, _lastPos, 0);
			//reset last position send buffer
			_lastPos = 0;
			// reset count flag which sending buffer is full
			_fullCount = 0;
		}

		return ret;
	}

	// receive data from socket
	int read4socket(SOCKET sockfd)
	{
		// prevent _nSize - _lastPos == 0
		if (_nSize - _lastPos <= 0)
		{
			return 0;
		}

		// calculate position to copy
		char* szRecv = _pBuf + _lastPos;

		// receive client data
		int nLen = recv(sockfd, szRecv, _nSize - _lastPos, 0);

		if (nLen <= 0)
		{
			return nLen;
		}

		// tail position of buffer move backward
		_lastPos += nLen;
		return nLen;
	}

	// if buffer had messages more than one
	bool has_data()
	{
		// whether message buffer size greater than message header(DataHeader)'s size,
		// if yes, converting message buffer to struct DataHeader and clear message buffer
		// had prcessed.
		if (_lastPos >= sizeof(CRCDataHeader))
		{
			// convert message buffer to DataHeader
			CRCDataHeader* pheader = (CRCDataHeader*)_pBuf;
			// whether message buffer size greater than current client message size,
			return _lastPos >= pheader->data_length;
		}

		return false;
	}

	// whether buffer has data
	inline bool has_any_data()
	{
		return _lastPos > 0;
	}

	// return buffer address
	inline char* getBuf()
	{
		return _pBuf;
	}

	// buffer total length
	inline int buf_total_len()
	{
		return _nSize;
	}

	// existing data length
	inline int buf_data_len()
	{
		return _lastPos;
	}

	// ---------------- iocp ---------------------
	inline bool recv4Iocp(int nLen)
	{
		if (nLen > 0 && _nSize - _lastPos >= nLen)
		{
			_lastPos += nLen;
			return true;
		}
		return false;
	}
	inline int send4Iocp(int nLen)
	{
		return pop(nLen);
	}
};

#endif
