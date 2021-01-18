#ifndef _CRC_BOSS_EPOLL_SERVER_HPP_
#define _CRC_BOSS_EPOLL_SERVER_HPP_

#ifdef __linux__
#include "../common/include/crc_epoll.hpp"
#include "crc_boss_server.hpp"

/**
*	select mode
*/

class CRCBossEpollServer : public CRCBossServer<CRCWorkEpollServer, CRCWorkEpollServerPtr>
{
private:

public:
	CRCBossEpollServer()
	{

	}
	virtual ~CRCBossEpollServer()
	{
	}

protected:
	// override from CRCBossServer
	void OnRun(CRCThread* pCrcThread)
	{
		CRCLogger::info("BossServer thread start...\n");

		CRCEpoll epoll;
		// 因为这里只关心服务端的socket，所以只需要一个epoll_event
		epoll.create(1);
		epoll.ctl(nullptr, EPOLL_CTL_ADD, _sock, EPOLLIN);

		while (IsRunning())
		{
			time4Msg();

			int ret_events = epoll.wait(1);
			if (ret_events < 0)
			{
				CRCLogger::info("BossServer socket<%d> error occurs while epoll and mission finish.\n", (int)_sock);
				break;
			}

			if (0 == ret_events)continue;

			Accept();
		}

		pCrcThread->ExitInSelfThread();

		CRCLogger::info("BossServer thread exit...\n");
	}
};

#endif

#endif
