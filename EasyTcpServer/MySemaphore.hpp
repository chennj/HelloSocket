#ifndef _MY_SEMAPHORE_HPP_
#define _MY_SEMAPHORE_HPP_

#include <thread>
#include <chrono>

/**
* simple semaphore
*/
class MySemaphore
{
private:
	int _isWaitExit;

public:
	MySemaphore()
	{
		_isWaitExit = true;
	}

	~MySemaphore()
	{

	}

public:
	void wait()
	{
		while (_isWaitExit)
		{
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
		}
		// 在这里赋值，而不是在while语句前赋值
		// 是为了防止wakeup在wait的前面执行而导致的死锁
		_isWaitExit = true;
	}

	void wakeup()
	{
		_isWaitExit = false;
	}
};

#endif
