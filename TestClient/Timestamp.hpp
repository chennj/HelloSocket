#ifndef _TIMESTAMP_HPP_
#define _TIMESTAMP_HPP_

/* -------------
// This method is only used for windows

#include <Windows.h>

class CellTimestamp
{
public:
	CellTimestamp()
	{
		QueryPerformanceFrequency(&_frequency);
		QueryPerformanceCounter(&_startCount);
	}
	~CellTimestamp()
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
class CellTimestamp
{
public:
	CellTimestamp()
	{
		update();
	}
	~CellTimestamp()
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
