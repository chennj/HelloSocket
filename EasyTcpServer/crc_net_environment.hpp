#ifndef _CRC_NET_ENVIRONMENT_HPP
#define _CRC_NET_ENVIRONMENT_HPP

#include "crc_init.h"

/**
*
*	initialize net environment
*
*/
class CRCNetEnvironment
{
private:
	CRCNetEnvironment()
	{
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(ver, &data);
#else
		// ignore exception signal
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		{
			return (1);
		}
#endif
	}

	~CRCNetEnvironment()
	{
#ifdef _WIN32
		WSACleanup();
#endif
	}

public:
	// ����ģʽ
	// c++11�Ѿ�Ϊ���������̰߳�ȫ����Ĺ��������Բ��õ����ظ�����
	// �����ڣ�thread_safe_statics.cpp �е� _Init_thread_header
	static void init()
	{
		static CRCNetEnvironment net_env;
	}
};

#endif
