#ifndef _CHANNEL_HPP_
#define _CHANNEL_HPP_

#include "Init.h"
#include "ObjectPool.hpp"

/**
*	client object with socket
*/
class Channel : public ObjectPoolBase<Channel, 1000>
{
public:
	Channel(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szRecvBuffer, 0, RECV_BUFFER_SIZE);
		_lastRecvPos = 0;
		memset(_szSendBuffer, 0, SEND_BUFFER_SIZE);
		_lastSendPos = 0;
	}

	virtual ~Channel()
	{
		//delete[] (void*)_szRecvBuffer;
	}

public:
	SOCKET sockfd()
	{
		return _sockfd;
	}

	char * recvBuf()
	{
		return _szRecvBuffer;
	}

	int GetLastRecvPos()
	{
		return _lastRecvPos;
	}

	void SetLastRecvPos(int lastRecvPos)
	{
		_lastRecvPos = lastRecvPos;
	}

	char * sendBuf()
	{
		return _szSendBuffer;
	}

	int GetLastSendPos()
	{
		return _lastSendPos;
	}

	void SetLastSendPos(int lastSendPos)
	{
		_lastSendPos = lastSendPos;
	}
public:
	// send data
	int SendData(DataHeaderPtrRef pheader)
	{
		int ret = SOCKET_ERROR;
		if (!pheader) {
			return ret;
		}
		// it's data length that would send
		int nSendLen = pheader->data_length;
		// it's data that would send
		const char* pSendData = (const char*)pheader.get();
		// 
		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFFER_SIZE)
			{
				// calculate the length of data that can be copied
				int nCanCopyLen = SEND_BUFFER_SIZE - _lastSendPos;
				// copy data to send buffer
				memcpy(_szSendBuffer + _lastSendPos, pSendData, nCanCopyLen);
				// position of remaining data
				pSendData += nCanCopyLen;
				// length of remaining data
				nSendLen -= nCanCopyLen;
				// send data
				ret = send(_sockfd/*client socket*/, _szSendBuffer, SEND_BUFFER_SIZE, 0);
				// set _lastSendPos to zero
				_lastSendPos = 0;
				// exception occur while send,such as client offline
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
			}
			else
			{
				// copy data to be sent to the end of the send buffer
				memcpy(_szSendBuffer + _lastSendPos, pSendData, nSendLen);
				// calculate the tail position of the send buffer
				_lastSendPos += nSendLen;
				// set return value
				ret = 0;
				// exit while
				break;
			}
		}

		return ret;
	}

private:
	// fd_set file descriptor
	SOCKET _sockfd;
	// recv message buffer
	char _szRecvBuffer[RECV_BUFFER_SIZE];
	// recv message buffer position
	int _lastRecvPos;

	// send message buffer
	char _szSendBuffer[SEND_BUFFER_SIZE];
	// send message buffer position
	int _lastSendPos;
};

#endif
