#ifndef _CRC_CLIENT_HPP_
#define _CRC_CLIENT_HPP_

#include "crc_init.h"
#include "crc_work_client.hpp"

class CRCClient : public CRCWorkClient
{
public:
	CRCClient()
	{

	}
	~CRCClient()
	{

	}
	// overwrite parent function
public:
	void OnNetMessage(CRCDataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESPONSE:
		{
			LoginResponse* ret = (LoginResponse*)header;
		}
		break;
		case CMD_LOGOUT_RESPONSE:
		{
			LogoutResponse* ret = (LogoutResponse*)header;
			//printf("receive server msg: CMD_LOGOUT_RESPONSE, data length: %d, result: %d\n", header->data_length, ret->result);
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			NewUserJoin* ret = (NewUserJoin*)header;
			//printf("receive server msg: CMD_NEW_USER_JOIN, data length: %d, result: %d\n", header->data_length, ret->sock);
		}
		break;
		case CMD_STREAM_RESPONSE:	// 字节流消息
		{
			CRCRecvStream r(header);
			printf("read %d\n", r.read_int8());
			printf("read %d\n", r.read_int16());
			printf("read %d\n", r.read_int32());
			printf("read %lf\n", r.read_float());
			printf("read %lf\n", r.read_double());
			char a1[16] = {};
			printf("read %d, %s\n", r.read_array(a1, 16), a1);
			char a2[16] = {};
			printf("read %d, %s\n", r.read_array(a2, 16), a2);
			int a3[10] = {};
			printf("read %d\n", r.read_array(a3, 10));
		}
		break;
		default:
		{
			printf("receive server msg: UNKNOWN, socket<%d>, data length: %d\n", (int)_pChannel->sockfd(), header->data_length);
		}
		break;
		}
	}

};

#endif

