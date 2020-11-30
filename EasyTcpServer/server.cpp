#include "EasyTcpServer.hpp"

int main()
{
	EasyTcpServer server;
	server.InitSocket();
	server.Bind(nullptr, 12345);
	server.Listen(5);

	EasyTcpServer server2;
	server2.InitSocket();
	server2.Bind(nullptr, 12346);
	server2.Listen(5);

	while (server.IsRunning() || server2.IsRunning())
	{
		server.OnRun();
		server2.OnRun();
	}
	server.Close();
	server2.Close();
	return 0;
}