#include "EasyTcpClient.hpp"

#include <thread>

bool g_run = true;
// sending thread amount
const int tCount = 4;
// client amount
const int nCount = 100;
// client object
EasyTcpClient* pclients[nCount];

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

void sendThread(int id) //1~4
{
	//4 thread around about 1~4
	int c = nCount / tCount;
	int begin = (id - 1)*c;
	int end = begin + c;

	for (int n = begin; n < end; n++)
	{
		if (!g_run)return;
		pclients[n] = new EasyTcpClient;
	}

	for (int n = begin; n < end; n++)
	{
		if (!g_run)return;
		pclients[n]->Connect("127.0.0.1", 12345);
	}

	Login login;
	strcpy(login.username, "cnj");
	strcpy(login.password, "cnj123");
	while (g_run)
	{
		for (int n = begin; n < end; n++)
		{
			if (pclients[n]->IsRunning())
			{
				pclients[n]->SendData(&login);
				pclients[n]->OnRun();
			}
		}
	}

	for (int n = begin; n < end; n++)
	{
		pclients[n]->Close();
	}
}

int main()
{
	//start ui thread
	std::thread t1(cmdThread);
	t1.detach();

	// start send thread 
	for (int n = 0; n < tCount; n++)
	{
		std::thread t(sendThread, n + 1);
		t.detach();
	}

	while (g_run)
	{
		Sleep(100);
	}

	return 0;
}