// linux compile command
// g++ server.cpp -std=c++11 -pthread -o server
// ----------------------------------
#include "crc_init.h"
#include "../common/include/crc_allocator.h"
#include "crc_server.hpp"

#include <thread>

int main()
{
	{
		CRCLogger::instance().set_log_path("/log/server.log","w");
		CRCLogger::instance().start();

		CRCServer server;
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