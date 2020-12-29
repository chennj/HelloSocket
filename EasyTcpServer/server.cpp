// linux compile command
// g++ server.cpp -std=c++11 -pthread -o server
// ----------------------------------
#include "crc_allocator.h"
#include "crc_server.hpp"

#include <thread>

int main()
{
	{
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