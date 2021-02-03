// linux compile command
// g++ server.cpp -std=c++11 -pthread -o server
// g++ server.cpp -std=c++11 -pthread -pg -o server --带性能分析参数（有可能导致服务端运行不稳定）
// ----------------------------------
#include "../common/include/crc_init.h"
#include "../common/include/crc_allocator.h"
#ifdef __linux__
#include "crc_server_epoll.hpp"
#else
#include "crc_server_iocp.hpp"
#include "../common/include/crc_iocp.hpp"
#include "crc_server_select.hpp"
#include "../common/include/crc_fdset.hpp"
#endif

#include <thread>

int main()
{
	{
#ifdef __linux__
		char path[] = { "server.log" };
#else
		char path[] = { "/log/server.log" };
#endif
		CRCLogger::instance().set_log_path(path,"w");
		CRCLogger::instance().start();
#ifdef __linux__
		CRCServerEpoll server;
#else
		CRCServerIOCP server;
		//CRCServerSelect server;
#endif
		server.InitSocket();
		server.Bind(nullptr, 12345);
		server.Listen(5);
		server.Start(4);

		while (true)
		{
			char cmd_buf[128] = {};
			scanf("%s", cmd_buf);
			if (0 == strcmp(cmd_buf, "exit"))
			{
				server.Close();
				break;
			}
			else
			{
				CRCLogger::info("this command was not supported.\n");
			}
		}
	}

	system("PAUSE");
	return 0;
}