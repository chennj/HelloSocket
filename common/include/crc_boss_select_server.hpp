#ifndef _CRC_BOSS_SELECT_SERVER_HPP_
#define _CRC_BOSS_SELECT_SERVER_HPP_

#include "crc_fdset.hpp"
#include "crc_boss_server.hpp"

/**
*	select mode
*/

class CRCBossSelectServer : public CRCBossServer<CRCWorkSelectServer, CRCWorkSelectServerPtr>
{
private:
	CRCFdSet _fd_read;

public:
	CRCBossSelectServer()
	{

	}
	~CRCBossSelectServer()
	{

	}

protected:
	// override from CRCBossServer
	void OnRun(CRCThread* pCrcThread)
	{
		CRCLogger::info("BossServer thread start...\n");
		//while (pCrcThread->IsRun())
		while (IsRunning())
		{
			time4Msg();

			_fd_read.zero();

			// put server's socket in all the fd_set
			_fd_read.add(_sock);

			//nfds is range of fd_set, not fd_set's count.
			//nfds is also max value+1 of all the file descriptor(socket).
			//nfds can be 0 in the windows.
			//that timeval was setted null means blocking, not null means nonblocking.
			timeval t = { 0,1 };
			//int ret = select(_sock + 1/*nfds*/, &fd_read, &fd_write, &fd_exception, &t);
			// only check readable
			int ret = select(_sock + 1/*nfds*/, _fd_read.get_fd_set(), nullptr, nullptr, &t);
			if (ret < 0)
			{
				if (EINTR == errno)
				{
					continue;
				}
				CRCLogger::info("BossServer socket<%d> error occurs while select and mission finish.\n", (int)_sock);
				//pCrcThread->ExitInSelfThread();
				//Close();
				break;
			}

			if (_fd_read.has(_sock))
			{
				//fd_read.del(_sock);
				Accept();
			}
		}

		pCrcThread->ExitInSelfThread();

		CRCLogger::info("BossServer thread exit...\n");
	}
};

#endif
