#ifndef INCLUDED_PROCESS_TIMER_H_
#define INCLUDED_PROCESS_TIMER_H_

#include <Windows.h>

class Benchmark
{
public:
	inline static bool IsSupportedQueryPerformance()
	{
		LARGE_INTEGER check{};
		return ( QueryPerformanceFrequency( &check ) ? true : false );
	}
private:
	LARGE_INTEGER	frequency;
	LARGE_INTEGER	start;
	LARGE_INTEGER	current;
public:
	Benchmark() : frequency(), start(), current()
	{
		QueryPerformanceFrequency( &frequency );
		QueryPerformanceCounter( &start );
		QueryPerformanceCounter( &current );
	}
	virtual ~Benchmark()							= default;
	Benchmark( const Benchmark & )					= delete;
	Benchmark( Benchmark && )						= delete;
	Benchmark & operator = ( const Benchmark & )	= delete;
	Benchmark & operator = ( Benchmark && )			= delete;
public:
	/// <summary>
	/// Please call when start recording.
	/// </summary>
	inline void Begin() { QueryPerformanceCounter( &start ); }

	/// <summary>
	/// Prease call end recording,<para></para>
	/// I return elapse time.
	/// </summary>
	inline double End()
	{
		QueryPerformanceCounter( &current );
		return static_cast<double>( current.QuadPart - start.QuadPart ) / static_cast<double>( frequency.QuadPart );
	}

	/// <summary>
	/// Returns float type.
	/// </summary>
	inline float EndF()
	{
		QueryPerformanceCounter( &current );
		return static_cast<float>( current.QuadPart - start.QuadPart ) / static_cast<float>( frequency.QuadPart );
	}
};

#endif // INCLUDED_PROCESS_TIMER_H_