#ifndef _CELLTASK_HPP_
#define _CELLTASK_HPP_

#include<thread>
#include<mutex>
#include<list>
#include<functional>

class TaskServer
{
	typedef std::function<void()> FTask;
private:
	std::list<FTask> _tasks;
	std::list<FTask> _tasksBuf;
	std::mutex _mutex;
	bool _isRunning;
	std::thread* _pThread;
public:
	TaskServer()
	{
		_pThread = nullptr;
		_isRunning = true;
	}
	~TaskServer()
	{
		_isRunning = false;
		if (_pThread->joinable())
		{
			_pThread->join();
		}
		_pThread = nullptr;
		_tasks.clear();
		_tasksBuf.clear();
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
			_pThread = new std::thread(std::mem_fn(&TaskServer::OnRun), this);
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
		printf("TaskServer thread exit...\n");
	}
};

#endif