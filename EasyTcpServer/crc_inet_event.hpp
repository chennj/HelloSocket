#ifndef _CRC_I_NET_EVENT_HPP_
#define _CRC_I_NET_EVENT_HPP_

#include "crc_init.h"

/**
*	net event interface
*/
class CRCINetEvent
{
public:
	/**
	*	event while client join
	*/
	virtual void OnJoin(CRCChannel* pChannel) = 0;

	/**
	*	event while client leaves
	*/
	virtual void OnLeave(CRCChannel* pChannel) = 0;

	/**
	*	event while client's message comes
	*/
	virtual void OnNetMessage(CRCWorkServer* pWorkServer, CRCChannel* pChannel, CRCDataHeader* pheader) = 0;

	/**
	*	event while client's receive message
	*/
	virtual void OnNetRecv(CRCChannel* pChannel) = 0;
};


#endif
