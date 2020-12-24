#ifndef _TIMESTAMP_HPP_
#define _TIMESTAMP_HPP_

/* -------------
// This method is only used for windows

#include <Windows.h>

class Timestamp
{
public:
	Timestamp()
	{
		QueryPerformanceFrequency(&_frequency);
		QueryPerformanceCounter(&_startCount);
	}
	~Timestamp()
	{

	}

public:
	void update()
	{
		QueryPerformanceCounter(&_startCount);
	}

	// get current time(second)
	double getElapsedSecond()
	{
		return getElapsedTimeInMicroSec() * 0.000001;
	}

	// get current time(millisec)
	double getElapsedTimeInMilliSec()
	{
		return this->getElapsedTimeInMicroSec() * 0.001;
	}

	// get current time(microsec)
	double getElapsedTimeInMicroSec()
	{
		LARGE_INTEGER endCount;
		QueryPerformanceCounter(&endCount);

		double startTimeInMicroSec	= _startCount.QuadPart * (1000000.0 / _frequency.QuadPart);
		double endTimeInMicroSec	= endCount.QuadPart * (1000000.0 / _frequency.QuadPart);

		return endTimeInMicroSec - startTimeInMicroSec;
	}

protected:
	LARGE_INTEGER _startCount;
	LARGE_INTEGER _frequency;
};
*///--------------------------

/**
 * using c++11
 */

#include <chrono>

using namespace std::chrono;

class Time
{
public:
	// get current time stamp in milliseconds
	static time_t getNowInMilliSec()
	{
		return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}
};

class Timestamp
{
public:
	Timestamp()
	{
		update();
	}
	~Timestamp()
	{

	}

public:
	/**
	*	fresh current time
	*/
	void update()
	{
		_begin = high_resolution_clock::now();
	}

	/**
	*	get current time(second)
	*/
	double getElapsedSecond()
	{
		return getElapsedTimeInMicroSec() * 0.000001;
	}

	/**
	*	get current time(millisec)
	*/
	double getElapsedTimeInMilliSec()
	{
		return this->getElapsedTimeInMicroSec() * 0.001;
	}

	/**
	*	get current time(microsec)
	*/
	long long getElapsedTimeInMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

protected:
	time_point<high_resolution_clock> _begin;
};

#endif
