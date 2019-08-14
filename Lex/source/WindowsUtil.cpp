#include "WindowsUtil.h"

RECT GetDesktopRect()
{
	HWND hDesktop = GetDesktopWindow();
	RECT rectDesktop{};

	GetWindowRect( hDesktop, &rectDesktop );

	return rectDesktop;
}

int GetCaptionBarHeight()
{
#if 1

	// see https://stackoverflow.com/questions/28524463/how-to-get-the-default-caption-bar-height-of-a-window-in-windows
	int height =
		GetSystemMetrics( SM_CYFRAME		) +
		GetSystemMetrics( SM_CYCAPTION		) +
		GetSystemMetrics( SM_CXPADDEDBORDER	)
		;
	return height;

#else

	// see http://sumishiro.blogspot.com/2010/03/blog-post_13.html
	int height = 
		GetSystemMetrics( SM_CYEDGE		) +
		GetSystemMetrics( SM_CYBORDER	) +
		GetSystemMetrics( SM_CYDLGFRAME	) +
		GetSystemMetrics( SM_CYCAPTION	)
		;
	return height;

#endif // 1
}

POINT GetClientCoordinate( HWND hWnd )
{
	// see https://stackoverflow.com/questions/15734528/client-rectangle-coordinates-on-screen

	RECT client{};
	GetClientRect( hWnd, &client );

	// see https://docs.microsoft.com/ja-jp/windows/win32/api/winuser/nf-winuser-mapwindowpoints
	MapWindowPoints( hWnd, nullptr, reinterpret_cast<LPPOINT>( &client ), 2U );

	POINT  coord{ client.left, client.top };
	return coord;
}
