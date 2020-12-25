#ifndef _WORKSERVER_HPP_
#define _WORKSERVER_HPP_

#include "Init.h"
#include "Channel.hpp"
#include "INetEvent.hpp"

#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

/**
*	resposible for process client message
*/
class WorkServer
{
private:
	// local socket
	SOCKET _sock;
	// client sockets
	//std::vector<Channel*> _clients;
	std::map<SOCKET, ChannelPtr> _clients;
	// client socket buffer
	std::vector<ChannelPtr> _clientsBuf;
	// lock
	std::mutex _mutex;
	// thread handle
	std::thread* _pThread;
	// register event
	INetEvent* _pNetEvent;
	// fd backup
	fd_set _fd_read_bak;
	bool _clients_change;
	// max socket descriptor
	SOCKET _max_socket;
	// task 
	TaskServer _taskServer;
	// whether stop
	bool _isStop;
	// used to timing heart beat
public:
	WorkServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pThread = nullptr;
		_pNetEvent = nullptr;
		_isStop = false;
	}
	~WorkServer()
	{
		printf("WorkServer destory.\n");
		//delete _pThread;
		Close();
	}

protected:
	// process net message
	int OnRun()
	{
		int ret = 1;

		printf("WorkServer thread start...\n");

		_clients_change = true;

		while (IsRunning())
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

			//nfds is range of fd_set, not fd_set's count.
			//nfds is also max value+1 of all the file descriptor(socket).
			//nfds can be 0 in the windows.
			//that timeval was setted null means blocking, not null means nonblocking.
			timeval t = { 0,1 };
			ret = select(_max_socket + 1/*nfds*/, &fd_read, &fd_write, &fd_exception, &t);
			if (ret < 0)
			{
				printf("socket<%d> error occurs while select and mission finish.\n", (int)_sock);
				Close();
				break;
			}
			//else if (0 == ret)
			//{
			//	continue;
			//}

			ReadData(fd_read);
			CheckTime();
		} // while (IsRunning())

		printf("WorkServer thread exit...\n");
		return ret;
	}

	void CheckTime()
	{
		time_t tNow = Time::getNowInMilliSec();

		for (auto iter = _clients.begin(); iter != _clients.end();)
		{
			// save current iterator;
			auto iterOld = iter++;

			// check heart beat
			if (!iterOld->second->check_heart_beat(tNow))
			{
				_clients_change = true;
				if (_pNetEvent)
				{
					_pNetEvent->OnLeave(iterOld->second);
				}
				closesocket(iterOld->first);
				_clients.erase(iterOld->first);
				continue;
			}

			// regulary check to decide whether to send data to client
			int ret = iterOld->second->timing_send(tNow);
			if (SOCKET_ERROR == ret)
			{
				printf("timing send data to client (socket<%d>) failed. ret<%d>\n", (int)iterOld->first, ret);
			}
		}
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
					_clients_change = true;
					if (_pNetEvent)
					{
						_pNetEvent->OnLeave(iter->second);
					}
					closesocket(iter->first);
					_clients.erase(iter->first);
				}
			}
			else
			{
				printf("incredible situation occurs while client offline\n");
			}

		}
#else
		std::vector<ChannelPtr> temp;
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
					close(iter->first);
				}
			}
		}

		for (auto pClient : temp)
		{
			_clients.erase(pClient->sockfd());
		}
#endif
	}

public:
	void addClient(ChannelPtrRef pChannel)
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
	// stop cellserver
	void Stop()
	{
		_isStop = true;
	}

	// if is running
	bool IsRunning()
	{
		return _sock != INVALID_SOCKET && !_isStop;
	}

	// start self
	void Start()
	{
		_pThread = new std::thread(std::mem_fn(&WorkServer::OnRun), this);
		_taskServer.Start();
	}

	// receive data, deal sticking package and splitting package
	int RecvData(ChannelPtrRef pclient)
	{
		char* szRecv = pclient->recvBuf() + pclient->GetLastRecvPos();
		//receive client data
		//int nLen = recv(pclient->sockfd(), _szRecvBuffer, RECV_BUFFER_SIZE, 0);
		int nLen = recv(pclient->sockfd(), szRecv, RECV_BUFFER_SIZE - pclient->GetLastRecvPos(), 0);

		_pNetEvent->OnNetRecv(pclient);

		if (nLen <= 0)
		{
			//printf("server socket<%d> client socket <%d> offline\n", (int)_sock, (int)pclient->sockfd());
			return -1;
		}

		// reset heart beat timing in here or in MyServer::OnNetMessage or in the all of these two place
		//pclient->reset_dt_heart();

		// copy receive buffer data to message buffer
		//memcpy(pclient->msgBuf() + pclient->GetLastPos(), _szRecvBuffer, nLen);

		// update end position of message buffer
		pclient->SetLastRecvPos(pclient->GetLastRecvPos() + nLen);

		// whether message buffer size greater than message header(DataHeader)'s size,
		// if yes, converting message buffer to struct DataHeader and clear message buffer
		// had prcessed.
		while (pclient->GetLastRecvPos() >= sizeof(DataHeader))
		{
			// convert message buffer to DataHeader
			DataHeader* pheader = (DataHeader*)pclient->recvBuf();
			// whether message buffer size greater than current client message size,
			if (pclient->GetLastRecvPos() >= pheader->data_length)
			{
				// processed message's length
				int nClientMsgLen = pheader->data_length;
				// length of message buffer which was untreated
				int nSize = pclient->GetLastRecvPos() - nClientMsgLen;
				// process net message
				OnNetMessage(pclient, pheader);
				// earse processed message buffer
				memcpy(pclient->recvBuf(), pclient->recvBuf() + nClientMsgLen, nSize);
				// update end position of message buffer
				pclient->SetLastRecvPos(nSize);
			}
			else
			{
				// length of message buffer which was untreated less than 
				// length of message header(DataHeader)'s size
				break;
			}
		}

		return 0;
	}

	// response net message
	virtual void OnNetMessage(ChannelPtrRef pChannel, DataHeader* pheader)
	{
		// statistics speed of server receiving client data packet
		_pNetEvent->OnNetMessage(this, pChannel, pheader);
	}

	// close socket
	void Close()
	{
		if (_sock == INVALID_SOCKET) return;

#ifdef _WIN32
		for (auto iter : _clients)
		{
			closesocket(iter.first);
			//delete iter.second;
		}
		closesocket(_sock);
#else
		for (auto iter : _clients)
		{
			close(iter.first);
			//delete iter.second;
		}
		close(_sock);
#endif
		_clients.clear();

		_sock = INVALID_SOCKET;

		printf("cell server is shutdown\n");
	}

public:
	void RegisterNetEventListener(INetEvent * pNetEvent)
	{
		_pNetEvent = pNetEvent;
	}

	// for task
public:
	void addSendTask(ChannelPtr pChannel, DataHeaderPtr pDataHeader)
	{
		//std::shared_ptr<ITask> pTask = std::make_shared<WorkServerSend2ClientTask>(pChannel, pDataHeader);
		_taskServer.addTask
		(
			[pChannel, pDataHeader]() {pChannel->SendData(pDataHeader); }
		);
	}
};



#endif