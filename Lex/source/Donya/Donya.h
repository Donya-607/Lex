#pragma once

// see https://maminus.hatenadiary.org/entry/20130226/1361895337
// I want to lightly this file.
#ifdef _M_IX86
#define _X86_
#else
#define _AMD64_
#endif
#include <windef.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Donya
{
	/// <summary>
	/// Initialize Donya's engine, if not initialized yet.<para></para>
	/// Use Windows::Foundation::Initialize( RO_INIT_MULTITHREADED ).<para></para>
	/// If initialize failed, returns false.<para></para>
	/// In release mode, the isAppendFPS flag fixed to false.
	/// </summary>
	bool Init( int nCmdShow, int screenWidth, int screenHeight, const char *windowCaption, bool fullScreenMode, bool isAppendFPS = true, bool isEnableMultiThreaded = true );

	/// <summary>
	/// Returns false if failed.
	/// </summary>
	bool SetWindowIcon( const HINSTANCE &hInstance, int iconID );

	/// <summary>
	/// Please call to first in game-loop.<para></para>
	/// If I received WM_QUIT, returns false.
	/// </summary>
	int MessageLoop();

	/// <summary>
	/// Please call after MessageLoop().<para></para>
	/// This function doing:<para></para>
	/// Keyboard::Update(),<para></para>
	/// ScreenShake::Update(),<para></para>
	/// Sound::Update().
	/// </summary>
	void SystemUpdate();

	/// <summary>
	/// Represent delta-time.
	/// </summary>
	float GetElapsedTime();

	float GetFPS();

	/// <summary>
	/// Returns caption was passed when Donya::Init().
	/// </summary>
	const char *GetWindowCaption();

	/// <summary>
	/// Doing ClearRenderTargetView(), ClearDepthStencilView().<para></para>
	/// these argument is fill color, range is 0.0f ~ 1.0f.
	/// </summary>
	void ClearViews( FLOAT R = 0.0f, FLOAT G = 0.0f, FLOAT B = 0.0f, FLOAT A = 1.0f );
	/// <summary>
	/// Doing ClearRenderTargetView(), ClearDepthStencilView().<para></para>
	/// fill color range is 0.0f ~ 1.0f.
	/// </summary>
	void ClearViews( const FLOAT ( &fillColor )[4] );

	/// <summary>
	/// Doing IDXGISwapChain::Present(), check and assertion return value.<para></para>
	/// returns false when failed.
	/// </summary>
	bool Present( UINT syncInterval = 0, UINT flags = 0 );

	/// <summary>
	/// Returns value is the exit code.<para></para>
	/// You should use the value in return value of WinMain().
	/// </summary>
	int Uninit();

	HWND				&GetHWnd();
	ID3D11Device		*GetDevice();
	ID3D11DeviceContext	*GetImmediateContext();

	/// <summary>
	/// This namespace there for Donya engine.
	/// </summary>
	namespace Private
	{
		int		RegisteredScreenWidth();
		float	RegisteredScreenWidthF();
		long	RegisteredScreenWidthL();

		int		RegisteredScreenHeight();
		float	RegisteredScreenHeightF();
		long	RegisteredScreenHeightL();

		int		RegisteredHalfScreenWidth();
		float	RegisteredHalfScreenWidthF();
		long	RegisteredHalfScreenWidthL();

		int		RegisteredHalfScreenHeight();
		float	RegisteredHalfScreenHeightF();
		long	RegisteredHalfScreenHeightL();
	}
}
