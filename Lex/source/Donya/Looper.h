#pragma once

namespace Donya
{
	/// <summary>
	/// This class provide looping count, that like animation.
	/// </summary>
	class Looper
	{
	private:
		int		timer;		// 0-based.
		int		current;	// 0-based.
		int		waitFrame;	// 0-based.
		int		first;		// 0-based. Playing range, contain this count.
		int		last;		// 0-based. Playing range, contain this count.
		bool	isFinished;	// True when complete waited when the current index is last.
		bool	doLoop;
		bool	doInverse;
	public:
		Looper();
	public:
		/// <summary>
		/// Please call every frame.
		/// </summary>
		void Update();
	public:
		/// <summary>
		/// Each arguments is 0-based.
		/// </summary>
		void SetRange( int rangeFirst, int rangeLast );
		/// <summary>
		/// The arguments is 0-based.<para></para>
		/// This frame apply every timing.<para></para>
		/// If set -1 to "waitFrame", I will stop.
		/// </summary>
		void SetWaitFrame( int waitFrame );
		/// <summary>
		/// Each arguments is 0-based.<para></para>
		/// If set false to "enableLoop", I will stop at last index.<para></para>
		/// If the "enableInverseLoop" is true, I loop from last to first, if it is false, I loop from first to last.
		/// </summary>
		void SetRegulation( bool enableLoop, bool enableInverseLoop );
		/// <summary>
		/// Each arguments is 0-based.<para></para>
		/// If set -1 to any argument, that argument will be ignore.
		/// </summary>
		void SetCurrentIndex( int currentIndex, int currentWaitingFrame = -1 );
	public:
		/// <summary>
		/// Returns current number of index.
		/// </summary>
		int Current()			const { return current; }

		int GetWaitFrame()		const { return waitFrame; }
		int GetRangeFirst()		const { return first; }
		int GetRangeLast()		const { return last; }

		/// <summary>
		/// Returns true if when complete waited when the current index is last.
		/// </summary>
		bool IsFinished()		const { return isFinished; }
		bool IsEnabledLoop()	const { return doLoop; }
		bool IsEnabledInverse()	const { return doInverse; }
	private:
		void NormalUpdate();
		void InverseUpdate();
		void NormalAdvance();
		void InverseAdvance();
	};
}
