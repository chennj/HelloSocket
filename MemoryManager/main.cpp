#include<stdlib.h>
#include"Alloctor.h"
//#include"MemoryMgr.hpp"

int main()
{
	char * data1 = new char[128];
	delete[] data1;

	char * data2 = new char;
	delete data2;

	char * data3 = (char*)mem_alloc(64);
	mem_free(data3);

	//MemoryMgr::destory();

	system("PAUSE");
	return 0;
}