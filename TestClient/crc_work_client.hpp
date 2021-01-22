#ifndef _CRC_WORK_CLIENT_HPP_
#define _CRC_WORK_CLIENT_HPP_

#include "crc_init.h"
#include "crc_net_environment.hpp"
#include "../common/include/crc_channel.hpp"

class CRCWorkClient
{
protected:
	//
	bool _isConnected;
	//
	CRCChannel* _pChannel;

public:
	CRCWorkClient()
	{
		_pChannel = nullptr;
		_isConnected = false;
	}

	virtual ~CRCWorkClient()
	{
		Close();
	}

public:
	// create socket
	int InitSocket()
	{
		CRCNetEnvironment::init();

		if (_pChannel)
		{
			printf("close previous connection<socket=%d>\n", (int)_pChannel->sockfd());
			Close();
		}

		// 1 create socket
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (SOCKET_ERROR == sock)
		{
			printf("create socket failure.\n");
			return -1;
		}

		_pChannel = new CRCChannel(sock);

		return 0;
	}

	// connect server
	int Connect(const char * ip, unsigned short port)
	{
		if (!_pChannel)
		{
			InitSocket();
		}

		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_pChannel->sockfd(), (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("connect server failure...\n");
			return ret;
		}
		
		_isConnected = true;
		return ret;
	}

	// close socket
	void Close()
	{
		if (!_pChannel) return;
	
		delete _pChannel;
		_pChannel = nullptr;
		_isConnected = false;
	}

	//deal net message
	int OnRun()
	{
		if (!IsRunning()) return -1;

		SOCKET sock = _pChannel->sockfd();

		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(sock, &fdRead);

		fd_set fdWrite;
		FD_ZERO(&fdWrite);
		timeval t = { 0,0 };
		int ret;
		if (_pChannel->is_need_write())
		{
			FD_SET(sock, &fdWrite);
			ret = select(sock + 1, &fdRead, &fdWrite, nullptr, &t);
		}
		else
		{
			ret = select(sock + 1, &fdRead, nullptr, nullptr, &t);
		}
		 
		if (ret < 0)
		{
			printf("error occurs while listen server\n");
			Close();
			return ret;
		}

		if (FD_ISSET(sock, &fdRead))
		{
			int ret = RecvData();
			if (-1 == ret)
			{
				Close();
				return ret;
			}
		}

		if (FD_ISSET(sock, &fdWrite))
		{
			int ret = _pChannel->SendDataIM();
			if (-1 == ret)
			{
				Close();
				return ret;
			}
		}

		return 0;
	}

	bool IsRunning()
	{
		return _pChannel && _isConnected;
	}

	/*
	 *	1. receive data
	 *	2. process sticking package
	 *	3. receive splitting package
	*/
	int RecvData()
	{
		if (!IsRunning())
			return -1;

		//receive client data
		int nLen = _pChannel->RecvData();
		if (nLen <= 0)
		{
			return -1;
		}

		// loop proccess message
		while (_pChannel->HasMessage())
		{
			// process message
			OnNetMessage(_pChannel->front_message());
			// remove one message from buffer
			_pChannel->pop_front_message();
		}

		return nLen;
	}

	int SendData(const CRCDataHeader* pHeader)
	{
		if (IsRunning())
			return _pChannel->SendDataBuffer(pHeader);
		return 0;
	}

	int SendData(const char* pData, int nLen)
	{
		if (IsRunning())
			return _pChannel->SendDataBuffer(pData, nLen);
		return 0;
	}

	virtual void OnNetMessage(CRCDataHeader* header) = 0;
};

#endif