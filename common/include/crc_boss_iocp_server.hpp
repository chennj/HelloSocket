#ifndef _CRC_BOSS_IOCP_SERVER_HPP_
#define _CRC_BOSS_IOCP_SERVER_HPP_

#ifdef _WIN32

#include "crc_iocp.hpp"
#include "crc_boss_server.hpp"

class CRCBossIOCPServer : public CRCBossServer<CRCWorkIOCPServer, CRCWorkIOCPServerPtr>
{
private:

public:
	CRCBossIOCPServer()
	{

	}
	virtual ~CRCBossIOCPServer()
	{

	}

protected:
	// main loop, override from CRCBossServer
	virtual void OnRun(CRCThread* pCrcThread)
	{
		CRCLogger::info("BossServer thread start...\n");

		CRCIOCP iocp;
		iocp.create();
		iocp.register_sock(_sock);
		iocp.load_acceptex(_sock);

		// ��IOCPͶ��һ�����ܿͻ������ӵ�����
		IO_CONTEXT ioData = {};
		iocp.delivery_accept(&ioData);

		IO_EVENT ioEvent = {};
		while (IsRunning())
		{
			time4Msg();

			int ret_events = iocp.wait(ioEvent, 1);
			if (ret_events < 0)
			{
				CRCLogger_Error("BossServer::OnRun socket<%d> error occurs while epoll and mission finish.\n", (int)_sock);
				break;
			}

			if (0 == ret_events)continue;

			// ���������Ѿ����
			if (IO_TYPE::ACCEPT == ioEvent.pIoData->_OpType)
			{
				CRCLogger_Info("BossServer::OnRun new client enter. socket=%d\n", ioEvent.pIoData->_sockfd);
				// ���һ��channel�������߳�
				IOCPAccept(ioEvent.pIoData);
				// ������IOCPͶ��һ�������µĿͻ������ӵ�����
				iocp.delivery_accept(&ioData);
			}
		}

		pCrcThread->ExitInSelfThread();

		CRCLogger::info("BossServer thread exit...\n");
	}

	// accept client's connectin
	void IOCPAccept(PIO_CONTEXT pIoData)
	{
		sockaddr_in client_addr = {};
		int client_addr_len = sizeof(sockaddr_in);	// note: accept will fall without sizeof(sockaddr_in)

		if (INVALID_SOCKET == pIoData->_sockfd)
		{
			CRCLogger_Error("BossServer::IOCPAccept socket<%d> accept a invalid client request.\n", (int)_sock);
			return;
		}

		if (_clientCount < _maxClient)
		{
			CRCWorkServer::make_reuseaddr(pIoData->_sockfd);

			// assign comed client to WorkServer with the least number of client
			addClient2WorkServer(new CRCChannel(pIoData->_sockfd));
		}
		else
		{
			CRCWorkServer::destory_socket(pIoData->_sockfd);
			CRCLogger_Warn("BossServer::IOCPAccept Accept to Max Client\n");
		}
	}
};

#endif

#endif
