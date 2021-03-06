#ifndef _CRC_SERVER_IOCP_HPP_
#define _CRC_SERVER_IOCP_HPP_

#ifdef _WIN32
#include "../common/include/crc_init.h"
#include "../common/include/crc_boss_iocp_server.hpp"
#include <string>

class CRCServerIOCP : public CRCBossIOCPServer
{
	// overwrite parent function
public:
	// multiple thread triggering, nosafe
	virtual void OnNetMessage(CRCWorkServer* pWorkServer, CRCChannel* pChannel, CRCDataHeader* pheader)
	{
		CRCBossIOCPServer::OnNetMessage(pWorkServer, pChannel, pheader);

		switch (pheader->cmd)
		{
		case CMD_LOGIN:
		{
			//pChannel->reset_dt_heart();
			Login* login = (Login*)pheader;
			//CRCLogger::info("socket<%d> receive client socket<%d> message: CMD_LOGIN , data length<%d>, user name<%s>, pwd<%s>\n", (int)_sock, (int)sock_client, pheader->data_length, login->username, login->password);

			// Asynchronous send mode with sending buffer which is as the same thread as main thread
			CRCLogger_Mem_Trace("开始发送\n");
			{
				//巨坑的内存泄漏的写法
				//if (SOCKET_ERROR == pChannel->SendDataBuffer(new LoginResponse))
				//{
				//	if (_tTime.getElapsedSecond() >= 1.0)
				//		CRCLogger_Info("Socket<%d>, Send Buffer Is Full\n", pChannel->sockfd());
				//}
				//下面是正确的写法,下面有更详细的解释。
				//CRCLogger_Mem_Trace真的很管用啊
				LoginResponse* response = new LoginResponse;
				if (SOCKET_ERROR == pChannel->SendDataBuffer(response))
				{
					if (_tTime.getElapsedSecond() >= 1.0)
						CRCLogger_Info("Socket<%d>, Send Buffer Is Full\n", pChannel->sockfd());
				}
				delete response;
			}
			CRCLogger_Mem_Trace("发送结束\n");
			// Synchronous send mode with sending task pool which is not as the same thread as main thread
			//CRCDataHeaderPtr ret = std::make_shared<LoginResponse>();
			//pWorkServer->addSendTask(pChannel, ret);
		}
		break;
		case CMD_LOGOUT:
		{
			//Logout* logout = (Logout*)pheader;
			//CRCCRCLogger::info("socket<%d> receive client socket<%d> message: CMD_LOGOUT , data length<%d>, user name<%s>\n", (int)_sock, (int)sock_client, pheader->data_length, logout->username);

			//LogoutResponse ret;
			//pChannel->SendData(&ret);
		}
		break;
		case CMD_HEART_C2S:
		{
			pChannel->reset_dt_heart();
			/**
			*
			*	内存泄漏的坑
			*
			*/
			// 下面这种方式会产生内存泄漏，因为 SendDataBuffer 函数内部并没有对 new HeartS2C
			// 所分配的内存进行释放
			//pChannel->SendData(new HeartS2C);
			// 下面时正确的写法
			HeartS2C* response = new HeartS2C();
			pChannel->SendDataBuffer(response);
			delete response;
			// ----------------------------------
		}
		break;
		case CMD_STREAM:	// 字节流消息
		{
			CRCRecvStream r(pheader);
			printf("read %d\n", r.read_int8());
			printf("read %d\n", r.read_int16());
			printf("read %d\n", r.read_int32());
			printf("read %lf\n", r.read_float());
			printf("read %lf\n", r.read_double());
			char a1[16] = {};
			printf("read %d, %s\n", r.read_array(a1, 16), a1);
			std::string a2;
			printf("read %d, %s\n", r.ReadString(a2, 16), a2.c_str());
			int a3[10] = {};
			printf("read %d\n", r.read_array(a3, 10));

			CRCSendStream s;
			s.set_cmd(CMD_STREAM_RESPONSE);
			s.write_int8(1);
			s.write_int16(2);
			s.write_int32(3);
			s.write_float(4.1f);
			s.write_double(5.2);
			char* ss = "hello";
			s.write_array(ss, strlen(ss));
			char* str = "i am server";
			s.write_array(str, strlen(str));
			int a[] = { 1,2,3,4 };
			s.write_array(a, 4);
			s.finish();

			if (SOCKET_ERROR == pChannel->SendDataBuffer(s.data(), s.data_length()))
			{
				if (_tTime.getElapsedSecond() >= 1.0)
					CRCLogger::info("Socket<%d>, Send Buffer Is Full\n", pChannel->sockfd());
			}
		}
		break;
		default:
		{
			CRCLogger::info("socket<%d> receive client socket<%d> message: CMD_UNKNOWN , data length<%d>\n", (int)_sock, (int)pChannel->sockfd(), pheader->data_length);
			//UnknownResponse ret;
			//pChannel->SendData(&ret);
		}
		break;
		}
	}

	// it would only be triggered by one thread, safe
	virtual void OnLeave(CRCChannel* pChannel)
	{
		CRCBossServer::OnLeave(pChannel);
	}

	// multiple thread triggering, nosafe
	virtual void OnJoin(CRCChannel* pChannel)
	{
		CRCBossServer::OnJoin(pChannel);
	}

	// multiple thread triggering, nosafe
	virtual void OnNetRecv(CRCChannel* pChannel)
	{
		CRCBossServer::OnNetRecv(pChannel);
	}
};

#endif
#endif
