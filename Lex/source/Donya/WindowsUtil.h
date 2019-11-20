#pragma once

#include <Windows.h>

namespace Donya
{
	int GetCaptionBarHeight();

	/// <summary>
	/// The desktop is also window.
	/// </summary>
	RECT GetDesktopRect();

	/// <summary>
	/// Returns client coordinate of specified window in screen-space.
	/// </summary>
	POINT GetClientCoordinate( HWND hWnd );
}
