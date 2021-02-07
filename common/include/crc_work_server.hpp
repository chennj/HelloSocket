#ifndef _CRC_WORK_SERVER_HPP_
#define _CRC_WORK_SERVER_HPP_

#include "crc_channel.hpp"
#include "crc_thread.hpp"
#include "crc_init.h"
#include "crc_inet_event.hpp"
#include "crc_task_server.hpp"

#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

/**
*	resposible for process client message
*/
class CRCWorkServer
{
private:
	// client socket buffer
	std::vector<CRCChannel*> _clientsBuf;
	// lock
	std::mutex _mutex;
	// thread handle
	CRCThread _crcThread;
	// register event
	CRCINetEvent* _pNetEvent;
	// task 
	CRCTaskServer _taskServer;
	// timer
	CRCTimestamp _tTime;

protected:
	// local socket
	SOCKET _sock;
	// client change such as offine.
	bool _clients_change;
	// client sockets
	//std::vector<Channel*> _clients;
	std::map<SOCKET, CRCChannel*> _clients;
public:
	CRCWorkServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pNetEvent = nullptr;
		_tTime.update();
	}
	virtual ~CRCWorkServer()
	{
		CRCLogger::info("WorkServer destory.\n");
		// release resource
		Close();
	}

protected:
	// select mode
	virtual int DoAction() = 0;

	// process net message
	int OnRun(CRCThread* pCrcThread)
	{
		int ret = 1;

		CRCLogger_Info("WorkServer thread start...\n");

		_clients_change = true;

		while (pCrcThread->IsRun())
		{
			if (_clientsBuf.size() > 0)
			{
				std::lock_guard<std::mutex> lockGuard(_mutex);
				for (auto pClient : _clientsBuf)
				{
					//_clients.push_back(pClient);
					_clients[pClient->sockfd()] = pClient;
					OnClientJoin(pClient);
				}
				_clientsBuf.clear();
				_clients_change = true;
			}

			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			// 定时检测心跳，或发送数据
			CheckTime();

			if (DoAction() < 0)
			{
				pCrcThread->ExitInSelfThread();
				break;
			}
		} // while (IsRunning())

		CRCLogger::info("WorkServer thread exit...\n");
		return ret;
	}

	void CheckTime()
	{
		time_t tNow = CRCTime::getNowInMilliSec();

		for (auto iter = _clients.begin(); iter != _clients.end();)
		{
			// save current iterator;
			auto iterOld = iter++;
			CRCChannel* pChannel = iterOld->second;

			// check heart beat
			if (!pChannel->check_heart_beat(tNow))
			{
#ifdef CRC_USE_IOCP
				if (pChannel->hasDeliveryIoAction())
				{
					pChannel->destory();
					_clients.erase(iterOld);
				}
				else
				{
					OnClientLeave(iterOld);
				}
#else
				OnClientLeave(iterOld);
#endif
				continue;
			}

			/**
			*
			*	regulary check to decide whether to send data to client
			*
			*/
			// synchronization execute, need add lock, will reduce sending speed
			//int ret = iterOld->second->timing_send(tNow);
			//if (SOCKET_ERROR == ret)
			//{
			//	CRCLogger::info("timing send data to client (socket<%d>) failed. ret<%d>\n", (int)iterOld->first, ret);
			//}

			// check if the timing sending data time is up
			// asynchronous execute, need not add lock
			//if (iterOld->second->check_timing_send(tNow))
			//{
			//	CRCChannelPtr pChannel = iterOld->second;
			//	_taskServer.addTask
			//	(
			//		[pChannel]() {pChannel->SendDataIM(); }
			//	);
			//}
		}
	}

	void OnClientLeave(std::map<SOCKET, CRCChannel*>::iterator iter)
	{
		_clients_change = true;
		if (_pNetEvent)
		{
			_pNetEvent->OnLeave(iter->second);
		}
		if (INVALID_SOCKET != iter->first)
			_clients.erase(iter->first);
		delete iter->second;
	}

	void OnClientLeave(CRCChannel* pChannel)
	{
		_clients_change = true;
		if (_pNetEvent)
		{
			_pNetEvent->OnLeave(pChannel);
		}
		if (INVALID_SOCKET != pChannel->sockfd())
			_clients.erase(pChannel->sockfd());
		delete pChannel;
	}
#ifdef CRC_USE_IOCP
	void OnClientLeave(IO_EVENT& ioEvent)
	{
		CRCChannel* pChannel = (CRCChannel*)ioEvent.data.ptr;
		if (!pChannel)return;
		if (_pNetEvent)
		{
			_pNetEvent->OnLeave(pChannel);
		}
		if (INVALID_SOCKET != pChannel->sockfd())
			_clients.erase(pChannel->sockfd());
		delete pChannel;
	}
