#ifndef _EASYTCPSERVER_HPP_
#define _EASYTCPSERVER_HPP_

#ifdef _WIN32
#ifndef FD_SETSIZE
#define FD_SETSIZE      2506			//windows default FD_SETSIZE equals 64, too small
#endif
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include <string.h>

#define SOCKET int
#define INVALID_SOCKET	(SOCKET)(~0)
#define SOCKET_ERROR			(-1)
#endif

#include<stdio.h>
#include<map>
#include<vector>
#include<thread>
#include<mutex>
#include<atomic>
#include"MessageHeader.hpp"
#include"CellTimestamp.hpp"
#include"Task.hpp"

// minimum buffer size
#ifndef RECV_BUFFER_SIZE
#define RECV_BUFFER_SIZE 1024*10*5
#endif

#ifndef SEND_BUFFER_SIZE
#define SEND_BUFFER_SIZE 1024*10*5
#endif

class CellServer;

/**
*	client object with socket
*/
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szRecvBuffer, 0, RECV_BUFFER_SIZE);
		_lastRecvPos = 0;
		memset(_szSendBuffer, 0, SEND_BUFFER_SIZE);
		_lastSendPos = 0;
	}

	virtual ~ClientSocket()
	{
		//delete[] (void*)_szRecvBuffer;
	}

public:
	SOCKET sockfd()
	{
		return _sockfd;
	}

	char * recvBuf()
	{
		return _szRecvBuffer;
	}

	int GetLastRecvPos()
	{
		return _lastRecvPos;
	}

	void SetLastRecvPos(int lastRecvPos)
	{
		_lastRecvPos = lastRecvPos;
	}

	char * sendBuf()
	{
		return _szSendBuffer;
	}

	int GetLastSendPos()
	{
		return _lastSendPos;
	}

	void SetLastSendPos(int lastSendPos)
	{
		_lastSendPos = lastSendPos;
	}
public:
	// send data
	int SendData(DataHeader * pheader)
	{
		int ret = SOCKET_ERROR;
		if (!pheader) {
			return ret;
		}
		// it's data length that would send
		int nSendLen = pheader->data_length;
		// it's data that would send
		const char* pSendData = (const char*)pheader;
		// 
		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFFER_SIZE)
			{
				// calculate the length of data that can be copied
				int nCanCopyLen = SEND_BUFFER_SIZE - _lastSendPos;
				// copy data to send buffer
				memcpy(_szSendBuffer + _lastSendPos, pSendData, nCanCopyLen);
				// position of remaining data
				pSendData += nCanCopyLen;
				// length of remaining data
				nSendLen -= nCanCopyLen;
				// send data
				ret = send(_sockfd/*client socket*/, _szSendBuffer, SEND_BUFFER_SIZE, 0);
				// set _lastSendPos to zero
				_lastSendPos = 0;
				// exception occur while send,such as client offline
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
			}
			else
			{
				// copy data to be sent to the end of the send buffer
				memcpy(_szSendBuffer + _lastSendPos, pSendData, nSendLen);
				// calculate the tail position of the send buffer
				_lastSendPos += nSendLen;
				// set return value
				ret = 0;
				// exit while
				break;
			}
		}

		return ret;
	}

private:
	// fd_set file descriptor
	SOCKET _sockfd;
	// recv message buffer
	char _szRecvBuffer[RECV_BUFFER_SIZE];
	// recv message buffer position
	int _lastRecvPos;

	// send message buffer
	char _szSendBuffer[SEND_BUFFER_SIZE];
	// send message buffer position
	int _lastSendPos;
};

/**
*	net event interface
*/
class INetEvent
{
public:
	/**
	*	event while client join
	*/
	virtual void OnJoin(ClientSocket* pClientSocket) = 0;

	/**
	*	event while client leaves
	*/
	virtual void OnLeave(ClientSocket* pClientSocket) = 0;

	/**
	*	event while client's message comes
	*/
	virtual void OnNetMessage(CellServer* pCellServer, ClientSocket* pClientSocket, DataHeader* pheader) = 0;

	/**
	*	event while client's receive message
	*/
	virtual void OnNetRecv(ClientSocket* pClientSocket) = 0;
};

