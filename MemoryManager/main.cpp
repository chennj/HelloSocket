#include"Allocator.h"
#include"CellTimestamp.hpp"
#include<iostream>
#include<thread>
#include<mutex>

using namespace std;

mutex m;
const int tCount = 4;
const int mCount = 1000 * 100;
const int nCount = mCount / tCount;

void workFun(int index)
{
	char* data[nCount];

	for (int i = 0; i < nCount; i++)
	{
		data[i] = new char[1 + rand()%1024];
	}

	for (auto pv : data)
	{
		delete pv;
	}

	//for (int n = 0; n < nCount; n++)
	//{
	//	lock_guard<mutex> lg(m);
	//}
}

int main()
{
	thread t[tCount];
	for (int n = 0; n < tCount; n++)
	{
		t[n] = thread(workFun, n);
	}

	CellTimestamp tTime;
	for (int n = 0; n < tCount; n++)
	{
		t[n].join();
	}

	cout << "ÏûºÄÊ±¼ä(ºÁÃë)£º\t" << tTime.getElapsedTimeInMilliSec() << endl;

	system("PAUSE");
	return 0;

}