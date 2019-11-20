#include "Looper.h"

#include <algorithm> // Use for std::min, std::max.

#undef min
#undef max

namespace Donya
{
	Looper::Looper() :
		timer( 0 ), current( 0 ), waitFrame( -1 ),
		first( 0 ), last( 0 ),
		isFinished( false ), doLoop( true ), doInverse( false )
	{

	}

	void Looper::Update()
	{
		if ( waitFrame < 0 )			{ return; }
		if ( !doLoop && isFinished )	{ return; }
		// else

		if ( doInverse )
		{
			InverseUpdate();
		}
		else
		{
			NormalUpdate();
		}
	}

	void Looper::SetRange( int rangeFirst, int rangeLast )
	{
		if ( rangeLast < rangeFirst )
		{
			std::swap( rangeFirst, rangeLast );
		}

		first = rangeFirst;
		last  = rangeLast;

		current = std::max( first, current );
		current = std::min( last,  current );
	}
	void Looper::SetWaitFrame( int wait )
	{
		waitFrame = wait;
	}
	void Looper::SetRegulation( bool enableLoop, bool enableInverseLoop )
	{
		doLoop		= enableLoop;
		doInverse	= enableInverseLoop;
	}
	void Looper::SetCurrentIndex( int currentIndex, int currentTimer )
	{
		if ( 0 <= currentIndex ) { current	= currentIndex; }
		if ( 0 <= currentTimer ) { timer	= currentTimer; }
	}

	void Looper::NormalUpdate()
	{
		timer++;
		isFinished = false;

		if ( timer < waitFrame ) { return; }
		// else

		timer = 0;

		NormalAdvance();
	}
	void Looper::InverseUpdate()
	{
		// Note:Currently, this function is almost same as NormalUpdate().

		timer++;
		isFinished = false;

		if ( timer < waitFrame ) { return; }
		// else

		timer = 0;

		InverseAdvance();
	}

	void Looper::NormalAdvance()
	{
		if ( doLoop )
		{
			current = ( last <= current ) ? first : current + 1;
			if ( current == first )
			{
				isFinished = true;
			}
		}
		else
		{
			current++;

			if ( last < current )
			{
				current		= last;
				isFinished	= true;
			}
		}
	}
	void Looper::InverseAdvance()
	{
		if ( doLoop )
		{
			current = ( current <= first ) ? last : current - 1;
			if ( current == last )
			{
				isFinished = true;
			}
		}
		else
		{
			current--;

			if ( current < first )
			{
				current		= first;
				isFinished	= true;
			}
		}
	}
}
