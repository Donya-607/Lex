#ifndef INCLUDED_COUNTER_H_
#define INCLUDED_COUNTER_H_

namespace Donya
{
	/// <summary>
	/// Requirement :<para></para>
	/// Overload operators; assignment, addition, comparison.
	/// </summary>
	template<typename T>
	class Counter
	{
	private:
		T		addAmount;
		T		current;
		T		goal;
		bool	isDone;
	public:
		Counter() :
			addAmount( T{} ), current( T{} ), goal( T{} ), isDone( false ) {}
		Counter( T changeAmount, T goal ) :
			addAmount( changeAmount ), current( T{} ), goal( goal ), isDone( false ) {}
	public:
		void Update( float elapsedTime )
		{
			if ( isDone ) { return; }
			// else

			current += addAmount * elapsedTime;
			if ( goal <= current )
			{
				current = goal;
				isDone = true;
			}
		}

		void Set( T newGoal, T changeAmount, T initCurrent = T{} )
		{
			addAmount	= changeAmount;
			goal		= newGoal;
			current		= initCurrent;
			isDone		= false;
		}

		void Reset( T newCurrent )
		{
			current = newCurrent;
			isDone  = false;
		}

		T		GetChangeAmount()	const { return addAmount;	}
		T		GetCurrent()		const { return current;		}
		T		GetGoal()			const { return goal;		}
		bool	IsDone()			const { return isDone;		}
	};
}

#endif // INCLUDED_COUNTER_H_