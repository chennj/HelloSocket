#ifndef _CRC_WORK_SELECT_SERVER_HPP_
#define _CRC_WORK_SELECT_SERVER_HPP_

#include "crc_work_server.hpp"
#include "../common/include/crc_fdset.hpp"

class CRCWorkSelectServer : public CRCWorkServer
{
private:
	// fd set
	CRCFdSet _fd_read_bak;
	CRCFdSet _fd_read;
	CRCFdSet _fd_write;
	// max socket descriptor
	SOCKET _max_socket;
public:
	CRCWorkSelectServer(SOCKET sock = INVALID_SOCKET):
		CRCWorkServer(sock)
	{

	}
	~CRCWorkSelectServer()
	{
		Close();
	}

public:
	// select mode
	int DoAction()
	{
		int ret = 1;

		_fd_read.zero();
		_fd_write.zero();

		if (_clients_change)
		{
			_clients_change = false;

			_max_socket = _clients.begin()->first;
			for (auto iter : _clients)
			{
				_max_socket = iter.first > _max_socket ? iter.first : _max_socket;
				// the following statement needs to be optimized,
				// because it takes up a lot of CPU
				_fd_read.add(iter.first);
			}
			_fd_read_bak.copy(_fd_read);
		}
		else
		{
			_fd_read.copy(_fd_read_bak);
		}

		//memcpy(&fd_write, &_fd_read_bak, sizeof(fd_set));
		//memcpy(&fd_exception, &_fd_read_bak, sizeof(fd_set));
		// 优化可写检查，不直接拷贝，只有有了需要写数据的客户端
		// 才去加入到写监听集合里。避免cpu空转，产生大量的浪费。
		bool haveDataToWrite = false;
		for (auto iter : _clients)
		{
			if (iter.second->is_need_write())
			{
				haveDataToWrite = true;
				_fd_write.add(iter.first);
			}
		}


		//nfds is range of fd_set, not fd_set's count.
		//nfds is also max value+1 of all the file descriptor(socket).
		//nfds can be 0 in the windows.
		//that timeval was setted null means blocking, not null means nonblocking.
		timeval t = { 0,1 };
		if (haveDataToWrite)
		{
			ret = select(_max_socket + 1/*nfds*/, _fd_read.get_fd_set(), _fd_write.get_fd_set(), nullptr, &t);
		}
		else
		{
			ret = select(_max_socket + 1/*nfds*/, _fd_read.get_fd_set(), nullptr, nullptr, &t);
		}
		if (ret < 0)
		{
			CRCLogger::info("WorkServer socket<%d> error occurs while select and mission finish.\n", (int)_sock);
			return ret;
		}
		if (0 == ret)
		{
			return ret;
		}

		ReadData();
		WriteData();

		//#ifdef _WIN32
		//			WriteData(fd_exception);
		//			if (_tTime.getElapsedSecond() >= 1.0)
		//			{
		//				//if (fd_exception.fd_count > 0)
		//				//{
		//				//	xPrintf("###>>>WorkServer exception<%d>\n", fd_exception.fd_count);
		//				//}
		//				xPrintf("WorkServer readable<%d>, writable<%d>\n", fd_read.fd_count, fd_write.fd_count);
		//				_tTime.update();
		//			}
		//#endif
		return ret;
	}

	void WriteData()
	{
#ifdef _WIN32
		for (size_t n = 0; n < _fd_write.get_fd_set()->fd_count; n++)
		{
			auto iter = _clients.find(_fd_write.get_fd_set()->fd_array[n]);
			if (iter != _clients.end())
			{
				if (-1 == iter->second->SendDataIM())
				{
					OnClientLeave(iter);
				}
			}
			else
			{
				CRCLogger::info("incredible situation occurs while write data to client\n");
			}

		}
#else
		for (auto iter = _clients.begin(); iter != _clients.end(); iter++)
		{
			if (iter->second->is_need_write() && _fd_write.has(iter->first))
			{
				if (-1 == iter->second->SendDataIM())
				{
					OnClientLeave(iter);
				}
			}
		}
#endif

	}

	void ReadData()
	{
#ifdef _WIN32
		for (size_t n = 0; n < _fd_read.get_fd_set()->fd_count; n++)
		{
			auto iter = _clients.find(_fd_read.get_fd_set()->fd_array[n]);

			if (iter != _clients.end())
			{
				if (-1 == RecvData(iter->second))
				{
					OnClientLeave(iter);
				}
			}
			else
			{
				CRCLogger::info("incredible situation occurs while read data from client\n");
			}

		}
#else
		for (auto iter = _clients.begin(); iter != _clients.end(); iter++)
		{
			if (_fd_read.has(iter->first))
			{
				if (-1 == RecvData(iter->second))
				{
					OnClientLeave(iter);
				}
			}
		}
#endif
	}


};
#endif
