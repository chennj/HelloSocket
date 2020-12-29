#ifndef _CRC_SERVER_HPP_
#define _CRC_SERVER_HPP_

#include "crc_init.h"
#include "crc_boss_server.hpp"

class CRCServer : public CRCBossServer
{
	// overwrite parent function
public:
	// multiple thread triggering, nosafe
	virtual void OnNetMessage(CRCWorkServer* pWorkServer, CRCChannelPtrRef pChannel, CRCDataHeader* pheader)
	{
		CRCBossServer::OnNetMessage(pWorkServer, pChannel, pheader);

		switch (pheader->cmd)
		{
		case CMD_LOGIN:
		{
			//pChannel->reset_dt_heart();

			Login* login = (Login*)pheader;
			//printf("socket<%d> receive client socket<%d> message: CMD_LOGIN , data length<%d>, user name<%s>, pwd<%s>\n", (int)_sock, (int)sock_client, pheader->data_length, login->username, login->password);
			
			// Asynchronous send mode with sending buffer which is as the same thread as main thread
			if (SOCKET_ERROR == pChannel->SendDataBuffer(std::make_shared<LoginResponse>()))
			{
				if (_tTime.getElapsedSecond() >= 1.0)
					printf("Socket<%d>, Send Buffer Is Full\n", pChannel->sockfd());
			}
			
			// Synchronous send mode with sending task pool which is not as the same thread as main thread
			//CRCDataHeaderPtr ret = std::make_shared<LoginResponse>();
			//pWorkServer->addSendTask(pChannel, ret);
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
		case CMD_HEART_C2S:
		{
			pChannel->reset_dt_heart();

			pChannel->SendData(std::make_shared<HeartS2C>());
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
	virtual void OnLeave(CRCChannelPtrRef pChannel)
	{
		CRCBossServer::OnLeave(pChannel);
	}

	// multiple thread triggering, nosafe
	virtual void OnJoin(CRCChannelPtrRef pChannel)
	{
		CRCBossServer::OnJoin(pChannel);
	}

	// multiple thread triggering, nosafe
	virtual void OnNetRecv(CRCChannelPtrRef pChannel)
	{
		CRCBossServer::OnNetRecv(pChannel);
	}
};

#endif
