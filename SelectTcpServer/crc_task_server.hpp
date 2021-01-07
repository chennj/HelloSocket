#ifndef _CRC_TASK_SERVER_HPP_
#define _CRC_TASK_SERVER_HPP_

#include "crc_init.h"
#include "../common/include/crc_thread.hpp"
#include "../common/include/crc_logger.hpp"

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
	CRCThread _crcThread;
public:
	CRCTaskServer()
	{
	}
	~CRCTaskServer()
	{
		Close();
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
		_crcThread.Start
		(
			nullptr,
			[this](CRCThread* pCrcThread){ OnRun(pCrcThread); },
			nullptr
		);
	}

	void Close()
	{
		if (_crcThread.IsRun())_crcThread.Close();

		// 有可能循环结束时，任务没有执行完
		for (auto fTask : _tasksBuf)
		{
			fTask();
		}
	}

	// work loop
	void OnRun(CRCThread* threadPtr)
	{
		CRCLogger::info("TaskServer thread start...\n");

		while (threadPtr->IsRun())
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

		CRCLogger::info("TaskServer thread exit...\n");
	}
};

#endif