#endif

public:
	void addClient(CRCChannel* pChannel)
	{
		// self unlocking
		std::lock_guard<std::mutex> lockGuard(_mutex);
		//_mutex.lock();
		_clientsBuf.push_back(pChannel);
		//_mutex.unlock();
	}

	size_t getClientCount()
	{
		return _clients.size() + _clientsBuf.size();
	}

public:
	// if is running
	bool IsRunning()
	{
		return _sock != INVALID_SOCKET;
	}

	// start self
	void Start()
	{
		_crcThread.Start
		(
			// onCreate
			nullptr,
			// onRun
			[this](CRCThread* pCrcThread) {OnRun(pCrcThread); },
			// onDestory
			[this](CRCThread* pCrcThread) {ClearClients(pCrcThread); }
		);
		_taskServer.Start();
	}

	// receive data, deal sticking package and splitting package
	int RecvData(CRCChannel* pClient)
	{
		//receive client data
		int nLen = pClient->RecvData();
		if (nLen <= 0)
		{
			//CRCLogger::info("server socket<%d> client socket <%d> offline\n", (int)_sock, (int)pclient->sockfd());
			return -1;
		}

		// trigger event while received net message
		_pNetEvent->OnNetRecv(pClient);

		// reset heart beat timing in here or in CRCServer::OnNetMessage or in the all of these two place
		//pclient->reset_dt_heart();

		// loop proccess message
		while (pClient->HasMessage())
		{
			// process message
			OnNetMessage(pClient, pClient->front_message());
			// remove one message from buffer
			pClient->pop_front_message();
		}

		return 0;
	}

	void DoMessage()
	{
		CRCChannel* pChannel = nullptr;
		for (auto iter : _clients)
		{
			pChannel = iter.second;
			OnNetRecevie(pChannel);
			while (pChannel->HasMessage())
			{
				OnNetMessage(pChannel, pChannel->front_message());
				pChannel->pop_front_message();
			}
		}
	}

	void OnNetRecevie(CRCChannel* pChannel)
	{
		if (_pNetEvent)
			_pNetEvent->OnNetRecv(pChannel);
	}

	// response net message
	virtual void OnNetMessage(CRCChannel* pChannel, CRCDataHeader* pheader)
	{
		// statistics speed of server receiving client data packet
		if (_pNetEvent)
			_pNetEvent->OnNetMessage(this, pChannel, pheader);
	}

	// close socket
	void Close()
	{
		if (_sock == INVALID_SOCKET) return;
		_sock = INVALID_SOCKET;
		// close task server
		_taskServer.Close();
		// close thread
		if (_crcThread.IsRun())_crcThread.Close();
	}

	// clear resource
	void ClearClients(CRCThread* pCrcThread)
	{
		_clients.clear();
		_clientsBuf.clear();
	}

public:
	void RegisterNetEventListener(CRCINetEvent * pNetEvent)
	{
		_pNetEvent = pNetEvent;
	}

	// for task
public:
	void addSendTask(CRCChannel* pChannel, CRCDataHeader* pDataHeader)
	{
		//std::shared_ptr<ITask> pTask = std::make_shared<WorkServerSend2ClientTask>(pChannel, pDataHeader);
		_taskServer.addTask
		(
			[pChannel, pDataHeader]() {pChannel->SendData(pDataHeader); }
		);
	}

protected:
	virtual void OnClientJoin(CRCChannel* pClient)
	{
	}

public:
	static void make_reuseaddr(SOCKET clientSock)
	{
#ifdef _WIN32
		linger so_linger = { 1,2 };
		int bOptLen	 = sizeof(linger);
		int ret = setsockopt(clientSock, SOL_SOCKET, SO_LINGER, (char *)&so_linger, bOptLen);
		if (ret == SOCKET_ERROR) {
			CRCLogger_Error("WorkServer::make_reuseaddr SO_LINGER failed with error=%u\n", WSAGetLastError());
		}
#else
		int reuse = 0;
		int ret = setsockopt(clientSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(int));
		if (ret == SOCKET_ERROR) {
			CRCLogger_Error("WorkServer::make_reuseaddr SO_REUSEADDR failed ");
		}
#endif
	}

	static void destory_socket(SOCKET clientSock)
	{
		//make_reuseaddr(clientSock);
#ifdef _WIN32
		closesocket(clientSock);
#else
		close(clientSock);
#endif
	}
};



#endif