#ifndef _CRC_WORK_IOCP_CLIENT_HPP_
#define _CRC_WORK_IOCP_CLIENT_HPP_

#ifdef _WIN32

#ifndef CRC_USE_IOCP
#define CRC_USE_IOCP
#endif

#include "crc_work_client.hpp"
#include "crc_iocp.hpp"

class CRCWorkIOCPClient : public CRCWorkClient
{
private:
	CRCIOCP _iocp;
	IO_EVENT _ioEvent = {};

public:
	CRCWorkIOCPClient()
	{

	}
	virtual ~CRCWorkIOCPClient()
	{
		Close();
	}

public:
	// Override from CRCWorkClient
	virtual void OnInitSocket()
	{
		_iocp.create();
		_iocp.register_sock(_pChannel, _pChannel->sockfd());
	};

    // Override from CRCWorkClient
    int OnRun(int microseconds=1)
    {
        if (!IsRunning()) return -1;

		if (_pChannel->is_need_write())
		{
			auto pIoCtxSend = _pChannel->get_send_io_ctx();
			if (pIoCtxSend)
			{
				if (_iocp.delivery_send(pIoCtxSend) < 0)
				{
					CRCLogger_Warn("CRCWorkIOCPClient::OnRun delivery_send failed\n");
					Close();
					return -1;
				}
			}
			auto pIoCtxRecv = _pChannel->get_recv_io_ctx();
			if (pIoCtxRecv)
			{
				if (_iocp.delivery_receive(pIoCtxRecv) < 0)
				{
					CRCLogger_Warn("CRCWorkIOCPClient::OnRun delivery_receive 1 failed\n");
					Close();
					return -1;
				}
			}
		}
		else
		{
			auto pIoCtx = _pChannel->get_recv_io_ctx();
			if (pIoCtx)
			{
				if (_iocp.delivery_receive(pIoCtx) < 0)
				{
					CRCLogger_Warn("CRCWorkIOCPClient::OnRun delivery_receive 2 failed\n");
					Close();
					return -1;
				}
			}
		}

		while (true)
		{
			int ret = DoIOCPAction(microseconds);
			if (ret < 0)
			{
				return ret;
			}
			else if (0 == ret)
			{
				break;
			}
		}

		int ret;

		ret = RecvData();
		if (-1 == ret)
		{
			CRCLogger_Error("CRCWorkIOCPClient::OnRun RecvData exit");
			Close();
			return ret;
		}

		ret = _pChannel->SendDataIM();
		if (-1 == ret)
		{
			Close();
			return ret;
		}

		return 0;
    }

protected:
	// 不同于epoll模式，iocp每次只处理一个网络事件
	// return -1 iocp错误
	// return 0 没有事件
	// return 1 有事件发生
	int DoIOCPAction(int microseconds)
	{
		int ret = _iocp.wait(_ioEvent, microseconds);
		if (ret < 0)
		{
			CRCLogger_Error("WorkIOCPClient::DoIOCPAction wait error");
			return -1;
		}
		else if (0 == ret)
		{
			return ret;
		}

		// 接收数据已经完成 Completion
		if (IO_TYPE::RECV == _ioEvent.pIoCtx->_OpType)
		{
			// 客户端断开处理
			if (_ioEvent.bytesTrans <= 0)
			{
				CRCLogger_Info("CLOSE socket=%d,bytes_trans=%d\n", _ioEvent.pIoCtx->_sockfd, _ioEvent.bytesTrans);
				Close();
				return ret;
			}

			CRCChannel* pChannel = (CRCChannel*)_ioEvent.data.ptr;
			if (pChannel)
			{
				pChannel->recv4Iocp(_ioEvent.bytesTrans);
			}
			else
			{
				CRCLogger_Warn("WorkIOCPClient::DoIOCPAction RECV pChannel==nullptr\n");
			}
		}
		// 发送数据已经完成
		else if (IO_TYPE::SEND == _ioEvent.pIoCtx->_OpType)
		{
			// 客户端断开处理
			if (_ioEvent.bytesTrans <= 0)
			{
				CRCLogger_Info("CLOSE socket=%d,bytes_trans=%d\n", _ioEvent.pIoCtx->_sockfd, _ioEvent.bytesTrans);
				Close();
				return -1;
			}

			CRCChannel* pChannel = (CRCChannel*)_ioEvent.data.ptr;
			if (pChannel)
			{
				_pChannel->send4Iocp(_ioEvent.bytesTrans);
			}
			else
			{
				CRCLogger_Warn("WorkIOCPClient::DoIOCPAction SEND pChannel==nullptr\n");
			}
		}
		else
		{
			CRCLogger_Error("undefine action.");
		}
		return ret;
	}

    // Override from CRCWorkClient
    void OnNetMessage(CRCDataHeader* header)
    {

    }

	void Close()
	{
		_iocp.destory();
		CRCWorkClient::Close();
	}
};
#endif

#endif