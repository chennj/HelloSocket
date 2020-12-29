#ifndef _CRC_LOG_HPP_
#define _CRC_LOG_HPP_

#include "crc_init.h"

class CRCLogger
{
private:
	CRCTaskServer _taskServer;

public:
	CRCLogger()
	{
		_taskServer.Start();
	}

	~CRCLogger()
	{
		_taskServer.Close();
	}

public:
	static CRCLogger& instance()
	{
		static CRCLogger logger;
		return logger;
	}

	// level: info
	static void info(const char* str)
	{
		printf("%s", str);
	}

	template<typename ...Args>
	static void info(const char* format, Args... args)
	{
		printf(format, args...);
	}
};

#endif