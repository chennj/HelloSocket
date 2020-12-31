#include "crc_logger.h"

void 
Logger::info(const char* str)
{
	auto pLog = &CRCLogger::instance();

	pLog->_pTaskServer->addTask([pLog, str]() {
		if (pLog->_logFile)
		{
			auto t = system_clock::now();
			auto tt = system_clock::to_time_t(t);
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

CRCLogger::CRCLogger()
{
	_logFile = nullptr;
	
	_pTaskServer = std::make_shared<CRCTaskServer>();
}

CRCLogger::~CRCLogger()
{
}

void 
CRCLogger::start()
{
	_pTaskServer->Start();
}

void 
CRCLogger::set_log_path(const char* path, const char* mode)
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