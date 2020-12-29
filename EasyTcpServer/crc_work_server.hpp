#ifndef _CRC_WORK_SERVER_HPP_
#define _CRC_WORK_SERVER_HPP_

#include "crc_init.h"
#include "crc_channel.hpp"
#include "crc_inet_event.hpp"
#include "crc_thread.hpp"

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
	// local socket
	SOCKET _sock;
	// client sockets
	//std::vector<Channel*> _clients;
	std::map<SOCKET, CRCChannelPtr> _clients;
	// client socket buffer
	std::vector<CRCChannelPtr> _clientsBuf;
	// lock
	std::mutex _mutex;
	// thread handle
	CRCThread _crcThread;
	// register event
	CRCINetEvent* _pNetEvent;
	// fd backup
	fd_set _fd_read_bak;
	bool _clients_change;
	// max socket descriptor
	SOCKET _max_socket;
	// task 
	CRCTaskServer _taskServer;
	// timer
	CRCTimestamp _tTime;
public:
	CRCWorkServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pNetEvent = nullptr;
		_tTime.update();
	}
	~CRCWorkServer()
	{
		printf("WorkServer destory.\n");
		// release resource
		Close();
	}

protected:
	// process net message
	int OnRun(CRCThread* pCrcThread)
	{
		int ret = 1;

		printf("WorkServer thread start...\n");

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

			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			fd_set fd_read;
			fd_set fd_write;
			fd_set fd_exception;

			FD_ZERO(&fd_read);
			FD_ZERO(&fd_write);
			FD_ZERO(&fd_exception);

			if (_clients_change)
			{
				_clients_change = false;

				_max_socket = _clients.begin()->first;
				for (auto iter : _clients)
				{
					_max_socket = iter.first > _max_socket ? iter.first : _max_socket;
					// the following statement needs to be optimized,
					// because it takes up a lot of CPU
					FD_SET(iter.first, &fd_read);
				}
				memcpy(&_fd_read_bak, &fd_read, sizeof(fd_set));
			}
			else
			{
				memcpy(&fd_read, &_fd_read_bak, sizeof(fd_set));
			}

			memcpy(&fd_write, &_fd_read_bak, sizeof(fd_set));
			memcpy(&fd_exception, &_fd_read_bak, sizeof(fd_set));

			//nfds is range of fd_set, not fd_set's count.
			//nfds is also max value+1 of all the file descriptor(socket).
			//nfds can be 0 in the windows.
			//that timeval was setted null means blocking, not null means nonblocking.
			timeval t = { 0,1 };
			ret = select(_max_socket + 1/*nfds*/, &fd_read, &fd_write, &fd_exception, &t);
			if (ret < 0)
			{
				printf("WorkServer socket<%d> error occurs while select and mission finish.\n", (int)_sock);
				pCrcThread->ExitInSelfThread();
				break;
			}
			//else if (0 == ret)
			//{
			//	continue;
			//}

			ReadData(fd_read);
			WriteData(fd_write);
			WriteData(fd_exception);
#ifdef _WIN32
			if (_tTime.getElapsedSecond() >= 1.0)
			{
				if (fd_exception.fd_count > 0)
				{
					xPrintf("###>>>WorkServer exception<%d>\n", fd_exception.fd_count);
				}
				xPrintf("WorkServer readable<%d>, writable<%d>\n", fd_read.fd_count, fd_write.fd_count);
				_tTime.update();
			}

#endif
			CheckTime();
		} // while (IsRunning())

		printf("WorkServer thread exit...\n");
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
			*/
			// synchronization execute, need add lock, will reduce sending speed
			//int ret = iterOld->second->timing_send(tNow);
			//if (SOCKET_ERROR == ret)
			//{
			//	printf("timing send data to client (socket<%d>) failed. ret<%d>\n", (int)iterOld->first, ret);
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

	void WriteData(fd_set& fd_write)
	{
#ifdef _WIN32
		for (size_t n = 0; n < fd_write.fd_count; n++)
		{
			auto iter = _clients.find(fd_write.fd_array[n]);
			if (iter != _clients.end())
			{
				if (-1 == iter->second->SendDataIM())
				{
					OnClientLeave(iter);
				}
			}
			else
			{
				printf("incredible situation occurs while write data to client\n");
			}

		}
#else
		std::vector<CRCChannelPtr> temp;
		for (auto iter : _clients)
		{
			if (FD_ISSET(iter.first, &fd_write))
			{
				if (-1 == RecvData(iter.second))
				{
					_clients_change = true;
					temp.push_back(iter.second);

					if (_pNetEvent)
					{
						_pNetEvent->OnLeave(iter.second);
					}
				}
			}
		}
		for (auto pClient : temp)
		{
			_clients.erase(pClient->sockfd());
		}
#endif

	}

	void ReadData(fd_set& fd_read)
	{
#ifdef _WIN32
		for (size_t n = 0; n < fd_read.fd_count; n++)
		{
			auto iter = _clients.find(fd_read.fd_array[n]);

			if (iter != _clients.end())
			{
				if (-1 == RecvData(iter->second))
				{
					OnClientLeave(iter);
				}
			}
			else
			{
				printf("incredible situation occurs while read data from client\n");
			}

		}
#else
		std::vector<CRCChannelPtr> temp;
		for (auto iter : _clients)
		{
			if (FD_ISSET(iter.first, &fd_read))
			{
				if (-1 == RecvData(iter.second))
				{
					_clients_change = true;
					temp.push_back(iter.second);

					if (_pNetEvent)
					{
						_pNetEvent->OnLeave(iter.second);
					}
				}
			}
		}
		for (auto pClient : temp)
		{
			_clients.erase(pClient->sockfd());
		}
#endif
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
			//printf("server socket<%d> client socket <%d> offline\n", (int)_sock, (int)pclient->sockfd());
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