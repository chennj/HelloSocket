#include"Allocator.h"
#include"CellTimestamp.hpp"
#include"AutoPtr.hpp"
#include"ObjectPool.hpp"
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

public:
	/**
	*	有了这两个函数，ClassA的new和delete将不再使用内存池的
	*	new和delete
	*/
	void* operator new(size_t size)
	{
		printf("self new\n");
		return malloc(size);
	}

	void operator delete(void * pv)
	{
		printf("self delete\n");
		free(pv);
	}

public:
	static ClassA* create_object()
	{
		ClassA* pobj = new ClassA();
		return pobj;
	}

	static void destory_object(ClassA* pobj)
	{
		delete pobj;
	}
};

void fun(shared_ptr<ClassA> pa)
{
	pa->num++;
	printf("fun pa->num = %d\n",pa->num);
}

void fun(AutoPtr<ClassA> pa)
{
	pa->num++;
	printf("fun pa->num = %d\n", pa->num);
}

class ClassB : public ObjectPoolBase<ClassB>
{
public:
	ClassB()
	{
		_num = 0;
		_num1 = 0;
		printf("ClassB\n");
	}
	ClassB(int num)
	{
		_num = num;
		_num1 = 0;
		printf("ClassB\n");
	}
	ClassB(int num, int num1)
	{
		_num = num;
		_num1 = num1;
		printf("ClassB\n");
	}
	~ClassB()
	{
		printf("~ClassB\n");
	}

public:
	int _num;
	int _num1;
};

int main()
{
	/*
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
	cout << "消耗时间(毫秒)：\t" << tTime.getElapsedTimeInMilliSec() << endl;

	shared_ptr<int> b = make_shared<int>();
	*b = 100;
	printf("b=%d\n", *b);
	*/

	/*
	{
		shared_ptr<ClassA> cls_a = make_shared<ClassA>();
		cls_a->num = 100;
		fun(cls_a);
	}

	{
		AutoPtr<ClassA> cls_a = new ClassA;
		cls_a->num = 100;
		//有缺陷，如果和内存池一起用会出现两次删除同一地址而崩溃
		//fun(cls_a);
	}
	*/

	/*
	ClassA* pa1 = new ClassA();
	delete pa1;
	ClassA* pa2 = ClassA::create_object();
	ClassA::destory_object(pa2);
	*/

	ClassB* pa1 = new ClassB();
	printf("ClassB num=%d,num1=%d\n", pa1->_num, pa1->_num1);
	delete pa1;
	ClassB* pa2 = ClassB::create_object(5);
	printf("ClassB num=%d,num1=%d\n", pa2->_num, pa2->_num1);
	ClassB::destory_object(pa2);
	ClassB* pa3 = ClassB::create_object(5,6);
	printf("ClassB num=%d,num1=%d\n", pa3->_num,pa3->_num1);
	ClassB::destory_object(pa3);

	system("PAUSE");
	return 0;

}