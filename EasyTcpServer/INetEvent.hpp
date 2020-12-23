#ifndef _I_NETEVENT_HPP_
#define _I_NETEVENT_HPP_

#include "Init.h"

/**
*	net event interface
*/
class INetEvent
{
public:
	/**
	*	event while client join
	*/
	virtual void OnJoin(ChannelPtrRef pChannel) = 0;

	/**
	*	event while client leaves
	*/
	virtual void OnLeave(ChannelPtrRef pChannel) = 0;

	/**
	*	event while client's message comes
	*/
	virtual void OnNetMessage(WorkServer* pWorkServer, ChannelPtrRef pChannel, DataHeader* pheader) = 0;

	/**
	*	event while client's receive message
	*/
	virtual void OnNetRecv(ChannelPtrRef pChannel) = 0;
};


#endif
