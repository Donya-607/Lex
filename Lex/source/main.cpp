#include <assert.h>
#include <locale.h>
#include <memory>
#include <tchar.h>
#include <time.h>
#include <windows.h>

#include "Donya/Donya.h"
#include "Donya/Useful.h"
#include "Donya/WindowsUtil.h"

#include "Common.h"
#include "Framework.h"

void RegisterWindowClass( HINSTANCE instance );
LRESULT CALLBACK fnWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

INT WINAPI wWinMain( _In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ LPWSTR cmdLine, _In_ INT cmdShow )
{
#if defined( DEBUG ) | defined( _DEBUG )
	// reference:https://docs.microsoft.com/ja-jp/visualstudio/debugger/crt-debug-heap-details?view=vs-2015
	_CrtSetDbgFlag
	(
		_CRTDBG_ALLOC_MEM_DF
		| _CRTDBG_LEAK_CHECK_DF
		// | _CRTDBG_CHECK_ALWAYS_DF
	);
	// When memory leak detected, if you assign the output number to "_crtBreakAlloc",
	// program will be stop in that memory allocate place. ex : _crtBreakAlloc = 123;
	// _crtBreakAlloc = ;
#endif

	setlocale( LC_ALL, "JPN" );

	srand( scast<unsigned int>( time( NULL ) ) );

	std::string title{ "‚¨‚ä‚¤‚¬" };
	Donya::Init( cmdShow, Common::ScreenWidth(), Common::ScreenHeight(), title.c_str(), /* fullScreenMode = */ false );

	// Donya::SetWindowIcon( instance, IDI_ICON );

	Framework framework{};
	framework.Init();

	while ( Donya::MessageLoop() )
	{
		Donya::ClearViews();

		Donya::SystemUpdate();
		framework.Update( Donya::GetElapsedTime() );

		framework.Draw( Donya::GetElapsedTime() );
		Donya::Present( 1 );
	}

	framework.Uninit();

	auto   returnValue = Donya::Uninit();
	return returnValue;
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