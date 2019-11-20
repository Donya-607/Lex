#ifndef _INCLUDED_HIGH_RESOLUTION_TIMER_H_
#define _INCLUDED_HIGH_RESOLUTION_TIMER_H_

#include <windows.h>

class HighResolutionTimer
{
private:
	double secondsPerCount;
	double deltaTime;

	LONGLONG baseTime;
	LONGLONG pausedTime;
	LONGLONG stopTime;
	LONGLONG lastTime;
	LONGLONG thisTime;

	bool stopped;
public:
	HighResolutionTimer() :
		secondsPerCount( 0.0 ), deltaTime( -1.0 ),
		baseTime(), pausedTime( 0 ), stopTime(),
		lastTime(), thisTime(),
		stopped( false )
	{
		LONGLONG counts_per_sec;
		QueryPerformanceFrequency( reinterpret_cast<LARGE_INTEGER *>( &counts_per_sec ) );
		secondsPerCount = 1.0 / static_cast<double>( counts_per_sec );

		QueryPerformanceCounter( reinterpret_cast<LARGE_INTEGER *>( &thisTime ) );
		baseTime = thisTime;
		lastTime = thisTime;
	}

	/// <summary>
	/// Returns the total time elapsed since Reset() was called,<para></para>
	/// NOT counting any time when the clock is stopped.
	/// </summary>
	float TimeStamp() const  // in seconds
	{
		// If we are stopped, do not count the time that has passed since we stopped.
		// Moreover, if we previously already had a pause, the distance 
		// stop_time - base_time includes paused time, which we do not want to count.
		// To correct this, we can subtract the paused time from mStopTime:  
		//
		//                     |<--paused_time-->|
		// ----*---------------*-----------------*------------*------------*------> time
		//  base_time       stop_time        start_time     stop_time    this_time

		if ( stopped )
		{
			return static_cast<float>( ( ( stopTime - pausedTime ) - baseTime ) * secondsPerCount );
		}

		// The distance this_time - mBaseTime includes paused time,
		// which we do not want to count.  To correct this, we can subtract 
		// the paused time from this_time:  
		//
		//  (this_time - paused_time) - base_time 
		//
		//                     |<--paused_time-->|
		// ----*---------------*-----------------*------------*------> time
		//  base_time       stop_time        start_time     this_time
		else
		{
			return static_cast<float>( ( ( thisTime - pausedTime ) - baseTime ) * secondsPerCount );
		}
	}

	/// <summary>
	/// in seconds
	/// </summary>
	float TimeInterval() const
	{
		return static_cast<float>( deltaTime );
	}

	/// <summary>
	/// Call before message loop.
	/// </summary>
	void Reset()
	{
		QueryPerformanceCounter( reinterpret_cast<LARGE_INTEGER *>( &thisTime ) );
		baseTime = thisTime;
		lastTime = thisTime;

		stopTime = 0;
		stopped = false;
	}

	/// <summary>
	/// Call when unpaused.
	/// </summary>
	void Start()
	{
		LONGLONG startTime;
		QueryPerformanceCounter( reinterpret_cast<LARGE_INTEGER *>( &startTime ) );

		// Accumulate the time elapsed between stop and start pairs.
		//
		//                     |<-------d------->|
		// ----*---------------*-----------------*------------> time
		//  base_time       stop_time        start_time     
		if ( stopped )
		{
			pausedTime += ( startTime - stopTime );
			lastTime = startTime;
			stopTime = 0;
			stopped = false;
		}
	}

	/// <summary>
	/// Call when paused.
	/// </summary>
	void Stop()
	{
		if ( !stopped )
		{
			QueryPerformanceCounter( reinterpret_cast<LARGE_INTEGER *>( &stopTime ) );
			stopped = true;
		}
	}

	/// <summary>
	/// Please call every frame.
	/// </summary>
	void Tick()
	{
		if ( stopped )
		{
			deltaTime = 0.0;
			return;
		}

		QueryPerformanceCounter( reinterpret_cast<LARGE_INTEGER *>( &thisTime ) );
		// Time difference between this frame and the previous.
		deltaTime = ( thisTime - lastTime ) * secondsPerCount;

		// Prepare for next frame.
		lastTime = thisTime;

		// Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the 
		// processor goes into a power save mode or we get shuffled to another
		// processor, then mDeltaTime can be negative.
		if ( deltaTime < 0.0 )
		{
			deltaTime = 0.0;
		}
	}
};

#endif // _INCLUDED_HIGH_RESOLUTION_TIMER_H_