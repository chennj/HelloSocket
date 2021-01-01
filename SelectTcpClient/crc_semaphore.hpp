#ifndef _CRC_SEMAPHORE_HPP_
#define _CRC_SEMAPHORE_HPP_

#include <thread>
#include <chrono>
#include <mutex>

#include <condition_variable>

/**
* simple semaphore
*/
class CRCSemaphore
{
private:
	// wait count
	int _waitCount;
	// wakeup count
	int _wakeupCount;
	// lock data share area
	std::mutex _mutex;
	// condition variable be used to blocking and waitting
	std::condition_variable _condv;
public:
	CRCSemaphore()
	{
		_waitCount = 0;
		_wakeupCount = 0;
	}

	~CRCSemaphore()
	{

	}

public:
	void wait()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (--_waitCount < 0)
		{
			_condv.wait(lock, [this]() ->bool{
				return _wakeupCount > 0;
			});
			--_wakeupCount;
		}
	}

	void wakeup()
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (++_waitCount <= 0)
		{
			++_wakeupCount;
			_condv.notify_one();
		}

	}
};

#endif
