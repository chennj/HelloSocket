#ifndef _CRC_LOGGER_H_
#define _CRC_LOGGER_H_

#include "crc_init.h"
#include "crc_task_server.hpp"

#include <ctime>

class CRCLogger
{
	friend void Logger::info(const char* str);

private:
	std::shared_ptr<CRCTaskServer> _pTaskServer;
	FILE* _logFile;

public:
	CRCLogger();
	~CRCLogger();

public:
	void set_log_path(const char* path, const char* mode = "");
	void start();
public:
	static CRCLogger& instance()
	{
		static CRCLogger logger;
		return logger;
	}

	static void info(const char* str)
	{
		auto pLog = &CRCLogger::instance();

		pLog->_pTaskServer->addTask([pLog, str]() {
			if (pLog->_logFile)
			{
				auto t = system_clock::now();
				auto tt = system_clock::to_time_t(t);
				std::tm* ttt = std::gmtime(&tt);
				fprintf(pLog->_logFile, "%d-%d-%d %d:%d:%d\t%s",
					ttt->tm_year + 1900, ttt->tm_mon + 1, ttt->tm_mday,
					ttt->tm_hour + 8, ttt->tm_min, ttt->tm_sec,
					str);
				fflush(pLog->_logFile);
			}
			printf("%s", str);
		});
	}

	template<typename ...Args>
	static void info(const char* format, Args... args)
	{
		auto pLog = &CRCLogger::instance();

		pLog->_pTaskServer->addTask([pLog, format, args...]() {
			if (pLog->_logFile)
			{
				auto t = system_clock::now();
				auto tt = system_clock::to_time_t(t);
				std::tm* ttt = std::gmtime(&tt);
				fprintf(pLog->_logFile, "%d-%d-%d %d:%d:%d\t",
					ttt->tm_year + 1900, ttt->tm_mon + 1, ttt->tm_mday,
					ttt->tm_hour + 8, ttt->tm_min, ttt->tm_sec);
				fprintf(pLog->_logFile, format, args...);
				fflush(pLog->_logFile);
			}
			printf(format, args...);
		});
	}
};

#endif
