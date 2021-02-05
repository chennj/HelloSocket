//#include "../common/include/crc_allocator.h"
//#include "../common/include/crc_memory_pool.hpp"
//#include "../common/include/crc_object_pool.hpp"
#include"Allocator.h"
#include"CellTimestamp.hpp"
#include"ObjectPool.hpp"
#include"AutoPtr.hpp"
#include<iostream>
#include<thread>
#include<mutex>
#include<memory>

using namespace std;

mutex m;
const int tCount = 8;
const int mCount = 32;
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

class ClassBOfA : public ObjectPoolBase<ClassBOfA, 10>
{
public:
	ClassBOfA()
	{
		_num = 0;
		_num1 = 0;
		printf("ClassBOfA\n");
	}
	ClassBOfA(int num)
	{
		_num = num;
		_num1 = 0;
		printf("ClassBOfA\n");
	}
	ClassBOfA(int num, int num1)
	{
		_num = num;
		_num1 = num1;
		printf("ClassBOfA\n");
	}
	~ClassBOfA()
	{
		printf("~ClassBOfA\n");
	}

public:
	int _num;
	int _num1;
};

class ClassA
{
private:
	ClassBOfA* pClassBOfA;
public:
	ClassA()
	{
		printf("ClassA\n");
		pClassBOfA = new ClassBOfA();
	}
	~ClassA()
	{
		printf("~ClassA\n");
		delete pClassBOfA;
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

class ClassB : public ObjectPoolBase<ClassB, 10>
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

class ClassC : public ObjectPoolBase<ClassC, 10>
{
public:
	ClassC()
	{
		_num = 0;
		_num1 = 0;
		printf("ClassC\n");
	}
	ClassC(int num)
	{
		_num = num;
		_num1 = 0;
		printf("ClassC\n");
	}
	ClassC(int num, int num1)
	{
		_num = num;
		_num1 = num1;
		printf("ClassC\n");
	}
	~ClassC()
	{
		printf("~ClassC\n");
	}

public:
	int _num;
	int _num1;
};


class ClassD : public ObjectPoolBase<ClassD, 5>
{
public:
	ClassD()
	{
		_num = 0;
		_num1 = 0;
		printf("ClassD\n");
	}
	ClassD(int num)
	{
		_num = num;
		_num1 = 0;
		printf("ClassD\n");
	}
	ClassD(int num, int num1)
	{
		_num = num;
		_num1 = num1;
		printf("ClassD\n");
	}
	~ClassD()
	{
		printf("~ClassD\n");
	}

public:
	int _num;
	int _num1;
};

void workFunC(int index)
{
	ClassC* data[nCount];

	for (int i = 0; i < nCount; i++)
	{
		data[i] = ClassC::create_object(rand() % 1024, rand() % 1024);
	}

	for (auto pv : data)
	{
		ClassC::destory_object(pv);
	}
}

int main()
{
	//char path[] = { "/log/server.log" };
	//CRCLogger::instance().set_log_path(path, "w");
	//CRCLogger::instance().start();

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


	{
		printf("1.--------------------------------------\n");
		shared_ptr<ClassA> cls_a = make_shared<ClassA>();
		cls_a->num = 100;
		fun(cls_a);
	}

	{
		printf("2.--------------------------------------\n");
		AutoPtr<ClassA> cls_a = new ClassA;
		cls_a->num = 100;
		//有缺陷，如果和内存池一起用会出现两次删除同一地址而崩溃
		//fun(cls_a);
	}

	{
		printf("3.--------------------------------------\n");
		ClassA* cls_a = new ClassA;
		cls_a->num = 100;
		delete cls_a;
	}


	/*
	ClassA* pa1 = new ClassA();
	delete pa1;
	ClassA* pa2 = ClassA::create_object();
	ClassA::destory_object(pa2);
	*/

	/*
	ClassB* pb1 = new ClassB();
	printf("ClassB num=%d,num1=%d\n", pb1->_num, pb1->_num1);
	delete pb1;
	ClassB* pb2 = ClassB::create_object(5);
	printf("ClassB num=%d,num1=%d\n", pb2->_num, pb2->_num1);
	ClassB::destory_object(pb2);
	ClassB* pb3 = ClassB::create_object(5,6);
	printf("ClassB num=%d,num1=%d\n", pb3->_num, pb3->_num1);
	ClassB::destory_object(pb3);
	printf("---------------------------------\n");
	ClassC* pc1 = new ClassC();
	printf("ClassC num=%d,num1=%d\n", pc1->_num, pc1->_num1);
	ClassC* pc2 = ClassC::create_object(50);
	printf("ClassC num=%d,num1=%d\n", pc2->_num, pc2->_num1);
	ClassC* pc3 = ClassC::create_object(50, 60);
	printf("ClassC num=%d,num1=%d\n", pc3->_num, pc3->_num1);
	delete pc1;
	ClassC::destory_object(pc3);
	ClassC::destory_object(pc2);
	*/

	/*
	thread t[tCount];
	for (int n = 0; n < tCount; n++)
	{
		t[n] = thread(workFunC, n);
	}
	CellTimestamp tTime;
	for (int n = 0; n < tCount; n++)
	{
		t[n].join();
	}
	cout << "消耗时间(毫秒)：\t" << tTime.getElapsedTimeInMilliSec() << endl;
	*/

	/*
	// will skip object pool if creating shared pointer by this way,but only call new once and efficiency
	{
		shared_ptr<ClassD> s1 = make_shared<ClassD>(1, 2);
	}
	printf("-------------------------------------------------------\n");
	// so we should creating shared pointer by this way,but this way will call new twice and inefficiency
	{
		shared_ptr<ClassD> s1(new ClassD(1, 2));
	}
	*/
	system("PAUSE");
	return 0;

}