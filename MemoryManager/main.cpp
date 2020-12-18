#include<stdlib.h>
#include"Allocator.h"
#include<stdio.h>

int main()
{
	const int count = 1025;
	char* pc[count];

	for (int i = 0; i < count; i++)
	{
		pc[i] = new char[1+i];
	}

	for (auto pv : pc)
	{
		delete pv;
	}

	system("PAUSE");
	return 0;
}