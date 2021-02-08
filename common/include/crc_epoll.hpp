#ifndef _CRC_EPOLL_HPP_
#define _CRC_EPOLL_HPP_

#ifdef __linux__

#include "crc_common.h"
#include "crc_logger.hpp"
#include <sys/epoll.h>

#define EPOLL_ERROR     (-1)

class CRCEpoll
{
private: 
    int _epfd;
    int _nMaxEvents;
    epoll_event* _pEvents;

public: 
    CRCEpoll()
    {
        _epfd = EPOLL_ERROR;
        _nMaxEvents = 0;
        _pEvents = nullptr;
    }
    ~CRCEpoll()
    {
        CRCLogger_Info("~CRCEpoll()\n");
        destory();
    }

public: 
    // create epoll object
    int create(int nMaxEvents = 10240)
    {
        if (_pEvents)
        {
            destory();
            CRCLogger_Warn("warning close old epoll\n");
        }

        _epfd = epoll_create(nMaxEvents/*已经没有意义，只是为了兼容更早的写法*/);
        if (EPOLL_ERROR == _epfd)
        {
			CRCLogger_Error("epoll_create error");
            throw std::runtime_error("CRCEpoll::epoll_create failed");
            return _epfd;
        }

        _nMaxEvents = nMaxEvents;
        _pEvents = new epoll_event[_nMaxEvents];
        return _epfd;
    }

    // epoll event controll
    int ctl(void* pChannel, int operation, SOCKET socket, uint32_t events)
    {
        // 定义服务端的socket并将其关联到EPOLLIN（可读）事件
        epoll_event event;
        //事件类型(比如可读-EPOLLIN)
        event.events = events;
        //此事件类型所关联的描述符对象
        event.data.ptr = pChannel;
        // epoll controll
        // 向epoll对象注册需要管理、监听、检测的socket
        // 并且说明关心的事件（events）
        int ret = epoll_ctl(_epfd, operation, socket, &event);    
        if (EPOLL_ERROR == ret)
        {
            CRCLogger_Error("error epoll_ctl(%d,%d,%d,%d)\n", _epfd, operation, socket, events);
        }
        return ret;
    }

    int wait(int timeout=0)
    {
        // 等待网络事件的到来，或者立即返回
        int ret_events = epoll_wait(
            _epfd,          // 由 epoll_create 建立的对象
            _pEvents,       // 保存在 _sock 和 events 上检测到的事件
            _nMaxEvents,    // 接收数组 events 的大小，不能超过，否则会数组越界
            timeout         // 等待超时。
                            // >0   ：如果没有事件，等待指定毫秒数后返回
                            // 0    ：有事件，就把事件装入 events ，没有就立即返回。
                            // -1   ：一直等待，直到有事件发生。
        );
        // 检查返回的事件数
        // < 0，表示出错
        // = 0，表示没有事件发生
        if (EPOLL_ERROR == ret_events)
        {
			// 如果编译的时候使用了 -pg（性能分析）参数,或者添加了一些其他的三方库,可能会出现 
			// Interrupted system call（EINTR）错误，需要忽略掉
			if (EINTR == errno)
			{
				return 0;
			}			
			CRCLogger_Error("epoll_wait error");
        }

        return ret_events;
    }

    epoll_event* get_epoll_events()
    {
        return _pEvents;
    }

    void destory()
    {
        if (_pEvents)
        {
            CRCLogger_Info("epoll object destroy\n");
            delete[] _pEvents;
            _pEvents = nullptr;
            close(_epfd);
            _epfd = -1;
        }
    }
};


#endif

#endif