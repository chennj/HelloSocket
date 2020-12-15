#ifndef _MYSERVER_HPP_
#define _MYSERVER_HPP_

#include "EasyTcpServer.hpp"

class MyServer : public EasyTcpServer
{
	// overwrite parent function
public:
	// multiple thread triggering, nosafe
	virtual void OnNetMessage(ClientSocket* pClientSocket, DataHeader* pheader)
	{
		_msgCount++;
		switch (pheader->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)pheader;
			//printf("socket<%d> receive client socket<%d> message: CMD_LOGIN , data length<%d>, user name<%s>, pwd<%s>\n", (int)_sock, (int)sock_client, pheader->data_length, login->username, login->password);

			LoginResponse ret;
			pClientSocket->SendData(&ret);
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

	// it would only be triggered by one thread, safe
	virtual void OnLeave(ClientSocket* pClientSocket)
	{
		_clientCount--;
		//printf("client socket<%d> leaved\n", pClientSocket->sockfd());
	}

	// multiple thread triggering, nosafe
	virtual void OnJoin(ClientSocket* pClientSocket)
	{
		_clientCount++;
		//printf("client socket<%d> joined\n", pClientSocket->sockfd());
	}

	// multiple thread triggering, nosafe
	virtual void OnNetRecv(ClientSocket* pClientSocket)
	{
		_recvCount++;
	}
};

#endif
