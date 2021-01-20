#ifndef _CRC_WORK_SELECT_CLIENT_HPP_
#define _CRC_WORK_SELECT_CLIENT_HPP_

#include "crc_work_client.hpp"
#include "crc_fdset.hpp"

class CRCWorkSelectClient : public CRCWorkClient
{
private:
	// fd set
	CRCFdSet _fd_read;
	CRCFdSet _fd_write;
	// max socket descriptor
	SOCKET _max_socket;
public:
	CRCWorkSelectClient()
	{

	}
	virtual ~CRCWorkSelectClient()
	{
		Close();
	}

public:
    // Override from CRCWorkClient
    int OnRun(int microseconds=1)
    {
        if (!IsRunning()) return -1;

		SOCKET sock = _pChannel->sockfd();

		_fd_read.zero();
		_fd_read.add(sock);


		_fd_write.zero();
		timeval t = { 0,microseconds };
		int ret;
		if (_pChannel->is_need_write())
		{
			_fd_write.add(sock);
			ret = select(sock + 1, _fd_read.get_fd_set(), _fd_write.get_fd_set(), nullptr, &t);
		}
		else
		{
			ret = select(sock + 1, _fd_read.get_fd_set(), nullptr, nullptr, &t);
		}
		 
		if (ret < 0)
		{
            if (EINTR == errno) return 0;
			CRCLogger_Error("socket<%d> CRCWorkSelectClient::OnRun select exit\n",(int)sock);
			Close();
			return ret;
		}

		if (_fd_read.has(sock))
		{
			int ret = RecvData();
			if (-1 == ret)
			{
                CRCLogger_Error("socket<%d> CRCWorkSelectClient::OnRun RecvData exit\n",(int)sock);
				Close();
				return ret;
			}
		}

		if (_fd_write.has(sock))
		{
			int ret = _pChannel->SendDataIM();
			if (-1 == ret)
			{
                CRCLogger_Error("socket<%d> CRCWorkSelectClient::OnRun SendDataIM exit\n",(int)sock);
				Close();
				return ret;
			}
		}

		return 0;
    }

    // Override from CRCWorkClient
    void OnNetMessage(CRCDataHeader* header)
    {

    }
};
#endif
