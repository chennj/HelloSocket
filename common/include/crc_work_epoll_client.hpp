#ifndef _CRC_WORK_EPOLL_CLIENT_HPP_
#define _CRC_WORK_EPOLL_CLIENT_HPP_

#ifdef __linux__
#include "crc_work_client.hpp"
#include "crc_epoll.hpp"

class CRCWorkEpollClient : public CRCWorkClient
{
private:
	// epoll
	CRCEpoll _epoll;
public:
	CRCWorkEpollClient()
	{

	}
	virtual ~CRCWorkEpollClient()
	{
		Close();
	}

public:
	// Override from CRCWorkClient
	virtual void OnInitSocket()
	{
		_epoll.create(1);
		_epoll.ctl(nullptr, EPOLL_CTL_ADD, _pChannel->sockfd(), EPOLLIN);
	};

    // Override from CRCWorkClient
    int OnRun(int microseconds=1)
    {
        if (!IsRunning()) return -1;

		int err;
		if (_pChannel->is_need_write())
		{
			err = _epoll.ctl(_pChannel, EPOLL_CTL_MOD, _pChannel->sockfd(), EPOLLIN|EPOLLOUT);
			if (err < 0)
			{
				CRCLogger_Error("CRCWorkEpollClient::OnRun ctl EPOLLIN|EPOLLOUT socket<%d> errorno<%d> errmsg<%s>.\n", (int)_pChannel->sockfd(), errno, strerror(errno));
			}
		}
		else 
		{
			err = _epoll.ctl(_pChannel, EPOLL_CTL_MOD, _pChannel->sockfd(), EPOLLIN);
			if (err < 0)
			{
				CRCLogger_Error("CRCWorkEpollClient::OnRun ctl EPOLLIN socket<%d> errorno<%d> errmsg<%s>.\n", (int)_pChannel->sockfd(), errno, strerror(errno));
			}
		}

		
		if (err < 0)
		{
			Close();
			return err;
		}		

		int ret_events = _epoll.wait(microseconds);

		if (ret_events < 0)
		{
			CRCLogger_Error("CRCWorkEpollClient::OnRun socket<%d> errorno<%d> errmsg<%s>.\n", (int)_pChannel->sockfd(), errno, strerror(errno));
			Close();
			return ret_events;
		}
		if (0 == ret_events)
		{
			return ret_events;
		}

		epoll_event * events = _epoll.get_epoll_events();

		for (int n=0; n < ret_events; n++)
		{
			CRCChannel* pChannel = (CRCChannel*)events[n].data.ptr;
			if (!pChannel)
			{
				CRCLogger_Error("CRCWorkEpollClient::OnRun pChannel is null.\n");
				Close();
				continue;
			}
			// 处理客户端socket事件
			// -----------------------------------------
			// 处理客户端可读事件，表示客户端socket有数据可读
			if(events[n].events & EPOLLIN)
			{
				if (-1 == RecvData())
				{
					CRCLogger_Error("CRCWorkEpollClient::OnRun RecvData.\n");
					Close();
					continue;
				}
			}
			// 处理客户端可写事件，表示客户端socket现在可以发送数据（没有因各种原因导致的堵塞）
			if(events[n].events & EPOLLOUT)
			{
				int ret = pChannel->SendDataIM();
				if (-1 == ret)
				{
					CRCLogger_Error("CRCWorkEpollClient::OnRun SendDataIM.\n");
					Close();
				}
			}
			// -----------------------------------------
		}
		return 0;
    }

    // Override from CRCWorkClient
    void OnNetMessage(CRCDataHeader* header)
    {

    }

	void Close()
	{
		_epoll.destory();
		CRCWorkClient::Close();
	}
};
#endif

#endif