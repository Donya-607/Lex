#ifndef _INCLUDED_MOUSE_H_
#define _INCLUDED_MOUSE_H_

#include <Windows.h>

namespace Donya
{
	namespace Mouse
	{
		/// <summary>
		/// Please call when received WM_MOUSEMOVE at window procedure.
		/// </summary>
		void UpdateMouseCoordinate( LPARAM lParam );

		/// <summary>
		/// Please call when received WM_MOUSEWHEEL or WM_MOUSEHWHEEL at window procedure.
		/// </summary>
		void CalledMouseWheelMessage( bool isVertical, WPARAM wParam, LPARAM lParam );

		/// <summary>
		/// Please call once when every game-loop.
		/// </summary>
		void ResetMouseWheelRot();

		/// <summary>
		/// Mouse coordinate is cliant space.<para></para>
		/// You can set nullptr.
		/// </summary>
		POINT Coordinate();
		/// <summary>
		/// Mouse coordinate is cliant space.<para></para>
		/// You can set nullptr.
		/// </summary>
		void Coordinate( int *x, int *y );
		/// <summary>
		/// Mouse coordinate is cliant space.<para></para>
		/// You can set nullptr.
		/// </summary>
		void Coordinate( float *x, float *y );

		POINT Size();
		/// <summary>
		/// You can set nullptr.
		/// </summary>
		void Size( int *x, int *y );

		/// <summary>
		/// Returns :<para></para>
		/// positive : rotate to up.<para></para>
		/// zero : not rotate.<para></para>
		/// negative : rotate to down.
		/// </summary>
		int WheelRot( bool isVertical = true );

		enum Mode
		{
			PRESS = 0,
			RELEASE,
			REPEAT,
			TRIGGER
		};
		enum Kind
		{
			LEFT	= VK_LBUTTON,
			MIDDLE	= VK_MBUTTON,
			RIGHT	= VK_RBUTTON
		};

		// bool State( Mode inputMode, Kind checkButton );

		/// <summary>
		/// Same as call Donya::Keyboard::Press( checkButton ).
		/// </summary>
		int Press( Kind checkButton );
		/// <summary>
		/// Same as call Donya::Keyboard::Release( checkButton ).
		/// </summary>
		int Release( Kind checkButton );
		/// <summary>
		/// Same as call Donya::Keyboard::Repeat( checkButton ).
		/// If current pressing frame is equal to "lowestNeedFrame", return TRUE(1).
		/// </summary>
		int Repeat( Kind checkButton, int repeatIntervalFrame, int lowestNeedFrame = 0 );
		/// <summary>
		/// Same as call Donya::Keyboard::Trigger( checkButton ).
		/// </summary>
		int Trigger( Kind checkButton );
	}
}

#endif // _INCLUDED_MOUSE_H_