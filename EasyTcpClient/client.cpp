#include "EasyTcpClient.hpp"

#include <thread>
#include <chrono>

bool g_run = true;
// sending thread amount
const int tCount = 4;
// client amount
const int nCount = 10000;
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

	std::chrono::milliseconds t(5000);
	std::this_thread::sleep_for(t);

	Login login[10];
	for (int n = 0; n < 10; n++)
	{
		strcpy(login[n].username, "cnj");
		strcpy(login[n].password, "cnj123");
	}
	const int nLen = sizeof(login);

	while (g_run)
	{
		for (int n = begin; n < end; n++)
		{
			if (pclients[n]->IsRunning())
			{
				pclients[n]->SendData(login, nLen);
				//pclients[n]->OnRun();
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