#ifndef _CRC_THREAD_HPP_
#define _CRC_THREAD_HPP_

#include "crc_semaphore.hpp"

class CRCThread
{
private:
	typedef std::function<void(CRCThread*)> EVENTCALL;

private:
	// thread event
	EVENTCALL _onCreate;
	EVENTCALL _onRun;
	EVENTCALL _onDestory;

	// semaphore that controll thread to end
	CRCSemaphore _sem;
	// thread pointer
	std::thread* _pThread;
	// flag whether running
	bool _isRun;
	// lock
	std::mutex _mutex;

public:
	CRCThread()
	{
		_isRun		= false;

		_onCreate	= nullptr;
		_onRun		= nullptr;
		_onDestory	= nullptr;
	}
	~CRCThread()
	{
	}

public:
	// start thread
	void Start(EVENTCALL onCreate = nullptr, EVENTCALL onRun = nullptr, EVENTCALL onDestory = nullptr)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (_isRun)
		{
			printf("thread is running\n");
			return;
		}

		_isRun = true;

		_onCreate = onCreate;
		_onRun = onRun;
		_onDestory = onDestory;

		_pThread = new std::thread(std::mem_fn(&CRCThread::OnWork), this);
		_pThread->detach();
	}

	// close thread
	void Close()
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (_isRun)
		{
			_isRun = false;
			_sem.wait();
		}
	}

	void ExitInSelfThread()
	{
		std::lock_guard<std::mutex> lg(_mutex);
		_isRun = false;
	}

	bool IsRun()
	{
		return _isRun;
	}

protected:
	// work function while thread running
	void OnWork()
	{
		if (_onCreate && _isRun)_onCreate(this);
		if (_onRun && _isRun)_onRun(this);
		if (_onDestory && _isRun)_onDestory(this);

		_sem.wakeup();
	}
};


#endif
