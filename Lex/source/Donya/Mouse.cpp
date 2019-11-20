#include "Mouse.h"

#include "Keyboard.h" // use for GetMouseButtonState.

namespace Donya
{
	namespace Mouse
	{
		struct Int2 { int x = 0, y = 0; };
		static Int2 coordinate; // cliant space.

		static Int2 wheelFraction{};
		static Int2 rotateAmount{};

		void UpdateMouseCoordinate( LPARAM lParam )
		{
			coordinate.x = LOWORD( lParam );
			coordinate.y = HIWORD( lParam );
		}

		void CalledMouseWheelMessage( bool isVertical, WPARAM wParam, LPARAM lParam )
		{
			// see http://black-yuzunyan.lolipop.jp/archives/2544

			// If update here, the coordinate will be screen-space.
			// but the coordinate is client-space, so I don't update.
			// UpdateMouseCoordinate( lParam );

			int *fraction = ( isVertical ) ? &wheelFraction.y : &wheelFraction.x;
			int *rotation = ( isVertical ) ? &rotateAmount.y  : &rotateAmount.x;

			int delta = GET_WHEEL_DELTA_WPARAM( wParam );
			delta += *fraction;

			*fraction  = delta % WHEEL_DELTA;

			int  notch = delta / WHEEL_DELTA;
			if ( notch < 0 )
			{
				( *rotation )--;
			}
			else if ( 0 < notch )
			{
				( *rotation )++;
			}
			else
			{
				*rotation = 0;
			}
		}

		void ResetMouseWheelRot()
		{
			rotateAmount.x =
			rotateAmount.y = 0;
		}

		POINT Coordinate()
		{
			return POINT{ coordinate.x, coordinate.y };
		}
		void Coordinate( int *x, int *y )
		{
			if ( x != nullptr ) { *x = coordinate.x; }
			if ( y != nullptr ) { *y = coordinate.y; }
		}
		void Coordinate( float *x, float *y )
		{
			int ix = 0, iy = 0;
			Coordinate( &ix, &iy );

			if( x != nullptr ) { *x = static_cast<float>( ix ); }
			if( y != nullptr ) { *y = static_cast<float>( iy ); }
		}

		POINT Size()
		{
			POINT size
			{
				GetSystemMetrics( SM_CXCURSOR ),
				GetSystemMetrics( SM_CYCURSOR )
			};
			return size;
		}
		void Size( int *x, int *y )
		{
			POINT size = Size();

			if ( x != nullptr ) { *x = size.x; }
			if ( y != nullptr ) { *y = size.y; }
		}

		int  WheelRot( bool isVertical )
		{
			return ( isVertical ) ? rotateAmount.y : rotateAmount.x;
		}

		/*
		int State( Kind checkButton, Mode inputMode )
		{
			using Donya::Keyboard::Mode;

			switch ( inputMode )
			{
			case PRESS:		return Donya::Keyboard::Press( checkButton );	// break;
			case RELEASE:	return Donya::Keyboard::Release( checkButton );	// break;
			case REPEAT:	return Donya::Keyboard::Repeat( checkButton );	// break;
			case TRIGGER:	return Donya::Keyboard::Trigger( checkButton );	// break;
			default:		break;
			}

			return Donya::Keyboard::Press( checkButton );
		}
		*/

		int Press( Kind checkButton )
		{
			return Donya::Keyboard::Press( checkButton );
		}

		int Release( Kind checkButton )
		{
			return Donya::Keyboard::Release( checkButton );
		}

		int Repeat( Kind checkButton, int interval, int lowestFrame )
		{
			return Donya::Keyboard::Repeat( checkButton, interval, lowestFrame );
		}

		int Trigger( Kind checkButton )
		{
			return Donya::Keyboard::Trigger( checkButton );
		}

	}
}