#ifndef _CRC_WORK_EPOLL_SERVER_HPP_
#define _CRC_WORK_EPOLL_SERVER_HPP_

#ifdef __linux__
#include "crc_work_server.hpp"
#include "crc_epoll.hpp"

class CRCWorkEpollServer : public CRCWorkServer
{
private:
	CRCEpoll _epoll;

public:
	CRCWorkEpollServer(SOCKET sock = INVALID_SOCKET):
		CRCWorkServer(sock)
	{
		_epoll.create();
	}
	~CRCWorkEpollServer()
	{
		Close();
		_epoll.destory();
	}

public:
	// epoll mode
	int DoAction()
	{
		// 遍历是否有需要发送的数据
		int err;
		for (auto iter : _clients)
		{
			if (iter.second->is_need_write())
			{
				err = _epoll.ctl(iter.second, EPOLL_CTL_MOD, iter.second->sockfd(), EPOLLIN|EPOLLOUT);
				if (err < 0)
				{
					//CRCLogger::info("epoll.ctl socket<%d> errorno<%d> errmsg<%s>.\n", (int)_sock, errno, strerror(errno));
					break;
				}
			}
			else 
			{
				err = _epoll.ctl(iter.second, EPOLL_CTL_MOD, iter.second->sockfd(), EPOLLIN);
				if (err < 0)
				{
					//CRCLogger::info("epoll.ctl socket<%d> errorno<%d> errmsg<%s>.\n", (int)_sock, errno, strerror(errno));
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
			//CRCLogger::info("WorkEpollServer socket<%d> errorno<%d> errmsg<%s>.\n", (int)_sock, errno, strerror(errno));
			return ret_events;
		}
		if (0 == ret_events)
		{
			return ret_events;
		}

		epoll_event * events = _epoll.get_epoll_events();

		for (int n=0; n < ret_events; n++)
		{
			CRCChannel* pClient = (CRCChannel*)events[n].data.ptr;
			if (!pClient)
			{
				CRCLogger_Warn("WorkEpollServer error while DoAction,pClient is null.\n");
				continue;
			}
			// 处理客户端socket事件
			// -----------------------------------------
			// 处理客户端可读事件，表示客户端socket有数据可读
			if(events[n].events & EPOLLIN)
			{
				if (-1 == RecvData(pClient))
				{
					OnClientLeave(pClient);
					continue;
				}
			}
			// 处理客户端可写事件，表示客户端socket现在可以发送数据（没有因各种原因导致的堵塞）
			if(events[n].events & EPOLLOUT)
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
		_epoll.ctl(pClient, EPOLL_CTL_ADD, pClient->sockfd(), EPOLLIN);
	}
};

#endif

#endif
