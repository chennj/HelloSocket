#ifndef _CRC_CLIENT_HPP_
#define _CRC_CLIENT_HPP_

#include "../common/include/crc_init.h"
#include "../common/include/crc_work_client.hpp"

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
			//printf("receive server msg: CMD_LOGIN_RESPONSE, data length: %d, result: %d\n", header->data_length, ret->result);
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
		case CMD_HEART_S2C:
		{
			HeartS2C* ret = (HeartS2C*)header;
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

