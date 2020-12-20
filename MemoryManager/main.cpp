#include"Allocator.h"
#include"CellTimestamp.hpp"
#include"AutoPtr.hpp"
#include<iostream>
#include<thread>
#include<mutex>
#include<memory>

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
}

class ClassA
{
private:

public:
	ClassA()
	{
		printf("ClassA\n");
	}
	~ClassA()
	{
		printf("~ClassA\n");
	}

public:
	int num;

public:
	void release() {}
	void clone() {}
};

void fun1(shared_ptr<ClassA> pa)
{
	pa->num++;
	printf("fun pa->num = %d\n",pa->num);
}

void fun2(AutoPtr<ClassA> pa)
{
	pa->num++;
	printf("fun pa->num = %d\n", pa->num);
}

int main()
{
	//thread t[tCount];
	//for (int n = 0; n < tCount; n++)
	//{
	//	t[n] = thread(workFun, n);
	//}
	//CellTimestamp tTime;
	//for (int n = 0; n < tCount; n++)
	//{
	//	t[n].join();
	//}
	//cout << "ÏûºÄÊ±¼ä(ºÁÃë)£º\t" << tTime.getElapsedTimeInMilliSec() << endl;

	//shared_ptr<int> b = make_shared<int>();
	//*b = 100;
	//printf("b=%d\n", *b);

	{
		shared_ptr<ClassA> cls_a = make_shared<ClassA>();
		cls_a->num = 100;
		fun1(cls_a);
	}

	{
		AutoPtr<ClassA> cls_a = new ClassA;
		cls_a->num = 100;
		fun2(cls_a);
	}

	system("PAUSE");
	return 0;

}