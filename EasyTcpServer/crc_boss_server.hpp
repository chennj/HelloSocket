#ifndef _CRC_BOSS_SERVER_HPP_
#define _CRC_BOSS_SERVER_HPP_

#include "crc_init.h"
#include "crc_inet_event.hpp"
#include "crc_work_select_server.hpp"
#include "crc_work_epoll_server.hpp"
#include "crc_net_environment.hpp"
#include "../common/include/crc_thread.hpp"
#include "../common/include/crc_logger.hpp"

#include <thread>
#include <mutex>
#include <atomic>

/**
*	main server which manage WorkServer
*/
template<class T,class T_PTR>
class CRCBossServer : public CRCINetEvent
{
private:
	// thread
	CRCThread _crcThread;
	// WorkServers
	std::vector<T_PTR> _workServers;
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
		Close();
	}

public:
	// initialize socket
	int InitSocket()
	{
		CRCNetEnvironment::init();

		if (INVALID_SOCKET != _sock)
		{
			CRCLogger::info("close previous connection socket<%d>\n", (int)_sock);
			Close();
		}

		// 1 create socket
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (SOCKET_ERROR == _sock)
		{
			CRCLogger::info("create socket failure.\n");
			return _sock;
		}
		CRCLogger::info("socket<%d> create success.\n", (int)_sock);
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
			CRCLogger::info("socket<%d> bind failure because port used by other.\n", (int)_sock);
			Close();
			return ret;
		}
		CRCLogger::info("socket<%d> bind port<%d> success.\n", (int)_sock, port);
		return ret;
	}

	// listen port
	int Listen(int n=5)
	{
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			CRCLogger::info("socket<%d> listen failure.\n", (int)_sock);
			Close();
			return ret;
		}
		CRCLogger::info("socket<%d> listenning ... \n", (int)_sock);
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
			CRCLogger::info("socket<%d> accept a invalid client request.\n", (int)_sock);
			return sock_client;
		}

		// assign comed client to WorkServer with the least number of client
		addClient2WorkServer(new CRCChannel(sock_client));
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
			T_PTR pWorkServer = std::make_shared<T>(_sock);
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
	void addClient2WorkServer(CRCChannel* pChannel)
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
#else
		close(_sock);
#endif
		_sock = INVALID_SOCKET;

		_workServers.clear();

		if (_crcThread.IsRun())_crcThread.Close();
		CRCLogger::info("server is shutdown\n");
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
			CRCLogger::info("threads<%d>,time<%lf>,socket<%d> clients<%d>,recv<%d>,msg<%d>\n", (int)_workServers.size(), t1, (int)_sock, _clientCount.load(), (int)(_recvCount / t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}

protected:
	// only process accept request
	virtual void OnRun(CRCThread* pCrcThread) = 0;

	// inherit INetEvent
public:
	// it would only be triggered by one thread, safe
	virtual void OnLeave(CRCChannel* pChannel)
	{
		_clientCount--;
	}

	// multiple thread triggering, not safe
	virtual void OnNetMessage(CRCWorkServer* pWorkServer, CRCChannel* pChannel, CRCDataHeader* pheader)
	{
		_msgCount++;
	}

	// multiple thread triggering, not safe
	virtual void OnJoin(CRCChannel* pChannel)
	{
		_clientCount++;
	}

	// multiple thread triggering, not safe
	virtual void OnNetRecv(CRCChannel* pChannel)
	{
		_recvCount++;
	}
};


#endif