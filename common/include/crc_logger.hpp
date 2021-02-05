#ifndef _CRC_LOGGER_HPP_
#define _CRC_LOGGER_HPP_

#include "crc_logger_server.hpp"
#include <ctime>
#include <mutex>
#include <atomic>
#include <stdio.h>

class CRCLogger
{

#ifdef _MEM_TRACE_
#define CRCLogger_Mem_Trace(...) CRCLogger::mem(__VA_ARGS__)
#else
#define CRCLogger_Mem_Trace(...) 
#endif

#define CRCLogger_Info(...) CRCLogger::info(__VA_ARGS__)
#define CRCLogger_Warn(...) CRCLogger::warn(__VA_ARGS__)
#define CRCLogger_Error(...) CRCLogger::error(__VA_ARGS__)

private:
	//CRCAutoPtr<CRCLogServer> _pLogServer;
	CRCLogServer* _pLogServer;
	FILE* _logFile;

public:
	CRCLogger()
	{
		_logFile = nullptr;
		_pLogServer = nullptr;
	}

	~CRCLogger()
	{
		fclose(_logFile);
		_logFile = nullptr;
		if (_pLogServer)
			delete _pLogServer;
	}

	CRCLogger(const CRCLogger& signal) = delete;
	const CRCLogger& operator=(const CRCLogger &signal) = delete;

	/**
	*	有了这两个函数，CRCLogger的new和delete将不再使用内存池(CRCMemoryPool)的
	*	new和delete,因为会产生循环调用
	*/
	void* operator new(size_t size)
	{
		printf("CRCLogger self new\n");
		return malloc(size);
	}

	void operator delete(void * pv)
	{
		printf("CRCLogger self delete\n");
		free(pv);
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
		if (!_pLogServer)
			_pLogServer = new CRCLogServer();

		_pLogServer->Start();
	}
public:
	//使用这种方式获取单例，本类中的new和delete重载将不起作用
	//static CRCLogger& instance()
	//{
	//	static CRCLogger logger;
	//	return logger;
	//}

	static CRCLogger& instance()
	{
		static std::mutex _mutex;
		static CRCLogger* _instance = nullptr;
		if (nullptr == _instance)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (nullptr == _instance)
			{
				printf("CRCLogger\t::instance()\n");
				CRCLogger* tmp = new CRCLogger();
				MemoryBarrier();
				_instance = tmp;
			}
		}
		return *_instance;
	}
	static void mem(const char* str)
	{
		mem("%s", str);
	}

	template<typename ...Args>
	static void mem(const char* format, Args... args)
	{
		echo_real_mem("INFO", format, args...);
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
		auto pLog = &instance();
		if (pLog->_pLogServer && pLog->_pLogServer->_isStart)
		{
			pLog->_pLogServer->addTask([=]() {
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
		}
		else
		{
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
		}
		
#else
		auto errCode = errno;
		auto pLog = &instance();
		if (pLog->_pLogServer && pLog->_pLogServer->_isStart)
		{
			pLog->_pLogServer->addTask([=]() {
				echo_real("\nERROR", format, args...);
				echo_real("\nERROR", "errno<%d>,errmsg<%s>", errCode, strerror(errCode));
			});
		}
		else
		{
			echo_real("\nERROR", format, args...);
			echo_real("\nERROR", "errno<%d>,errmsg<%s>", errCode, strerror(errCode));
		}

#endif
	}

	template<typename ...Args>
	static void echo(const char* level_tag, const char* format, Args... args)
	{
		auto pLog = &CRCLogger::instance();

		if (pLog->_pLogServer && pLog->_pLogServer->_isStart)
		{ 
			pLog->_pLogServer->addTask([=]() {
				echo_real(level_tag, format, args...);
			});
		}
		else {
			echo_real(level_tag, format, args...);
		}

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

	template<typename ...Args>
	static void echo_real_mem(const char* level_tag, const char* format, Args... args)
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
#ifdef _DEBUG
		printf("%s ", level_tag);
		printf(format, args...);
#endif
	}
};

#endif