/**
*	resposible for send client message
*/
class CellSend2ClientTask : public ITask
{
private:
	ClientSocket* _pClientSocket;
	DataHeader* _pDataHeader;

public:
	CellSend2ClientTask(ClientSocket* pClientSocket, DataHeader* pDataHeader)
	{
		_pClientSocket = pClientSocket;
		_pDataHeader = pDataHeader;
	}

	~CellSend2ClientTask()
	{

	}

public:
	void doTask()
	{
		_pClientSocket->SendData(_pDataHeader);
		delete _pDataHeader;
	}
};

/**
*	resposible for process client message
*/
class CellServer
{
private:
	// local socket
	SOCKET _sock;
	// client sockets
	//std::vector<ClientSocket*> _clients;
	std::map<SOCKET,ClientSocket*> _clients;
	// client socket buffer
	std::vector<ClientSocket*> _clientsBuf;
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
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pThread = nullptr;
		_pNetEvent = nullptr;

	}
	~CellServer()
	{
		printf("CellServer destory.\n");
		delete _pThread;
		Close();
	}

protected:
	// process net message
	int OnRun()
	{
		int ret = 1;

		printf("CellServer thread start...\n");

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
			timeval t = { 0,0 };
			ret = select(_max_socket + 1/*nfds*/, &fd_read, &fd_write, &fd_exception, &t);
			if (ret < 0)
			{
				printf("socket<%d> error occurs while select and mission finish.\n", (int)_sock);
				Close();
				break;
			}
			else if (0 == ret)
			{
				continue;
			}

#ifdef _WIN32
			for (int n = 0; n < fd_read.fd_count; n++)
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
						_clients.erase(iter->first);
					}
				}
				else
				{
					printf("incredible situation occurs while client offline\n");
				}

			}
#else
			std::vector<ClientSocket*> temp;
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
				delete pClient;
			}
#endif
		} // while (IsRunning())

		printf("CellServer thread exit...\n");
		return ret;
	}

