#ifndef _CRC_CHANNEL_HPP_
#define _CRC_CHANNEL_HPP_

#include "crc_object_pool.hpp"
#include "crc_buffer.hpp"
#include "crc_logger.hpp"

// countdown to heart beat check
#define CLIENT_HEART_DEAD_TIME 15 * 1000
// time interval in which server timing send data in sending buffer to client 
#define CLIENT_TIMING_SEND_TIME 200

/**
*	client object with socket
*/
class CRCChannel : public CRCObjectPoolBase<CRCChannel, 100000>
{

private:
	// fd_set file descriptor
	SOCKET _sockfd;
	
	// receive buffer
	CRCBuffer _recvBuf;
	// sending buffer
	CRCBuffer _sendBuf;

	// timing heart beat check
	time_t _dtHeart;
	// timing send data to client
	time_t _dtSend;
	//
	//std::mutex _mutex;

public:
	CRCChannel(SOCKET sockfd = INVALID_SOCKET):
		_sendBuf(SEND_BUFFER_SIZE),
		_recvBuf(RECV_BUFFER_SIZE)
	{
		_sockfd = sockfd;

		reset_dt_heart();
		reset_dt_send();
	}

	virtual ~CRCChannel()
	{
		CRCLogger_Info("~CRCChannel() socket<%d>\n", _sockfd);
		destory();
	}

public:
	void destory()
	{
		if (INVALID_SOCKET != _sockfd)
		{
#ifdef _WIN32
			closesocket(_sockfd);
#else
			close(_sockfd);
#endif
			_sockfd = INVALID_SOCKET;
			CRCLogger_Info("CRCChannel::destory socket<%d>\n", _sockfd);
		}
	}
	SOCKET sockfd()
	{
		return _sockfd;
	}

public:
	// if had message
	bool HasMessage()
	{
		return _recvBuf.has_data();
	}

	CRCDataHeader* front_message()
	{
		return (CRCDataHeader*)_recvBuf.getBuf();
	}

	void pop_front_message()
	{
		if (HasMessage())
			_recvBuf.pop(front_message()->data_length);
	}

	// receive data
	int RecvData()
	{
		return _recvBuf.read4socket(_sockfd);
	}

	// immediately send data in sending buffer to client
	int SendDataIM()
	{
		int ret = 0;
		// send data
		ret = _sendBuf.write2socket(_sockfd);
		// reset timing send timer
		reset_dt_send();

		return ret;
	}

	// used to send data by synchronous block mode
	int SendData(CRCDataHeader* pheader)
	{
		int ret = SOCKET_ERROR;
		if (!pheader) {
			return ret;
		}

		//// it's data length that would send
		//int nSendLen = pheader->data_length;
		//// it's data that would send
		//const char* pSendData = (const char*)pheader.get();
		////
		////std::lock_guard<std::mutex> lg(_mutex);
		//// 
		//while (true)
		//{
		//	if (_lastSendPos + nSendLen >= SEND_BUFFER_SIZE)
		//	{
		//		// count while buffer is full
		//		_sendBufFullCount++;
		//		// calculate the length of data that can be copied
		//		int nCanCopyLen = SEND_BUFFER_SIZE - _lastSendPos;
		//		// copy data to send buffer
		//		memcpy(_szSendBuffer + _lastSendPos, pSendData, nCanCopyLen);
		//		// position of remaining data
		//		pSendData += nCanCopyLen;
		//		// length of remaining data
		//		nSendLen -= nCanCopyLen;
		//		// send data
		//		ret = send(_sockfd/*client socket*/, _szSendBuffer, SEND_BUFFER_SIZE, 0);
		//		// set _lastSendPos to zero
		//		_lastSendPos = 0;
		//		//
		//		reset_dt_send();
		//		// exception occur while send,such as client offline
		//		if (SOCKET_ERROR == ret)
		//		{
		//			return ret;
		//		}
		//	}
		//	else
		//	{
		//		// copy data to be sent to the end of the send buffer
		//		memcpy(_szSendBuffer + _lastSendPos, pSendData, nSendLen);
		//		// calculate the tail position of the send buffer
		//		_lastSendPos += nSendLen;
		//		// set return value
		//		ret = 0;
		//		// exit while
		//		break;
		//	}
		//}

		return ret;
	}

