﻿// linux compile command
// g++ client.cpp -std=c++11 -pthread -o client
// ----------------------------------
#include "crc_client.hpp"

#include <thread>
#include <chrono>
#include <atomic>

bool g_run = true;

// sending thread amount
const int tCount = 2;
// client amount
const int nCount = 100;

// client object
CRCClient* pclients[nCount];

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
	int ret = 0;
	while (g_run)
	{
		for (int n = begin; n < end; n++)
		{
			//if (t.getElapsedSecond() > 4.0 && n == begin) {
			//	//模拟一个客户端接收被堵塞的情况，服务器的运行情况
			//	//期望：对于没有被堵塞的客户端运行正常
			//	continue;
			//}

			ret = pclients[n]->OnRun();
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
		pclients[n] = new CRCClient;
	}

	for (int n = begin; n < end; n++)
	{
		if (!g_run)return;
		// 192.168.137.129 ubuntu
		// 127.0.0.1
		pclients[n]->Connect("127.0.0.1", 12345);
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

	int one_print = 1;
	int one = end - begin;
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
				if (SOCKET_ERROR != pclients[n]->SendData(login))
				{
					sendCount++;
				}
			}
			else {
				if (one_print <= one) {
					printf("server close one client\n");
					one_print++;
				}
			}
		}

		// to control speed to send
		std::chrono::microseconds t(100);
		std::this_thread::sleep_for(t);
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
	/*
	CRCLogger::instance().set_log_path("/log/client.log", "w");
	CRCLogger::instance().start();

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
	return 0；
	*/

	CRCSendStream s;
	s.set_cmd(CMD_STREAM);
	s.write_int8(1);
	s.write_int16(2);
	s.write_int32(3);
	s.write_float(4.1f);
	s.write_double(5.2);
	char* ss = "cnj";
	s.write_array(ss, strlen(ss));
	char* str = "i am client";
	s.WriteString(str);
	int a[] = { 1,2,3,4 };
	s.write_array(a, 4);
	s.finish();

	//CRCRecvStream r((CRCDataHeader*)s.data());
	//printf("read %d\n", r.read_int8());
	//printf("read %d\n", r.read_int16());
	//printf("read %d\n", r.read_int32());
	//printf("read %lf\n", r.read_float());
	//printf("read %lf\n", r.read_double());
	//char a1[16] = {};
	//printf("read %d, %s\n", r.read_array(a1, 16), a1);
	//char a2[16] = {};
	//printf("read %d, %s\n", r.read_array(a2, 16), a2);
	//int a3[10] = {};
	//printf("read %d\n", r.read_array(a3, 10));

	// 192.168.137.129
	// 127.0.0.1
	CRCClient client;
	client.Connect("127.0.0.1", 12345);
	while (client.IsRunning())
	{
		client.OnRun();
		client.SendData(s.data(), s.data_length());
		CRCThread::SleepForMilli(1000);
	}

	system("PAUSE");
}