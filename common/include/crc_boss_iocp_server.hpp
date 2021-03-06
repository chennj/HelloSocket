#ifndef _CRC_BOSS_IOCP_SERVER_HPP_
#define _CRC_BOSS_IOCP_SERVER_HPP_

#ifdef _WIN32

#ifndef CRC_USE_IOCP
#define CRC_USE_IOCP
#endif

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

		/**
		*
		* 向IOCP投递一个接受客户端连接的申请
		*/
		// minLen 不需要客户端在连接后立即发送数据的情况下的最小长度
		const int minLen = (sizeof(sockaddr_in) + 16) * 2;
		const int len = minLen * 2;
		char buffer[len] = {};
		// IO_CONTEXT 包含投递一个接受连接任务需要的所有类型数据
		IO_CONTEXT ioData = {};
		ioData._wsabuf.len = len;
		ioData._wsabuf.buf = buffer;
		// 投递一个接受连接任务
		iocp.delivery_accept(&ioData);

		IO_EVENT ioEvent = {};
		while (IsRunning())
		{
			time4Msg();

			int ret_events = iocp.wait(ioEvent, 1);
			if (ret_events < 0)
			{
				CRCLogger_Error("BossServer::OnRun socket<%d> error occurs while iocp and mission finish.", (int)_sock);
				break;
			}

			if (0 == ret_events)continue;

			// 接受连接已经完成
			if (IO_TYPE::ACCEPT == ioEvent.pIoCtx->_OpType)
			{
				//CRCLogger_Info("BossServer::OnRun new client enter. socket=%d\n", ioEvent.pIoCtx->_sockfd);
				// 添加一个channel到工作线程
				IOCPAccept(ioEvent.pIoCtx);
				// 继续向IOCP投递一个接受新的客户端连接的申请
				iocp.delivery_accept(&ioData);
			}
		}

		pCrcThread->ExitInSelfThread();

		CRCLogger_Info("BossServer thread exit...\n");
	}

	// accept client's connectin
	void IOCPAccept(PIO_CONTEXT pIoCtx)
	{
		//sockaddr_in client_addr = {};
		//int client_addr_len = sizeof(sockaddr_in);	// note: accept will fall without sizeof(sockaddr_in)

		if (INVALID_SOCKET == pIoCtx->_sockfd)
		{
			CRCLogger_Error("BossServer::IOCPAccept socket<%d> accept a invalid client request.", (int)_sock);
			return;
		}

		if (_clientCount < _maxClient)
		{
			//CRCWorkServer::make_reuseaddr(pIoCtx->_sockfd);

			// assign comed client to WorkServer with the least number of client
			addClient2WorkServer(new CRCChannel(pIoCtx->_sockfd));
		}
		else
		{
			CRCWorkServer::destory_socket(pIoCtx->_sockfd);
			CRCLogger_Warn("BossServer::IOCPAccept Accept to Max Client\n");
		}
	}
};

#endif

#endif
