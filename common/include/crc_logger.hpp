#ifndef _CRC_LOGGER_HPP_
#define _CRC_LOGGER_HPP_

#include "crc_logger_server.hpp"
#include <ctime>


class CRCLogger
{

#define CRCLogger_Info(...) CRCLogger::info(__VA_ARGS__)
#define CRCLogger_Warn(...) CRCLogger::warn(__VA_ARGS__)
#define CRCLogger_Error(...) CRCLogger::error(__VA_ARGS__)

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
		fclose(_logFile);
		_logFile = nullptr;
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
		info("%s", str);
	}

	template<typename ...Args>
	static void info(const char* format, Args... args)
	{
		echo("INFO", format, args...);
	}

	static void warn(const char* str)
	{
		warn("%s", str);
	}

	template<typename ...Args>
	static void warn(const char* format, Args... args)
	{
		echo("WARN", format, args...);
	}

	static void error(const char* str)
	{
		error("%s", str);
	}

	template<typename ...Args>
	static void error(const char* format, Args... args)
	{
#ifdef _WIN32
		auto errCode = ::GetLastError();
		//第一种用法：有点坑
		//char* text = nullptr;
		//FormatMessageA(
		//	FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		//	NULL,
		//	errCode,
		//	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		//	(LPSTR)&text,
		//	0,
		//	NULL
		//);
		//echo("ERROR", format, args...);
		//echo("ERROR", "errno<%d>,errmsg<%s>", errCode, text);
		//instance()._pLogServer->addTask([=]() {
		//	LocalFree(text);
		//});
		instance()._pLogServer->addTask([=]() {
			char text[256] = {};
			FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				errCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)&text,
				256,
				NULL
			);
			echo_real("\nERROR", format, args...);
			echo_real("\nERROR", "errno<%d>,errmsg<%s>", errCode, text);
		});
#else
		auto errCode = errno;
		instance()._pLogServer->addTask([=]() {
			echo_real("\nERROR", format, args...);
			echo_real("\nERROR", "errno<%d>,errmsg<%s>", errCode, strerror(errCode));
		});

#endif
	}

	template<typename ...Args>
	static void echo(const char* level_tag, const char* format, Args... args)
	{
		auto pLog = &CRCLogger::instance();

		pLog->_pLogServer->addTask([=]() {
			echo_real(level_tag, format, args...);
		});
	}

	template<typename ...Args>
	static void echo_real(const char* level_tag, const char* format, Args... args)
	{
		auto pLog = &CRCLogger::instance();

		if (pLog->_logFile)
		{
			auto t = std::chrono::system_clock::now();
			auto tt = std::chrono::system_clock::to_time_t(t);
			std::tm* ttt = std::gmtime(&tt);
			fprintf(pLog->_logFile, "%s ", level_tag);
			fprintf(pLog->_logFile, "%d-%d-%d %d:%d:%d\t",
				ttt->tm_year + 1900, ttt->tm_mon + 1, ttt->tm_mday,
				ttt->tm_hour + 8, ttt->tm_min, ttt->tm_sec);
			fprintf(pLog->_logFile, format, args...);
			fflush(pLog->_logFile);
		}
		printf("%s ", level_tag);
		printf(format, args...);
	}

};

#endif

