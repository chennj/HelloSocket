// linux compile command
// g++ client.cpp -std=c++11 -pthread -o client
// ----------------------------------
#include "crc_work_client.hpp"
#include "crc_timestamp.hpp"

#include <thread>
#include <chrono>
#include <atomic>

bool g_run = true;

// sending thread amount
const int tCount = 2;
// client amount
const int nCount = 100;

// client object
CRCWorkClient* pclients[nCount];

// client send count
std::atomic_int sendCount;
// client parallel ready
std::atomic_int readyCount;
// client exit count
std::atomic_int exitCount;

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

void recvThread(int begin, int end)
{
	CRCTimestamp t;
	while (g_run)
	{
		for (int n = begin; n < end; n++)
		{
			//if (t.getElapsedSecond() > 4.0 && n == begin) {
			//	//模拟一个客户端接收被堵塞的情况，服务器的运行情况
			//	//期望：对于没有被堵塞的客户端运行正常
			//	continue;
			//}

			int ret = pclients[n]->OnRun();
			if (ret < 0)
			{
				goto exit;
			}
		}
	}

exit:
	printf("client recv end...\n");
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
		pclients[n] = new CRCWorkClient;
	}

	for (int n = begin; n < end; n++)
	{
		if (!g_run)return;
		pclients[n]->Connect("192.168.137.129", 12345);
	}

	readyCount++;

	while (readyCount < tCount)
	{
		// waitting until other thread had readied
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}

	// start receive thread
	std::thread t(recvThread, begin, end);
	t.detach();

	const int msgcount = 1;
	Login login[msgcount];

	for (int n = 0; n < msgcount; n++)
	{
		strcpy(login[n].username, "cnj");
		strcpy(login[n].password, "cnj123");
	}

	const int nLen = sizeof(login);
	CRCTimestamp tTime;

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
				//else {
				//	g_run = false;
				//}
			}
		}

		// to control speed to send
		//std::chrono::milliseconds t(1);
		//std::this_thread::sleep_for(t);
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
	sendCount = 0;
	readyCount = 0;
	exitCount = 1;

	//start ui thread
	std::thread t1(cmdThread);
	t1.detach();

	// start send thread 
	for (int n = 0; n < tCount; n++)
	{
		std::thread t(sendThread, n + 1);
		t.detach();
	}

	CRCTimestamp tTime;

	while (g_run)
	{
		auto t1 = tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			printf("threads<%d>,clients<%d>,time<%lf>,send<%d>\n", tCount, nCount, t1, (int)(sendCount / t1));
			sendCount = 0;
			tTime.update();
		}
		std::chrono::milliseconds t(1);
		std::this_thread::sleep_for(t);
	}

	while (exitCount < nCount)
	{
		std::chrono::milliseconds t(100);
		std::this_thread::sleep_for(t);
	}

	printf("client shut down...\n");

	std::chrono::milliseconds t(2000);
	std::this_thread::sleep_for(t);
	return 0;

}