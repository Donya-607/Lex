#include "Keyboard.h"

#include <Windows.h>

namespace Donya
{

#if USE_STATIC_ARRAY_FOR_KEYBOARD_UPDATE

	namespace Keyboard
	{
		constexpr size_t KEYBOARD_SIZE = UCHAR_MAX + 1;

		static unsigned int previous[KEYBOARD_SIZE] = { 0 };
		static unsigned int current[KEYBOARD_SIZE]  = { 0 };

		void Update()
		{
			if ( memcpy_s( previous, sizeof( previous ), current, sizeof( current ) ) != 0 )
			{
				// NOP, I should be error process.
			}

			BYTE temporary[KEYBOARD_SIZE] = { 0 };
			if ( GetKeyboardState( temporary ) == FALSE )
			{
				// NOP, I should be error process.
			}

			for ( size_t i = 0; i < KEYBOARD_SIZE; ++i )
			{
				if ( temporary[i] & 0x80 )
				{
					if ( UINT_MAX <= current[i] ) { continue; }
					// else
					current[i]++;
				}
				else
				{
					current[i] = 0;
				}
			}
		}

		int State( int vKey, Mode inputMode )
		{
			switch ( inputMode )
			{
			case Mode::PRESS:	return current[vKey];
			case Mode::TRIGGER:	return ( 1 == current[vKey] ) ? TRUE : FALSE;
			case Mode::RELEASE:	return ( 0 == current[vKey] && 1 <= previous[vKey] ) ? TRUE : FALSE;
			default:			break;
			}

			// returns press state.
			return current[vKey];
		}

		int Press( int vKey )   { return State( vKey, Mode::PRESS );   }
		int Release( int vKey ) { return State( vKey, Mode::RELEASE ); }
		int Repeat( int vKey, int interval, int lowestFrame )
		{
			int current = State( vKey, Mode::PRESS );
			current -= lowestFrame;

			if ( current <= 0 ) { return FALSE; }
			// else
			
			// prevent zero-divide.
			if ( interval == 0 ) { return ( current ) ? TRUE : FALSE; }
			// else

			return ( ( current % interval ) == 1 ) ? TRUE : FALSE;
		}
		int Trigger( int vKey ) { return State( vKey, Mode::TRIGGER ); }

		int Shifts( Mode inputMode )
		{
			if ( State( VK_LSHIFT, inputMode ) ) { return true; }
			if ( State( VK_RSHIFT, inputMode ) ) { return true; }
			// else
			return false;
		}
	}

#else

	bool Keyboard::State( Keyboard::Mode inputMode )
	{
		previousState = currentState;

		currentState = ( GetAsyncKeyState( vKeycode ) & 0x8000 )
			? currentState + 1
			: 0;
		switch ( inputMode )
		{
		case Mode::TRIGGER:
			return ( previousState <= 0 && currentState == 1 ) ? true : false;
		case Mode::RELEASE:
			return ( 0 < previousState  && currentState <= 0 ) ? true : false;
		case Mode::PRESS:
			return ( 0 < currentState ) ? true : false;
		default:
			static char passErrorCount = 0; passErrorCount += 1;
			return false;
		}
	}

#endif

}