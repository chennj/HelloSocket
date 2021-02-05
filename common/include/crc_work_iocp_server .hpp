#ifndef _CRC_WORK_IOCP_SERVER_HPP_
#define _CRC_WORK_IOCP_SERVER_HPP_

#ifdef _WIN32

#include "crc_work_server.hpp"
#include "crc_logger.hpp"
#include "crc_iocp.hpp"

class CRCWorkIOCPServer : public CRCWorkServer
{
private:
	CRCIOCP _iocp;
	IO_EVENT _ioEvent = {};

public:
	CRCWorkIOCPServer(SOCKET sock = INVALID_SOCKET) :
		CRCWorkServer(sock)
	{
		_iocp.create();
	}
	~CRCWorkIOCPServer()
	{
		Close();
	}

public:
	// 继承上级类
	int DoAction()
	{
		CRCChannel* pChannel = nullptr;
		for (auto iter = _clients.begin(); iter != _clients.end();)
		{
			auto iter_old = iter;
			iter_old++;
			pChannel = iter->second;
			if (pChannel->is_need_write())
			{
				auto pIoCtx = pChannel->get_send_io_ctx();
				if (pIoCtx)
				{
					if (_iocp.delivery_send(pIoCtx) < 0)
					{
						CRCLogger_Warn("CRCWorkIOCPServer::DoAction delivery_send failed\n");
						OnClientLeave(pChannel);
					}
				}
			}
			else
			{
				auto pIoCtx = pChannel->get_recv_io_ctx();
				if (pIoCtx)
				{
					if (_iocp.delivery_receive(pIoCtx) < 0)
					{
						CRCLogger_Warn("CRCWorkIOCPServer::DoAction delivery_receive failed\n");
						OnClientLeave(pChannel);
					}
				}
			}
			iter = iter_old;
		}

		while (true)
		{
			int ret = DoIOCPAction();
			if (ret < 0)
			{
				return ret;
			}
			else if (0 == ret)
			{
				break;
			}
		}

		DoMessage();

		return 0;
	}

	// 不同于epoll模式，iocp每次只处理一个网络事件
	// return -1 iocp错误
	// return 0 没有事件
	// return 1 有事件发生
	int DoIOCPAction()
	{
		int ret = _iocp.wait(_ioEvent,1);
		if (ret < 0)
		{
			CRCLogger_Error("WorkIOCPServer socket<%d> errorno<%d> errmsg<%s>.", (int)_sock, errno, strerror(errno));
			return ret;
		}
		else if (0 == ret)
		{
			return ret;
		}
		//有更优雅的处理方式，在CRCWorkServer::CheckTime()中
		//else if (2 == ret)
		//{
		//	return 0;
		//}

		// 接收数据已经完成 Completion
		if (IO_TYPE::RECV == _ioEvent.pIoCtx->_OpType)
		{
			// 客户端断开处理
			if (_ioEvent.bytesTrans <= 0)
			{
				CRCLogger_Info("CLOSE socket=%d,bytes_trans=%d\n", _ioEvent.pIoCtx->_sockfd, _ioEvent.bytesTrans);
				OnClientLeave(_ioEvent);
				return ret;
			}

			CRCChannel* pChannel = (CRCChannel*)_ioEvent.data.ptr;
			if (pChannel)
			{
				pChannel->recv4Iocp(_ioEvent.bytesTrans);		
			}
			else
			{
				CRCLogger_Error("CRCWorkIOCPServer::DoIOCPAction RECV pChannel==nullptr");
			}
		}
		// 发送数据已经完成
		else if (IO_TYPE::SEND == _ioEvent.pIoCtx->_OpType)
		{
			// 客户端断开处理
			if (_ioEvent.bytesTrans <= 0)
			{
				CRCLogger_Info("CLOSE socket=%d,bytes_trans=%d\n", _ioEvent.pIoCtx->_sockfd, _ioEvent.bytesTrans);
				OnClientLeave(_ioEvent);
				return ret;
			}

			CRCChannel* pChannel = (CRCChannel*)_ioEvent.data.ptr;
			if (pChannel)
			{
				//int nSend = pChannel->send4Iocp(_ioEvent.bytesTrans);
				//CRCLogger_Info("SEND socket=%d,bytes_trans=%d\n", _ioEvent.pIoCtx->_sockfd, nSend);
				pChannel->send4Iocp(_ioEvent.bytesTrans);
			}
			else
			{
				CRCLogger_Error("CRCWorkIOCPServer::DoIOCPAction SEND pChannel==nullptr");
			}
		}
		else
		{ 
			CRCLogger_Error("undefine action.");
		}
		return ret;
	}

	// override from CRCWorkServer
	void OnClientJoin(CRCChannel* pClient)
	{
		if (FALSE == _iocp.register_sock(pClient, pClient->sockfd()))
		{
			return;
		}
		auto pIoCtx = pClient->get_recv_io_ctx();
		if (!pIoCtx)
		{
			CRCLogger_Error("WorkIOCPServer::OnClientJoin pIoCtx == null,sock=%d",pClient->sockfd());
			return;
		}
		_iocp.delivery_receive(pIoCtx);
	}

	//void DoMessage(CRCChannel* pChannel)
	//{
	//	OnNetRecevie(pChannel);
	//	while (pChannel->HasMessage())
	//	{
	//		OnNetMessage(pChannel, pChannel->front_message());
	//		pChannel->pop_front_message();
	//	}
	//}
};

#endif

#endif
