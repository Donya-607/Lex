#include <assert.h>
#include <locale.h>
#include <memory>
#include <tchar.h>
#include <time.h>
#include <windows.h>

#include "Common.h"
#include "Framework.h"
#include "Useful.h"
#include "WindowsUtil.h"

void RegisterWindowClass( HINSTANCE instance );
LRESULT CALLBACK fnWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

INT WINAPI wWinMain( HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd_line, INT cmd_show )
{
#if defined( DEBUG ) | defined( _DEBUG )
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	// When memory leak detected, if you assign the output number to _crtBreakAlloc,
	// program will be stop in that memory allocate place.
	// _crtBreakAlloc = ;
#endif

	setlocale( LC_ALL, "JPN" );

	srand( scast<unsigned int>( time( NULL ) ) );

	RegisterWindowClass( instance );

	HWND hwnd{};

	// Create Window
	{
		RECT rect =
		{
			0, 0,
			Common::ScreenWidthL(),
			Common::ScreenHeightL()
		};
		AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE );

		int width  = rect.right  - rect.left;
		int height = rect.bottom - rect.top;

		RECT rectDesk = GetDesktopRect();

		std::wstring title = Donya::MultiToWide( std::string{ Framework::TITLE_BAR_CAPTION } );
		hwnd = CreateWindow
		(
			title.data(),
			L"",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			( rectDesk.right  >> 1 ) - ( width  >> 1 ),
			( rectDesk.bottom >> 1 ) - ( height >> 1 ) - GetCaptionBarHeight(),
			width,
			height,
			NULL,
			NULL,
			instance,
			NULL
		);
		ShowWindow( hwnd, cmd_show );
	}

	Framework f{ hwnd };
	SetWindowLongPtr
	(
		hwnd,
		GWLP_USERDATA,
		reinterpret_cast<LONG_PTR>( &f )
	);
	return f.Run();
}

void RegisterWindowClass( HINSTANCE instance )
{
	std::wstring title = Donya::MultiToWide( std::string{ Framework::TITLE_BAR_CAPTION } );

	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof( WNDCLASSEX );
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= fnWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= instance;
	wcex.hIcon			= 0;
	wcex.hCursor		= LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground	= (HBRUSH)( COLOR_WINDOW + 1 );
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= title.data();
	wcex.hIconSm		= 0;
	RegisterClassEx( &wcex );
}

LRESULT CALLBACK fnWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
	Framework *f = reinterpret_cast<Framework*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	return f ? f->HandleMessage( hwnd, msg, wparam, lparam ) : DefWindowProc( hwnd, msg, wparam, lparam );
}