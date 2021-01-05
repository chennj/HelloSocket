#ifndef CRC_STREAM_MESSAGE_HPP
#define CRC_STREAM_MESSAGE_HPP

#include "crc_stream.hpp"
#include "crc_message_header.hpp"

/**
*
*	receive byte stream
*/
class CRCRecvStream : public CRCStream
{
public:
	int _cmd;
	uint16_t _streamLen;

public:
	CRCRecvStream(CRCDataHeader* pHeader):
		CRCStream((char*)pHeader, pHeader->data_length)
	{
		push(pHeader->data_length);

		// 预先读取消息流类型
		_cmd = get_cmd();
		// 预先读取消息流长度
		read<uint16_t>(_streamLen);
	}

	~CRCRecvStream()
	{
	}

public:
	uint16_t get_cmd()
	{
		uint16_t cmd = 0;
		read<uint16_t>(cmd, CMD_UNKNOWN);
		return cmd;
	}
};

/**
*
*	send byte stream
*/
class CRCSendStream : public CRCStream
{
private:

public:
	CRCSendStream(char* pData, int nSize, bool bDelete = false) :
		CRCStream(pData, nSize, bDelete)
	{
		// 预先占据消息长度所需的空间
		write<uint16_t>(0);
	}

	CRCSendStream(int nSize = 1024) :
		CRCStream(nSize)
	{
		// 预先占据消息长度所需的空间
		write<uint16_t>(0);
	}

	~CRCSendStream()
	{
	}

public:
	void finish()
	{
		int pos = get_write_pos();
		set_write_pos(0);
		write<uint16_t>(pos);
		set_write_pos(pos);
	}
	
	void set_cmd(uint16_t cmd)
	{
		write<uint16_t>(cmd);
	}
};
#endif