	// used to send data by asynchronous not-block mode
	int SendDataBuffer(const CRCDataHeader* pheader)
	{
		int ret = SOCKET_ERROR;
		if (!pheader) {
			return ret;
		}

		// it's data length that would send
		int nSendLen = pheader->data_length;
		// it's data that would send
		const char* pSendData = (const char*)pheader;

		ret = _sendBuf.push(pSendData, nSendLen);

		return ret;
	}

	// used to send byte stream
	int SendDataBuffer(const char* pData, int nLen)
	{
		int ret = SOCKET_ERROR;
		if (!pData) {
			return ret;
		}
		return _sendBuf.push(pData, nLen);
	}

	// reset timing heart beat
	void reset_dt_heart()
	{
		_dtHeart = CRCTime::getNowInMilliSec();
	}

	// check heart beat
	bool check_heart_beat(time_t tNow)
	{
		if ((tNow - _dtHeart) >= CLIENT_HEART_DEAD_TIME)
		{
			CRCLogger::info("check heart beat dead:socket<%d>,time<%d>\n", _sockfd, (tNow - _dtHeart));
			return false;
		}
		return true;
	}

	void reset_dt_send()
	{
		_dtSend = CRCTime::getNowInMilliSec();;
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

	bool check_timing_send(time_t tNow)
	{
		time_t dt = tNow - _dtSend;
		if (dt >= CLIENT_TIMING_SEND_TIME)
		{
			return true;
		}
		return false;
	}

	bool is_need_write()
	{
		return _sendBuf.has_any_data();
	}
	// ---------- for iocp --------------------
private:
#ifdef CRC_USE_IOCP
	IO_CONTEXT _IORecvCtx = {};
	IO_CONTEXT _IOSendCtx = {};
	bool _isDeliveryRecv = false;
	bool _isDeliverySend = false;
#endif
public:
#ifdef CRC_USE_IOCP
	bool hasDeliveryIoAction()
	{
		return _isDeliveryRecv || _isDeliverySend;
	}

	IO_CONTEXT * get_recv_io_ctx()
	{
		if (_isDeliveryRecv)return nullptr;
		_isDeliveryRecv = true;

		int len = _recvBuf.buf_total_len() - _recvBuf.buf_data_len();
		if (len > 0)
		{
			_IORecvCtx._wsabuf.buf = _recvBuf.getBuf() + _recvBuf.buf_data_len();
			_IORecvCtx._wsabuf.len = len;
			_IORecvCtx._sockfd = _sockfd;
			return &_IORecvCtx;
		}
		return nullptr;
	}

	void recv4Iocp(int nLen)
	{
		if (!_isDeliveryRecv)
		{
			CRCLogger_Error("CRCChannel::recv4Iocp _isDeliveryRecv is false");
			return;
		}
		_isDeliveryRecv = false;

		if (!_recvBuf.recv4Iocp(nLen))
		{
			CRCLogger_Error("CRCChannel::recv4Iocp socket=%d, nLen=%d", _sockfd, nLen);
		}
	}

	IO_CONTEXT * get_send_io_ctx()
	{
		if (_isDeliverySend)return nullptr;
		_isDeliverySend = true;

		if (_sendBuf.buf_data_len() > 0 && INVALID_SOCKET != _sockfd)
		{
			_IOSendCtx._wsabuf.buf = _sendBuf.getBuf();
			_IOSendCtx._wsabuf.len = _sendBuf.buf_data_len();
			_IOSendCtx._sockfd = _sockfd;
			return &_IOSendCtx;
		}
		return nullptr;
	}

	int send4Iocp(int nLen)
	{
		if (!_isDeliverySend)
		{
			CRCLogger_Error("CRCChannel::send4Iocp _isDeliverySend is false");
			return 0;
		}
		_isDeliverySend = false;

		int nSend = _sendBuf.send4Iocp(nLen);
		if (nSend < 0)
		{
			CRCLogger_Error("CRCChannel::send4Iocp socket=%d, nLen=%d", _sockfd, nLen);
		}
		return nSend;
	}
#endif
};

#endif
