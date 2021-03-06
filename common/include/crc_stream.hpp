﻿#ifndef _CRC_STREAM_HPP_
#define _CRC_STREAM_HPP_

#include "crc_common.h"
#include "crc_logger.hpp"
#include <cstdint>
#include <string>

/**
*
*	byte stream
*/
class CRCStream
{
private:
	// 数据缓冲区
	char* _pBuf;
	// capacity of data buffer
	int _nSize;
	// tail position of data which had been write
	int _lastWritePos;
	// tail position of data which had been read
	int _lastReadPos;
	// 当_pBuf时外部传入的数据块时，是由应该被释放。
	bool _bDelete;

public:
	CRCStream(char* pData, int nSize, bool bDelete=false)
	{
		_nSize		= nSize;
		_pBuf		= pData;
		_bDelete	= bDelete;

		_lastWritePos	= 0;
		_lastReadPos	= 0;
	}

	CRCStream(int nSize = 1024)
	{
		_nSize		= nSize;
		_pBuf		= new char[_nSize];
		_bDelete	= true;

		_lastWritePos	= 0;
		_lastReadPos	= 0;
	}

	virtual ~CRCStream()
	{
		if (_bDelete && _pBuf)
		{
			delete[] _pBuf;
			_pBuf = nullptr;
		}
	}

public:
	// 
	inline bool can_read(int n)
	{
		return _nSize - _lastReadPos >= n;
	}

	// 
	inline bool can_write(int n)
	{
		return _nSize - _lastWritePos >= n;
	}
	//
	inline void push(int n)
	{
		_lastWritePos += n;
	}
	//
	inline void pop(int n)
	{
		_lastReadPos += n;
	}

	//
	inline void set_write_pos(int pos)
	{
		_lastWritePos = pos;
	}
	//
	inline int get_write_pos()
	{
		return _lastWritePos;
	}

	/**
	*
	*	read part.
	*/
	template<class T>
	bool read_forward(T& t)
	{
		return read<T>(t, false);
	}

	template<class T>
	bool read(T& t, bool isoffset = true)
	{
		int nLen = sizeof(T);
		if (can_read(nLen))
		{
			memcpy(&t, _pBuf + _lastReadPos, nLen);
			if (isoffset)pop(nLen);
			return true;
		}
		CRCLogger::info("error CRCStream::read failed.\n");
		return false;
	}

	template<class T>
	uint32_t read_array(T* pData, int len)
	{
		uint32_t n = 0;
		// 读取数组元素个数，但不偏移读取位置(_lastReadPos),防止后面的动作不成功
		read<uint32_t>(n, false);
		// 判断缓存数组pData是否放得下
		if (n < len)
		{
			// 计算实际读取字节长度
			int nLen = sizeof(T) * n;
			// 判断能不能读出
			if (can_read(nLen + sizeof(uint32_t)))
			{
				// 更新_lastReadPos = 已读位置+数组长度所占空间
				pop(sizeof(uint32_t));
				// 将要读取的数据拷贝出来
				memcpy(pData, _pBuf + _lastReadPos, nLen);
				// 计算已读数据尾部位置
				pop(nLen);
				// 返回实际读取的元素个数
				return n;
			}
		}
		CRCLogger::info("error CRCStream::read_array failed.\n");
		return 0;
	}

	int8_t read_int8(int8_t n = 0)
	{
		read(n);
		return n;
	}

	int16_t read_int16(int16_t  n = 0)
	{
		read(n);
		return n;
	}

	int32_t read_int32(int32_t  n = 0)
	{
		read(n);
		return n;
	}

	float read_float(float  n = 0)
	{
		read(n);
		return n;
	}

	double read_double(double  n = 0)
	{
		read(n);
		return n;
	}

	/**
	*
	*	read part
	*/
	template<class T>
	bool write(T n)
	{
		// 计算写入数据的大小
		int nLen = sizeof(T);
		// 判断能否写入
		if (can_write(nLen))
		{
			// 将要写入的数据拷贝到缓冲区的尾部
			memcpy(_pBuf + _lastWritePos, &n, nLen);
			// 计算已写数据尾部位置
			push(nLen);
			return true;
		}
		CRCLogger::info("error CRCStream::write failed.\n");
		return false;
	}

	template<class T>
	bool write_array(T* pData, int len)
	{
		// 计算写入数组的数据字节长度
		int n = sizeof(T) * len;
		// 计算写入数组的总长度
		int nLen = n + sizeof(uint32_t);
		// 判断能否写入
		if (can_write(nLen))
		{
			// 先写入数组的元素数量
			write_int32(len);
			// 再将要写入的数据拷贝到缓冲区的尾部
			memcpy(_pBuf + _lastWritePos, pData, n);
			// 计算缓冲区尾部位置
			push(n);
			return true;
		}
		CRCLogger::info("error CRCStream::write_array failed.\n");
		return false;
	}

	bool write_int8(int8_t n)
	{

		return write(n);

	}
	bool write_int16(int16_t n)
	{
		return write(n);
	}

	bool write_int32(int32_t n)
	{
		return write<int32_t>(n);
	}

	bool write_float(float n)
	{
		return write(n);
	}

	bool write_double(double n)
	{
		return write(n);
	}

public:
	int ReadString(std::string& str, int nLen)
	{
		return read_array(const_cast<char*>(str.c_str()), nLen);
	}

	bool WriteString(const char* str, int nLen)
	{
		return write_array(str, nLen);
	}

	bool WriteString(const char* str)
	{
		return write_array(str, strlen(str));
	}

	bool WriteString(std::string& str)
	{
		return write_array(str.c_str(), str.length());
	}

public:
	char* data()
	{
		return _pBuf;
	}

	int data_length()
	{
		return _lastWritePos;
	}
};

#endif