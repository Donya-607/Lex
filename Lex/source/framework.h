#pragma once

#include <windows.h>
#include <tchar.h>
#include <sstream>

#include <array>
#include <d3d11.h>
#include <memory>
#include <vector>
#include <wrl.h>

#include "Loader.h"
#include "HighResolutionTimer.h"

#define scast static_cast

namespace Donya
{
	class Camera;
}

class Framework
{
public:
	static constexpr char *TITLE_BAR_CAPTION = "Lex - I can open a files by D&D.";
private:
	CONST HWND hWnd;

	Microsoft::WRL::ComPtr<IDXGISwapChain>			dxgiSwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	d3dRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	d3dDepthStencilView;
private:
	std::unique_ptr<Donya::Camera>	pCamera;
	std::vector<Donya::Loader>		loaders;
private:
	bool isFillDraw;
public:
	Framework( HWND hwnd );
	~Framework();
	Framework( const Framework &  ) = delete;
	Framework( const Framework && ) = delete;
	Framework & operator = ( const Framework &  ) = delete;
	Framework & operator = ( const Framework && ) = delete;
public:
	int Run();
	LRESULT CALLBACK HandleMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
private:
	bool Init();
	void Update( float elapsed_time/*Elapsed seconds from last frame*/ );
	void Render( float elapsed_time/*Elapsed seconds from last frame*/ );
private:
	HighResolutionTimer highResoTimer;
	void CalcFrameStats();
private:
	bool OpenCommonDialogAndFile();
};

