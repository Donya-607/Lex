#pragma once

#include <windows.h>
#include <tchar.h>
#include <sstream>

#include <array>
#include <d3d11.h>
#include <memory>
#include <vector>
#include <wrl.h>

#include <mutex>
#include <list>
#include <thread>

#include "Camera.h"
#include "Loader.h"
#include "HighResolutionTimer.h"
#include "SkinnedMesh.h"
#include "Vector.h"

#define scast static_cast

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
	Camera camera;
	struct Light
	{
		Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Donya::Vector4 direction{ 0.0f, 6.0f, 0.0f, 0.0f };
	};
	Light light;
	struct MeshAndInfo
	{
		Donya::Loader		loader;
		Donya::SkinnedMesh	mesh;
	};
	std::vector<MeshAndInfo> meshes;
private:
	int pressMouseButton; // contain value is: None:0, Left:VK_LBUTTON, Middle:VK_MBUTTON, Right:VK_RBUTTON.
	bool isCaptureWindow;
	bool isSolidState;
private:
	struct AsyncLoad
	{
		std::string						filePath{};	// absolute-path.
		std::unique_ptr<std::mutex>		mtx{};
		std::unique_ptr<std::thread>	pThread{};
		MeshAndInfo						meshInfo{};
		bool isFinished  = false;	// Is finished the loading process ?
		bool isSucceeded = false;	// Is Succeeded
	};
	// Note:If it is vector, I can't pass the pointer of element.
	std::list<AsyncLoad> loadingData;
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
	void StartLoadThread( std::string filePath );
	void AppendModelIfLoadFinished();
	void ShowNowLoadingModels();
private:
	bool OpenCommonDialogAndFile();
	void SetMouseCapture();
	void ReleaseMouseCapture();
	void PutLimitMouseMoveArea();
private:
	void ShowMouseInfo();
	void ShowModelInfo();
	void ChangeLightByImGui();
};

