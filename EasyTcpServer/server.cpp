// linux compile command
// g++ server.cpp -std=c++11 -pthread -o server
// ----------------------------------
#include "crc_allocator.h"
#include "crc_server.hpp"

#include <thread>

bool g_run = true;

void cmdThread()
{
	while (true)
	{
		char cmd_buf[128] = {};
		scanf("%s", cmd_buf);
		if (0 == strcmp(cmd_buf, "exit"))
		{
			printf("bye\n");
			g_run = false;
			break;
		}
		else
		{
			printf("this command was not supported.\n");
		}
	}

}

int main()
{
	std::thread t(cmdThread);
	t.detach();

	{
		CRCServer server;
		server.InitSocket();
		server.Bind(nullptr, 12345);
		server.Listen(5);
		server.Start(4);
		while (server.IsRunning() && g_run)
		{
			server.OnRun();
		}
		server.Close();
	}

	system("PAUSE");
	return 0;
}