#ifndef _CRC_CLIENT_HPP_
#define _CRC_CLIENT_HPP_

#include "crc_init.h"
#include "crc_work_client.hpp"

class CRCClient : public CRCWorkClient
{
private:
	void* _pByWhoCall;
	callback_on_net_message _callback;

public:
	CRCClient()
	{
		_pByWhoCall = nullptr;
		_callback = nullptr;
	}
	~CRCClient()
	{

	}
	// overwrite parent function
public:
	void OnNetMessage(CRCDataHeader* pHeader)
	{
		if (_callback)
		{
			_callback(_pByWhoCall, pHeader, pHeader->data_length);
		}
	}

public:
	void set_callback(callback_on_net_message callback)
	{
		_callback = callback;
	}

	void set_bywhocall(void* pByWhoCall)
	{
		_pByWhoCall = pByWhoCall;
	}
};

#endif

