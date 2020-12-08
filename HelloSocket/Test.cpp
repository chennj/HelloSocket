#define WIN32_LEAN_AND_MEAN	//try import small def loog ago

#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>
#include<stdlib.h>

//可以配置再工程-属性-链接器-输入中
//#pragma comment(lib, "ws2_32.lib")

void func(char str[100])
{
	printf("str size=%d\n", sizeof(str));
}
int main()
{
	//WORD ver = MAKEWORD(2, 2);
	//WSADATA data;
	//WSAStartup(ver, &data);

	//---------
	//-- create sample tcp client use socket api
	// 1 create socket
	// 2 connect server
	// 3 recv from server
	// 4 close socket
	//---------
	//-- create sample tcp server use socket api
	// 1 create socket
	// 2 bind port, which is be connected by client
	// 3 listen port
	// 4 accept a client's connection
	// 5 send message to client
	// 6 close socket
	// clean windows socket enviament
	//WSACleanup();

	int i = 1;
	int j = 2;
	int n = i++ + j;
	printf("n=%d\n", n);

	const int ii = -80.;
	char str[100];
	int n1 = sizeof(str);
	void *p = malloc(100);
	int pn = sizeof(p);

	func("234");

	char s1[2] = "a";
	char s2[2] = "b";
	printf("s1 strcmp s2 = %d\n", strcmp(s1, s2));

	int a[] = { 1,2,3,4,5 };
	int *pa = a;
	printf("++*pa=%d equals ++a[0] = %d\n", ++*pa,++a[0]);
	getchar();

	return 0;
}