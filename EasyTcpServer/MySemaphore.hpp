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
		// �����︳ֵ����������while���ǰ��ֵ
		// ��Ϊ�˷�ֹwakeup��wait��ǰ��ִ�ж����µ�����
		_isWaitExit = true;
	}

	void wakeup()
	{
		_isWaitExit = false;
	}
};

#endif
