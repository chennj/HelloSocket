#ifndef _CRC_WORK_IOCP_SERVER_HPP_
#define _CRC_WORK_IOCP_SERVER_HPP_

#ifdef _WIN32

#include "crc_work_server.hpp"
#include "crc_iocp.hpp"

class CRCWorkIOCPServer : public CRCWorkServer
{
private:
	CRCIOCP _iocp;

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
		for (auto iter : _clients)
		{
			if (iter.second->is_need_write())
			{
				err = _epoll.ctl(iter.second, EPOLL_CTL_MOD, iter.second->sockfd(), EPOLLIN | EPOLLOUT);
				if (err < 0)
				{
					CRCLogger::info("epoll.ctl socket<%d> errorno<%d> errmsg<%s>.\n", (int)_sock, errno, strerror(errno));
					break;
				}
			}
			else
			{
				err = _epoll.ctl(iter.second, EPOLL_CTL_MOD, iter.second->sockfd(), EPOLLIN);
				if (err < 0)
				{
					CRCLogger::info("epoll.ctl socket<%d> errorno<%d> errmsg<%s>.\n", (int)_sock, errno, strerror(errno));
					break;
				}
			}

		}

		if (err < 0)
		{
			return 0;
		}

		int ret_events = _epoll.wait(1);

		if (ret_events < 0)
		{
			CRCLogger::info("WorkEpollServer socket<%d> errorno<%d> errmsg<%s>.\n", (int)_sock, errno, strerror(errno));
			return ret_events;
		}
		if (0 == ret_events)
		{
			return ret_events;
		}

		epoll_event * events = _epoll.get_epoll_events();

		for (int n = 0; n < ret_events; n++)
		{
			CRCChannel* pClient = (CRCChannel*)events[n].data.ptr;
			if (!pClient)
			{
				CRCLogger::info("WorkEpollServer error while DoAction,pClient is null.\n");
				continue;
			}
			// ����ͻ���socket�¼�
			// -----------------------------------------
			// ����ͻ��˿ɶ��¼�����ʾ�ͻ���socket�����ݿɶ�
			if (events[n].events & EPOLLIN)
			{
				if (-1 == RecvData(pClient))
				{
					OnClientLeave(pClient);
					continue;
				}
			}
			// ����ͻ��˿�д�¼�����ʾ�ͻ���socket���ڿ��Է������ݣ�û�������ԭ���µĶ�����
			if (events[n].events & EPOLLOUT)
			{
				int ret = pClient->SendDataIM();
				if (-1 == ret)
				{
					OnClientLeave(pClient);
				}
			}
			// -----------------------------------------

		}

		return 1;
	}

	// override from CRCWorkServer
	void OnClientJoin(CRCChannel* pClient)
	{
		_iocp.register_sock(pClient->sockfd());
	}
};

#endif

#endif
