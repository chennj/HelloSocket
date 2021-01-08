#ifndef _CRC_WORK_SERVER_HPP_
#define _CRC_WORK_SERVER_HPP_

#include "../common/include/crc_channel.hpp"
#include "../common/include/crc_thread.hpp"
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
	std::vector<CRCChannelPtr> _clientsBuf;
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
	std::map<SOCKET, CRCChannelPtr> _clients;
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

		CRCLogger::info("WorkServer thread start...\n");

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
				}
				_clientsBuf.clear();
				_clients_change = true;
			}

			// 定时检测心跳，或发送数据
			CheckTime();

			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

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

			// check heart beat
			if (!iterOld->second->check_heart_beat(tNow))
			{
				OnClientLeave(iterOld);
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

	void OnClientLeave(std::map<SOCKET, CRCChannelPtr>::iterator iter)
	{
		_clients_change = true;
		if (_pNetEvent)
		{
			_pNetEvent->OnLeave(iter->second);
		}
		_clients.erase(iter->first);
	}

public:
	void addClient(CRCChannelPtrRef pChannel)
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
	int RecvData(CRCChannelPtrRef pClient)
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

	// response net message
	virtual void OnNetMessage(CRCChannelPtrRef pChannel, CRCDataHeader* pheader)
	{
		// statistics speed of server receiving client data packet
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
	void addSendTask(CRCChannelPtr pChannel, CRCDataHeaderPtr pDataHeader)
	{
		//std::shared_ptr<ITask> pTask = std::make_shared<WorkServerSend2ClientTask>(pChannel, pDataHeader);
		_taskServer.addTask
		(
			[pChannel, pDataHeader]() {pChannel->SendData(pDataHeader); }
		);
	}
};



#endif