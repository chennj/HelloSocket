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
	// epoll mode
	int DoAction()
	{
	
		// �����Ƿ�����Ҫ���͵�����
		int err;
		//for (auto iter : _clients)
		//{
		//	if (iter.second->is_need_write())
		//	{
		//		err = _epoll.ctl(iter.second, EPOLL_CTL_MOD, iter.second->sockfd(), EPOLLIN | EPOLLOUT);
		//		if (err < 0)
		//		{
		//			CRCLogger::info("epoll.ctl socket<%d> errorno<%d> errmsg<%s>.\n", (int)_sock, errno, strerror(errno));
		//			break;
		//		}
		//	}
		//	else
		//	{
		//		err = _epoll.ctl(iter.second, EPOLL_CTL_MOD, iter.second->sockfd(), EPOLLIN);
		//		if (err < 0)
		//		{
		//			CRCLogger::info("epoll.ctl socket<%d> errorno<%d> errmsg<%s>.\n", (int)_sock, errno, strerror(errno));
		//			break;
		//		}
		//	}

		//}

		//if (err < 0)
		//{
		//	return 0;
		//}

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

		// ���������Ѿ���� Completion
		if (IO_TYPE::RECV == _ioEvent.pIoData->_OpType)
		{
			// �ͻ��˶Ͽ�����
			if (_ioEvent.bytesTrans <= 0)
			{
				CRCLogger_Info("CLOSE socket=%d,bytes_trans=%d", _ioEvent.pIoData->_sockfd, _ioEvent.bytesTrans);
				CRCWorkServer::destory_socket(_ioEvent.pIoData->_sockfd);
				// �ر�һ��������һ��PER_IO_CONTEXT����Ͷ��һ��ACCEPT���ȴ��µĿͻ����ӣ����ֿ������ӵ���������
				_iocp.delivery_accept(_ioEvent.pIoData);
				return 1;
			}
		}
		//// ���������Ѿ����
		//else if (IO_TYPE::SEND == _ioEvent.pIoData->_OpType)
		//{
		//	// �ͻ��˶Ͽ�����
		//	if (_ioEvent.bytesTrans <= 0)
		//	{
		//		CRCLogger_Info("CLOSE socket=%d,bytes_trans=%d", _ioEvent.pIoData->_sockfd, _ioEvent.bytesTrans);
		//		CRCWorkServer::destory_socket(_ioEvent.pIoData->_sockfd);
		//		// �ر�һ��������һ��PER_IO_CONTEXT����Ͷ��һ��ACCEPT���ȴ��µĿͻ����ӣ����ֿ������ӵ���������
		//		_iocp.delivery_accept(_ioEvent.pIoData);
		//		return 1;
		//	}
		//}
		else
		{ 
			CRCLogger_Error("undefine action.");
		}
		return 1;
	}

	// override from CRCWorkServer
	void OnClientJoin(CRCChannel* pClient)
	{
		_iocp.register_sock(pClient, pClient->sockfd());
		auto pIoCtx = pClient->get_recv_io_ctx();
		if (!pIoCtx)
		{
			CRCLogger_Error("WorkIOCPServer::OnClientJoin pIoCtx == null,sock=%d",pClient->sockfd());
			return;
		}
		_iocp.delivery_receive(pIoCtx);
	}
};

#endif

#endif
