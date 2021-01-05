#ifndef _COMMON_DLL_H_
#define _COMMON_DLL_H_

extern "C"
{
	typedef void(*callback_on_net_message)(void* pByWhoCall, void* pData, int dataLen);
}

#include <string>

#include "crc_client.hpp"

#ifdef _WIN32
	#define EXPORT_DLL _declspec(dllexport)
#else
	#define EXPORT_DLL
#endif

extern "C"
{
	// ------------------------ test ------------------------------------------
	// ���ⲿϵͳ����
	EXPORT_DLL int export_test(int a, int b)
	{
		return a + b;
	}

	/**
	*
	*	�����ⲿϵͳ�ĺ���
	*	1.����һ������ָ��
	*	2.ִ�д���Ļص�����
	*/
	typedef void(*callback1)(const char* s);

	EXPORT_DLL void callback1_test(callback1 callback, const char* s)
	{
		std::string str = "Hello,";
		str += s;
		callback(str.c_str());
	}

	// ------------------------ code ------------------------------------------

	EXPORT_DLL void* CRCClient_Create(callback_on_net_message CallbackOnNetMessage, void* pByWhoCall)
	{
		CRCClient* pClient = new CRCClient();
		pClient->set_callback(CallbackOnNetMessage);
		pClient->set_bywhocall(pByWhoCall);

		return pClient;
	}

	EXPORT_DLL bool CRCClient_Connect(CRCClient* pClient, const char* ip, short port)
	{
		if (pClient && ip)
			return SOCKET_ERROR != pClient->Connect(ip, port);
		return false;
	}

	EXPORT_DLL bool CRCClient_OnRun(CRCClient* pClient)
	{
		if (pClient)
			return pClient->OnRun();
		return false;
	}

	EXPORT_DLL int CRCClient_SendData(CRCClient* pClient, void* pData, int dataLen)
	{
		if (pClient)
			return pClient->SendData((const char*)pData, dataLen);
		return 0;
	}

	EXPORT_DLL void CRCClient_Close(CRCClient* pClient)
	{
		if (pClient)
		{
			pClient->Close();
			delete pClient;
		}
	}
}

#endif