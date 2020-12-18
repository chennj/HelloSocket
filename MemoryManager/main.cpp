#include<stdlib.h>
#include"Allocator.h"
#include<stdio.h>

int main()
{
	printf("---------------test1------------\n");
	char* data1 = new char[128];
	delete[] data1;

	char* data2 = new char;
	delete data2;

	char* data3 = new char[64];
	delete[] data3;

	printf("---------------test2------------\n");

	const int count = 12;
	char* pc[count];

	for (int i = 0; i < count; i++)
	{
		pc[i] = new char[32];
		delete[] pc[i];
	}

	printf("---------------test3------------\n");

	for (int i = 0; i < count; i++)
	{
		pc[i] = new char[65];
		delete[] pc[i];
	}

	printf("---------------test4------------\n");
	printf("--------------------step1-------\n");

	for (int i = 0; i < count; i++)
	{
		pc[i] = new char[32];
	}

	for (auto pv : pc)
	{
		delete pv;
	}

	printf("--------------------step2-------\n");

	for (int i = 0; i < count; i++)
	{
		pc[i] = new char[32];
	}

	for (int i = count - 1; i >= 0; i--)
	{
		delete[] pc[i];
	}

	printf("--------------------step3-------\n");

	for (int i = count-1; i >= 0; i--)
	{
		pc[i] = new char[32];
	}

	for (int i = 0; i < count; i++)
	{
		delete pc[i];
	}

	system("PAUSE");
	return 0;
}