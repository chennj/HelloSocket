#ifndef _CRC_BOSS_SERVER_HPP_
#define _CRC_BOSS_SERVER_HPP_

#include "crc_init.h"
#include "crc_inet_event.hpp"
#include "crc_work_server.hpp"
#include "crc_thread.hpp"

#include <thread>
#include <mutex>
#include <atomic>

/**
*	main server which manage WorkServer
*/
class CRCBossServer : public CRCINetEvent
{
private:
	// thread
	CRCThread _crcThread;
	// WorkServers
	std::vector<CRCWorkServerPtr> _workServers;
	// lock
	std::mutex _mutex;

protected:
	// high resolution timers
	CRCTimestamp _tTime;

protected:
	// count while client receive message
	std::atomic_int _recvCount;
	// count while client message comes
	std::atomic_int _msgCount;
	// count while client join or offline
	std::atomic_int _clientCount;
	// local socket
	SOCKET _sock;

public:
	CRCBossServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_clientCount = 0;
		_msgCount = 0;
	}

	virtual ~CRCBossServer()
	{
		_workServers.clear();

		Close();
	}

public:
	// initialize socket
	int InitSocket()
	{
		if (INVALID_SOCKET != _sock)
		{
			printf("close previous connection socket<%d>\n", (int)_sock);
			Close();
		}

#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(ver, &data);
#endif
		// 1 create socket
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (SOCKET_ERROR == _sock)
		{
			printf("create socket failure.\n");
			return _sock;
		}
		printf("socket<%d> create success.\n", (int)_sock);
		return _sock;
	}

	// bind ip and port
	int Bind(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}

		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		if (ip)
		{
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip)
		{
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (ret == SOCKET_ERROR)
		{
			printf("socket<%d> bind failure because port used by other.\n", (int)_sock);
			Close();
			return ret;
		}
		printf("socket<%d> bind port<%d> success.\n", (int)_sock, port);
		return ret;
	}

	// listen port
	int Listen(int n=5)
	{
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("socket<%d> listen failure.\n", (int)_sock);
			Close();
			return ret;
		}
		printf("socket<%d> listenning ... \n", (int)_sock);
		return ret;
	}

	// accept client's connectin
	SOCKET Accept()
	{
		sockaddr_in client_addr = {};
		int client_addr_len = sizeof(sockaddr_in);	// note: accept will fall without sizeof(sockaddr_in)
		SOCKET sock_client = INVALID_SOCKET;

#ifdef _WIN32
		sock_client = accept(_sock, (sockaddr*)&client_addr, &client_addr_len);
#else
		sock_client = accept(_sock, (sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
#endif
		if (INVALID_SOCKET == sock_client)
		{
			printf("socket<%d> accept a invalid client request.\n", (int)_sock);
			return sock_client;
		}

		// assign comed client to WorkServer with the least number of client
		CRCChannelPtr cp(new CRCChannel(sock_client));
		addClient2WorkServer(cp);
		//addClient2WorkServer(std::make_shared<Channel>(sock_client));
		//// get client ip address
		//inet_ntoa(client_addr.sin_addr)

		return sock_client;
	}

	// start WorkServer
	void Start(int nWorkServer)
	{
		for (int n = 0; n < nWorkServer; n++)
		{
			CRCWorkServerPtr pWorkServer = std::make_shared<CRCWorkServer>(_sock);
			_workServers.push_back(pWorkServer);
			pWorkServer->RegisterNetEventListener(this);
			pWorkServer->Start();
		}

		_crcThread.Start
		(
			nullptr,
			[this](CRCThread* pCrcThread) { OnRun(pCrcThread); },
			nullptr
		);

	}

	// select WorkServer which queue is smallest add client message to it
	void addClient2WorkServer(CRCChannelPtrRef pChannel)
	{
		auto pMinWorkServer = _workServers[0];
		for (auto pWorkServer : _workServers)
		{
			if (pMinWorkServer->getClientCount() > pWorkServer->getClientCount()) {
				pMinWorkServer = pWorkServer;
			}
		}
		pMinWorkServer->addClient(pChannel);
		OnJoin(pChannel);
	}

	// close socket
	void Close()
	{
		if (_sock == INVALID_SOCKET) return;

#ifdef _WIN32
		closesocket(_sock);
		WSACleanup();
#else
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
		if (_crcThread.IsRun())_crcThread.Close();
		printf("server is shutdown\n");
	}

	// if is running
	bool IsRunning()
	{
		return _sock != INVALID_SOCKET;
	}

	// statistics speed with packages / second
	void time4Msg()
	{
		// speed of server receiving client data packet
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			printf("threads<%d>,time<%lf>,socket<%d> clients<%d>,recv<%d>,msg<%d>\n", (int)_workServers.size(), t1, (int)_sock, _clientCount.load(), (int)(_recvCount / t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}

protected:
	// only process accept request
	void OnRun(CRCThread* pCrcThread)
	{
		printf("BossServer thread start...\n");

		while (pCrcThread->IsRun())
		{
			time4Msg();

			fd_set fd_read;
			//fd_set fd_write;
			//fd_set fd_exception;

			FD_ZERO(&fd_read);
			//FD_ZERO(&fd_write);
			//FD_ZERO(&fd_exception);

			// put server's socket in all the fd_set
			FD_SET(_sock, &fd_read);
			//FD_SET(_sock, &fd_write);
			//FD_SET(_sock, &fd_exception);

			//nfds is range of fd_set, not fd_set's count.
			//nfds is also max value+1 of all the file descriptor(socket).
			//nfds can be 0 in the windows.
			//that timeval was setted null means blocking, not null means nonblocking.
			timeval t = { 0,1 };
			//int ret = select(_sock + 1/*nfds*/, &fd_read, &fd_write, &fd_exception, &t);
			// only check readable
			int ret = select(_sock + 1/*nfds*/, &fd_read, nullptr, nullptr, &t);
			if (ret < 0)
			{
				printf("BossServer socket<%d> error occurs while select and mission finish.\n", (int)_sock);
				pCrcThread->ExitInSelfThread();
				break;
			}

			if (FD_ISSET(_sock, &fd_read))
			{
				FD_CLR(_sock, &fd_read);

				Accept();
			}
		}

		printf("BossServer thread exit...\n");
	}

	// inherit INetEvent
public:
	// it would only be triggered by one thread, safe
	virtual void OnLeave(CRCChannelPtrRef pChannel)
	{
		_clientCount--;
	}

	// multiple thread triggering, not safe
	virtual void OnNetMessage(CRCWorkServer* pWorkServer, CRCChannelPtrRef pChannel, CRCDataHeader* pheader)
	{
		_msgCount++;
	}

	// multiple thread triggering, not safe
	virtual void OnJoin(CRCChannelPtrRef pChannel)
	{
		_clientCount++;
	}

	// multiple thread triggering, not safe
	virtual void OnNetRecv(CRCChannelPtrRef pChannel)
	{
		_recvCount++;
	}
};


#endif