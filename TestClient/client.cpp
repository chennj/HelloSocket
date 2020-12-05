#include "EasyTcpClient.hpp"

#include <thread>

void cmd_thread(EasyTcpClient* pclient)
{
	while (true)
	{
		char cmd_buf[128] = {};
		scanf("%s", cmd_buf);
		if (0 == strcmp(cmd_buf, "exit"))
		{
			printf("bye\n");
			break;
		}
		else if (0 == strcmp(cmd_buf, "login"))
		{
			Login login;
			strcpy(login.username, "cnj");
			strcpy(login.password, "cnj123");
			pclient->SendData(&login);
		}
		else if (0 == strcmp(cmd_buf, "logout"))
		{
			Logout logout;
			strcpy(logout.username, "cnj");
			pclient->SendData(&logout);
		}
		else
		{
			printf("this command was not supported.\n");
		}
	}

	pclient->Close();
}

bool g_run = true;
void cmd_thread2()
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
	//EasyTcpClient client1;
	//client1.Connect("127.0.0.1", 12345);

	//std::thread t1(cmd_thread, &client1);
	//t1.detach();

	//EasyTcpClient client2;
	//client2.Connect("127.0.0.1", 12346);

	//std::thread t2(cmd_thread, &client2);
	//t2.detach();

	//EasyTcpClient client3;
	//client3.Connect("127.0.0.1", 12347);

	//std::thread t3(cmd_thread, &client3);
	//t3.detach();
	//while (client1.IsRunning() || client2.IsRunning() || client3.IsRunning())
	//{
	//	client1.OnRun();
	//	client2.OnRun();
	//	client3.OnRun();
	//}

	//client1.Close();
	//client2.Close();
	//client3.Close();

	//EasyTcpClient client;
	//client.Connect("192.168.137.129", 12345);
	//client.Connect("127.0.0.1", 12345);

	//std::thread t(cmd_thread, &client);
	//t.detach();

	const int cCount = 4;

	//下面这种定义方式很不好，如果nCount过大，会造成栈溢出
	//EasyTcpClient clients[cCount];

	EasyTcpClient* pclients[cCount];
	
	for (int n = 0; n < cCount; n++)
	{
		pclients[n] = new EasyTcpClient;
	}

	for (int n = 0; n < cCount; n++)
	{
		pclients[n]->Connect("127.0.0.1", 12345);
	}

	// start ui thread
	std::thread t(cmd_thread2);
	t.detach();

	Login login;
	strcpy(login.username, "cnj");
	strcpy(login.password, "cnj123");
	while (g_run)
	{
		for (int n = 0; n < cCount; n++)
		{
			pclients[n]->OnRun();
			pclients[n]->SendData(&login);
		}
	}

	for (int n = 0; n < cCount; n++)
	{
		pclients[n]->Close();
	}

	return 0;
}