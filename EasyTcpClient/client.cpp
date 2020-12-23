#include "EasyTcpClient.hpp"
#include "CellTimestamp.hpp"

#include <thread>
#include <chrono>
#include <atomic>

bool g_run = true;
// sending thread amount
const int tCount = 8;
// client amount
const int nCount = 1000;
// client object
EasyTcpClient* pclients[nCount];
// client send count
std::atomic_int sendCount = 0;
// client parallel ready
std::atomic_int readyCount = 0;
// client exit count
std::atomic_int exitCount = 1;

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
	const int c = nCount / tCount;
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

	readyCount++;
	while (readyCount < tCount)
	{
		// waitting until other thread had readied
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}

	Login login[1];
	for (int n = 0; n < 1; n++)
	{
		strcpy(login[n].username, "cnj");
		strcpy(login[n].password, "cnj123");
	}
	const int nLen = sizeof(login);

	CellTimestamp tTime;

	while (g_run)
	{
		for (int n = begin; n < end; n++)
		{
			////timing send with one per one second
			//if (tTime.getElapsedSecond() >= 1.0) {
			//	tTime.update();
			//	if (pclients[n]->IsRunning())
			//	{
			//		if (SOCKET_ERROR != pclients[n]->SendData(login, nLen))
			//		{
			//			sendCount++;
			//		}
			//	}
			//}

			if (pclients[n]->IsRunning())
			{
				if (SOCKET_ERROR != pclients[n]->SendData(login, nLen))
				{
					sendCount++;
				}
			}

			pclients[n]->OnRun();

		}

	}

	for (int n = begin; n < end; n++)
	{
		pclients[n]->Close();
		delete pclients[n];
		//printf("client exit <%d>\n", exitCount++);
		exitCount++;
		//std::chrono::microseconds t(10);
		//std::this_thread::sleep_for(t);
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

	CellTimestamp tTime;

	while (g_run)
	{
		auto t1 = tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			printf("threads<%d>,clients<%d>,time<%lf>,send<%d>\n", tCount, nCount, t1, (int)(sendCount / t1));
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	while (exitCount < nCount)
	{
		Sleep(100);
	}

	return 0;
}