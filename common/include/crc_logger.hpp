#ifndef _CRC_LOGGER_HPP_
#define _CRC_LOGGER_HPP_

#include "crc_logger_server.hpp"
#include <ctime>


class CRCLogger
{
private:
	std::shared_ptr<CRCLogServer> _pLogServer;
	FILE* _logFile;

public:
	CRCLogger()
	{
		_logFile = nullptr;

		_pLogServer = std::make_shared<CRCLogServer>();
	}

	~CRCLogger()
	{
	}

public:
	void set_log_path(const char* path, const char* mode = "")
	{
		if (_logFile)
		{
			info("CRCLogger::set_log_path old logger file had closed\n");
			fclose(_logFile);
			_logFile = nullptr;
		}

		_logFile = fopen(path, mode);
		if (_logFile)
		{
			info("CRCLogger::set_log_path success, <%s,%s>\n", path, mode);
		}
		else
		{
			info("CRCLogger::set_log_path failed, <%s,%s>\n", path, mode);
		}
	}

	void start()
	{
		_pLogServer->Start();
	}
public:
	static CRCLogger& instance()
	{
		static CRCLogger logger;
		return logger;
	}

	static void info(const char* str)
	{
		auto pLog = &CRCLogger::instance();

		pLog->_pLogServer->addTask([pLog, str]() {
			if (pLog->_logFile)
			{
				auto t = std::chrono::system_clock::now();
				auto tt = std::chrono::system_clock::to_time_t(t);
				std::tm* ttt = std::gmtime(&tt);
				fprintf(pLog->_logFile, "%s", "INFO ");
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

		pLog->_pLogServer->addTask([pLog, format, args...]() {
			if (pLog->_logFile)
			{
				auto t = std::chrono::system_clock::now();
				auto tt = std::chrono::system_clock::to_time_t(t);
				std::tm* ttt = std::gmtime(&tt);
				fprintf(pLog->_logFile, "%s", "INFO ");
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

