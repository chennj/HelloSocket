#ifndef _CRC_TASK_SERVER_HPP_
#define _CRC_TASK_SERVER_HPP_

#include "crc_semaphore.hpp"

#include<thread>
#include<mutex>
#include<list>
#include<functional>

class CRCTaskServer
{
	typedef std::function<void()> FTask;
private:
	std::list<FTask> _tasks;
	std::list<FTask> _tasksBuf;
	std::mutex _mutex;
	bool _isRunning;
	std::thread* _pThread;
	CRCSemaphore _sem;
public:
	CRCTaskServer()
	{
		_pThread = nullptr;
		_isRunning = true;
	}
	~CRCTaskServer()
	{
		printf("TaskServer destory.\n");
		_isRunning = false;
		// waitting for child-thread(OnRun) exit
		_sem.wait();
		// release resources
		_tasks.clear();
		_tasksBuf.clear();
		// set null
		_pThread = nullptr;
	}

public:
	// add task
	void addTask(FTask fTask)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(fTask);
	}

	// start service thread
	void Start()
	{
		//std::thread t(std::mem_fn(&TaskServer::OnRun), this);
		//t.detach();
		if (!_pThread)
		{
			_pThread = new std::thread(std::mem_fn(&CRCTaskServer::OnRun), this);
			_pThread->detach();
		}
	}

	// work loop
	void OnRun()
	{
		while (_isRunning)
		{
			// take task from task buff to task
			if (_tasksBuf.size()>0)
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto fTask : _tasksBuf)
				{
					_tasks.push_back(fTask);
				}
				_tasksBuf.clear();
			}

			// if have not task
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}


			for (auto fTask : _tasks)
			{
				fTask();
			}
			// clear task;
			_tasks.clear();
		}

		// notice WorkServer main thread that child-thread(OnRun) had exit
		_sem.wakeup();
		printf("TaskServer thread exit...\n");
	}
};

#endif