#include "Mouse.h"

#include "Keyboard.h" // use for GetMouseButtonState.

namespace Donya
{
	namespace Mouse
	{
		struct Int2 { int x = 0, y = 0; };
		static Int2 coordinate; // client-space.

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

		void GetMouseCoord( int *x, int *y )
		{
			if ( x != nullptr ) { *x = coordinate.x; }
			if ( y != nullptr ) { *y = coordinate.y; }
		}
		void GetMouseCoord( float *x, float *y )
		{
			int ix = 0, iy = 0;
			GetMouseCoord( &ix, &iy );

			if( x != nullptr ) { *x = static_cast<float>( ix ); }
			if( y != nullptr ) { *y = static_cast<float>( iy ); }
		}

		int  GetMouseWheelRot( bool isVertical )
		{
			return ( isVertical ) ? rotateAmount.y : rotateAmount.x;
		}

		bool GetMouseButtonState( MouseButtonState checkState, MouseButtonType checkButton )
		{
			int vKey = NULL;
			switch ( checkButton )
			{
			case Left:		vKey = VK_LBUTTON;	break;
			case Middle:		vKey = VK_MBUTTON;	break;
			case Right:		vKey = VK_RBUTTON;	break;
			}

			Donya::Keyboard::Mode mode = Donya::Keyboard::Mode::PRESS;
			switch ( checkState )
			{
			case PRESS:		mode = Donya::Keyboard::Mode::PRESS;	break;
			case TRIGGER:	mode = Donya::Keyboard::Mode::TRIGGER;	break;
			case RELEASE:	mode = Donya::Keyboard::Mode::RELEASE;	break;
			}

			return Donya::Keyboard::State( vKey, mode );
		}

	}
}