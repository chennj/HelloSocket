#ifndef _CHANNEL_HPP_
#define _CHANNEL_HPP_

#include "Init.h"
#include "ObjectPool.hpp"

// countdown to heart beat check
#define CLIENT_HEART_DEAD_TIME 30 * 1000
// time interval in which server timing send data in sending buffer to client 
#define CLIENT_TIMING_SEND_TIME 200

/**
*	client object with socket
*/
class Channel : public ObjectPoolBase<Channel, 10000>
{
public:
	Channel(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szRecvBuffer, 0, RECV_BUFFER_SIZE);
		_lastRecvPos = 0;
		memset(_szSendBuffer, 0, SEND_BUFFER_SIZE);
		_lastSendPos = 0;
		reset_dt_heart();
		reset_dt_send();
	}

	virtual ~Channel()
	{
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
	// immediately send data in sending buffer to client
	int SendDataIM()
	{
		int ret = SOCKET_ERROR;

		if (_lastSendPos > 0 && SOCKET_ERROR != _sockfd)
		{
			ret = send(_sockfd/*client socket*/, _szSendBuffer, _lastSendPos, 0);
			_lastSendPos = 0;
		}
		else
		{
			ret = 0;
		}

		reset_dt_send();

		return ret;
	}

	// send data
	int SendData(DataHeaderPtr pheader)
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
				//
				reset_dt_send();
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

	// reset timing heart beat
	void reset_dt_heart()
	{
		_dtHeart = Time::getNowInMilliSec();
	}

	// check heart beat
	bool check_heart_beat(time_t tNow)
	{
		if ((tNow - _dtHeart) >= CLIENT_HEART_DEAD_TIME)
		{
			printf("check heart beat dead:socket<%d>,time<%d>\n", _sockfd, (tNow - _dtHeart));
			return false;
		}
		return true;
	}

	void reset_dt_send()
	{
		_dtSend = Time::getNowInMilliSec();;
	}

	int timing_send(time_t tNow)
	{
		int ret = SOCKET_ERROR;
		time_t dt = tNow - _dtSend;
		if (dt >= CLIENT_TIMING_SEND_TIME)
		{
			// immediately send data in sending buffer to client
			// reset sending timing
			ret = SendDataIM();
			xPrintf("check send:socket<%d>,time<%d>,result<%d>\n", (int)_sockfd, (int)dt, ret);
		}
		else
		{
			ret = 0;
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

	// timing heart beat check
	time_t _dtHeart;
	// timing send data to client
	time_t _dtSend;
};

#endif
