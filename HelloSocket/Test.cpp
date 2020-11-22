#define WIN32_LEAN_AND_MEAN	//try import small def loog ago

#include<Windows.h>
#include<WinSock2.h>

//可以配置再工程-属性-链接器-输入中
//#pragma comment(lib, "ws2_32.lib")

int main()
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);

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
	WSACleanup();

	return 0;
}