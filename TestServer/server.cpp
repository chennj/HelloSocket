#include "EasyTcpServer.hpp"

#include <thread>

bool g_run = true;

void cmd_thread()
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
	//EasyTcpServer server;
	//server.InitSocket();
	//server.Bind(nullptr, 12345);
	//server.Listen(5);

	//EasyTcpServer server2;
	//server2.InitSocket();
	//server2.Bind(nullptr, 12346);
	//server2.Listen(5);

	//while (server.IsRunning() || server2.IsRunning())
	//{
	//	server.OnRun();
	//	server2.OnRun();
	//}
	//server.Close();
	//server2.Close();

	std::thread t(cmd_thread);
	t.detach();

	EasyTcpServer server;
	server.InitSocket();
	server.Bind(nullptr, 12345);
	server.Listen(5);

	while (server.IsRunning() && g_run)
	{
		server.OnRun();
	}
	server.Close();
	return 0;
}