public:
	void addClient(ClientSocket* pClientSocket)
	{
		// self unlocking
		std::lock_guard<std::mutex> lockGuard(_mutex);
		//_mutex.lock();
		_clientsBuf.push_back(pClientSocket);
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
		_pThread = new std::thread(std::mem_fn(&CellServer::OnRun), this);
		_taskServer.Start();
	}

	// receive data, deal sticking package and splitting package
	int RecvData(ClientSocket* pclient)
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
			DataHeader * pheader = (DataHeader*)pclient->recvBuf();
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
	virtual void OnNetMessage(ClientSocket* pClientSocket, DataHeader* pheader)
	{
		// statistics speed of server receiving client data packet
		_pNetEvent->OnNetMessage(this, pClientSocket, pheader);

		switch (pheader->cmd)
		{
		case CMD_LOGIN:
		{
			//Login* login = (Login*)pheader;
			//printf("socket<%d> receive client socket<%d> message: CMD_LOGIN , data length<%d>, user name<%s>, pwd<%s>\n", (int)_sock, (int)sock_client, pheader->data_length, login->username, login->password);

			//LoginResponse ret;
			//pClientSocket->SendData(&ret);
		}
		break;
		case CMD_LOGOUT:
		{
			//Logout* logout = (Logout*)pheader;
			//printf("socket<%d> receive client socket<%d> message: CMD_LOGOUT , data length<%d>, user name<%s>\n", (int)_sock, (int)sock_client, pheader->data_length, logout->username);

			//LogoutResponse ret;
			//pClientSocket->SendData(&ret);
		}
		break;
		default:
		{
			printf("socket<%d> receive client socket<%d> message: CMD_UNKNOWN , data length<%d>\n", (int)_sock, (int)pClientSocket->sockfd(), pheader->data_length);
			//UnknownResponse ret;
			//pClientSocket->SendData(&ret);
		}
		break;
		}
	}

	// close socket
	void Close()
	{
		if (_sock == INVALID_SOCKET) return;

#ifdef _WIN32
		for (auto iter : _clients)
		{
			closesocket(iter.first);
			delete iter.second;
		}
		closesocket(_sock);
#else
		for (auto iter : _clients)
		{
			close(iter.first);
			delete iter.second;
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
	void addSendTask(ClientSocket* pClientSocket, DataHeader* pDataHeader)
	{
		_taskServer.addTask(new CellSend2ClientTask(pClientSocket, pDataHeader));
	}
};

/**
*	main server which manage CellServer
*/
class EasyTcpServer : public INetEvent
{
private:
	// CellServers
	std::vector<CellServer*> _cellServers;
	// lock
	std::mutex _mutex;
	// high resolution timers
	CellTimestamp _tTime;

protected:
	// local socket
	SOCKET _sock;
	// count while client receive message
	std::atomic_int _recvCount;
	// count while client message comes
	std::atomic_int _msgCount;
	// count while client join or offline
	std::atomic_int _clientCount;

public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_clientCount = 0;
		_msgCount = 0;
	}

	virtual ~EasyTcpServer()
	{
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

		// assign comed client to CellServer with the least number of client
		addClient2CellServer(new ClientSocket(sock_client));
		//// get client ip address
		//inet_ntoa(client_addr.sin_addr)

		return sock_client;
	}

	// start CellServer
	void Start(int nCellServer)
	{
		for (int n = 0; n < nCellServer; n++)
		{
			auto pCellServer = new CellServer(_sock);
			_cellServers.push_back(pCellServer);
			pCellServer->RegisterNetEventListener(this);
			pCellServer->Start();
		}
	}

	// select CellServer which queue is smallest add client message to it
	void addClient2CellServer(ClientSocket* pClientSocket)
	{
		auto pMinCellServer = _cellServers[0];
		for (auto pCellServer : _cellServers)
		{
			if (pMinCellServer->getClientCount() > pCellServer->getClientCount()) {
				pMinCellServer = pCellServer;
			}
		}
		pMinCellServer->addClient(pClientSocket);
		OnJoin(pClientSocket);
	}

	// close socket
	void Close()
	{
		if (_sock == INVALID_SOCKET) return;

#ifdef _WIN32
		closesocket(_sock);
		// clean windows socket environment
		WSACleanup();
#else
		close(_sock);
#endif

		//for (auto pCellServer : _cellServers)
		//{
		//	delete pCellServer;
		//}
		//_cellServers.clear();

		_sock = INVALID_SOCKET;
		printf("server is shutdown\n");
	}

	// only process accept request
	int OnRun()
	{
		if (!IsRunning())
		{
			return -1;
		}

		time4Msg();

		fd_set fd_read;
		fd_set fd_write;
		fd_set fd_exception;

		FD_ZERO(&fd_read);
		FD_ZERO(&fd_write);
		FD_ZERO(&fd_exception);

		// put server's socket in all the fd_set
		FD_SET(_sock, &fd_read);
		FD_SET(_sock, &fd_write);
		FD_SET(_sock, &fd_exception);

		//nfds is range of fd_set, not fd_set's count.
		//nfds is also max value+1 of all the file descriptor(socket).
		//nfds can be 0 in the windows.
		//that timeval was setted null means blocking, not null means nonblocking.
		timeval t = { 0,1 };
		int ret = select(_sock + 1/*nfds*/, &fd_read, &fd_write, &fd_exception, &t);
		if (ret < 0)
		{
			printf("socket<%d> error occurs while select and mission finish.\n", (int)_sock);
			Close();
			return ret;
		}

		if (FD_ISSET(_sock, &fd_read))
		{
			FD_CLR(_sock, &fd_read);

			Accept();
		}

		return ret;
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
			printf("threads<%d>,time<%lf>,socket<%d> clients<%d>,recv<%d>,msg<%d>\n", _cellServers.size(), t1, (int)_sock, _clientCount.load(), (int)(_recvCount / t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}

	// inherit INetEvent
public:
	// it would only be triggered by one thread, safe
	virtual void OnLeave(ClientSocket* pClientSocket)
	{
		_clientCount--;
	}

	// multiple thread triggering, not safe
	virtual void OnNetMessage(CellServer* pCellServer, ClientSocket* pClientSocket, DataHeader* pheader)
	{
		_msgCount++;
	}

	// multiple thread triggering, not safe
	virtual void OnJoin(ClientSocket* pClientSocket)
	{
		_clientCount++;
	}

	// multiple thread triggering, not safe
	virtual void OnNetRecv(ClientSocket* pClientSocket)
	{
		_recvCount++;
	}
};


#endif