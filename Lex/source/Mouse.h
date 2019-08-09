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
		void GetMouseCoord( int *x, int *y );

		/// <summary>
		/// Same as call int version by using static_cast.
		/// </summary>
		void GetMouseCoord( float *x, float *y );

		/// <summary>
		/// Returns :<para></para>
		/// positive : rotate to up.<para></para>
		/// zero : not rotate.<para></para>
		/// negative : rotate to down.
		/// </summary>
		int GetMouseWheelRot( bool isVertical = true );

		enum MouseButtonState
		{
			PRESS = 0,
			TRIGGER,
			RELEASE
		};
		enum MouseButtonType
		{
			Left = 0,
			Middle,
			Right
		};

		/// <summary>
		/// Same as call Donya::Keyboard::State() by VK_LBUTTON, VL_MBUTTON or VK_RBUTTON.
		/// </summary>
		bool GetMouseButtonState( MouseButtonState checkState, MouseButtonType checkButton );

	}
}

#endif // _INCLUDED_MOUSE_H_