#ifndef _CELLTASK_HPP_
#define _CELLTASK_HPP_

#include<thread>
#include<mutex>
#include<list>

class ITask
{
private:

public:
	ITask()
	{

	}
	virtual ~ITask()
	{

	}

public:
	virtual void doTask() = 0;
};

class TaskServer
{
private:
	std::list<ITask*> _tasks;
	std::list<ITask*> _tasksBuf;
	std::mutex _mutex;
public:
	TaskServer()
	{

	}
	~TaskServer()
	{

	}

public:
	// add task
	void addTask(ITask* pCellTask)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(pCellTask);
	}

	// start service thread
	void Start()
	{
		std::thread t(std::mem_fn(&TaskServer::OnRun), this);
		t.detach();
	}

	// work loop
	void OnRun()
	{
		while (true)
		{
			// take task from task buff to task
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf)
				{
					_tasks.push_back(pTask);
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


			for (auto pTask : _tasks)
			{
				pTask->doTask();
				delete pTask;
			}
			// clear task;
			_tasks.clear();

		}
	}
};
#endif