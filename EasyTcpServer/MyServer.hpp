#ifndef _MYSERVER_HPP_
#define _MYSERVER_HPP_

#include "Init.h"
#include "EasyTcpServer.hpp"

class MyServer : public EasyTcpServer
{
	// overwrite parent function
public:
	// multiple thread triggering, nosafe
	virtual void OnNetMessage(WorkServer* pWorkServer, ChannelPtrRef pChannel, DataHeader* pheader)
	{
		EasyTcpServer::OnNetMessage(pWorkServer, pChannel, pheader);

		switch (pheader->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)pheader;
			//printf("socket<%d> receive client socket<%d> message: CMD_LOGIN , data length<%d>, user name<%s>, pwd<%s>\n", (int)_sock, (int)sock_client, pheader->data_length, login->username, login->password);

			DataHeaderPtr ret = std::make_shared<LoginResponse>();
			pWorkServer->addSendTask(pChannel, ret);
		}
		break;
		case CMD_LOGOUT:
		{
			//Logout* logout = (Logout*)pheader;
			//printf("socket<%d> receive client socket<%d> message: CMD_LOGOUT , data length<%d>, user name<%s>\n", (int)_sock, (int)sock_client, pheader->data_length, logout->username);

			//LogoutResponse ret;
			//pChannel->SendData(&ret);
		}
		break;
		default:
		{
			printf("socket<%d> receive client socket<%d> message: CMD_UNKNOWN , data length<%d>\n", (int)_sock, (int)pChannel->sockfd(), pheader->data_length);
			//UnknownResponse ret;
			//pChannel->SendData(&ret);
		}
		break;
		}
	}

	// it would only be triggered by one thread, safe
	virtual void OnLeave(ChannelPtrRef pChannel)
	{
		EasyTcpServer::OnLeave(pChannel);
	}

	// multiple thread triggering, nosafe
	virtual void OnJoin(ChannelPtrRef pChannel)
	{
		EasyTcpServer::OnJoin(pChannel);
	}

	// multiple thread triggering, nosafe
	virtual void OnNetRecv(ChannelPtrRef pChannel)
	{
		EasyTcpServer::OnNetRecv(pChannel);
	}
};

#endif
