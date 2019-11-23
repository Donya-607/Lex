#include "Framework.h"

#include <array>
#include <algorithm>
#include <thread>

#include "Donya/Benchmark.h"
#include "Donya/Donya.h"
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Resource.h"
#include "Donya/UseImGui.h"
#include "Donya/Useful.h"
#include "Donya/WindowsUtil.h"
 
#include "Camera.h"
#include "Common.h"
#include "Loader.h"

#include "Framework.h"

#include <array>

#include "Donya/Blend.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Resource.h"
#include "Donya/ScreenShake.h"
#include "Donya/Sound.h"
#include "Donya/Useful.h"
#include "Donya/UseImgui.h"

#include "Common.h"
#include "Music.h"

using namespace DirectX;

Framework::Framework() :
	pSceneMng( nullptr )
{}
Framework::~Framework() = default;

bool Framework::Init()
{
	bool  loadResult = LoadSounds();
	if ( !loadResult )
	{
		_ASSERT_EXPR( 0, L"Failed : One or more sounds were not loaded." );
		return false;
	}

	pSceneMng = std::make_unique<SceneMng>();
	pSceneMng->Init( Scene::Type::Lex );

	return true;
}

void Framework::Uninit()
{
	pSceneMng->Uninit();
}

void Framework::Update( float elapsedTime/*Elapsed seconds from last frame*/ )
{
#if DEBUG_MODE

	DebugShowInformation();

#endif // DEBUG_MODE

	pSceneMng->Update( elapsedTime );
}

void Framework::Draw( float elapsedTime/*Elapsed seconds from last frame*/ )
{
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA );

	pSceneMng->Draw( elapsedTime );
}

bool Framework::LoadSounds()
{
	using Donya::Sound::Load;
	using Music::ID;

	struct Bundle
	{
		ID id;
		std::string fileName;
		bool isEnableLoop;
	public:
		Bundle() : id(), fileName(), isEnableLoop( false ) {}
		Bundle( ID id, const char *fileName, bool isEnableLoop ) : id( id ), fileName( fileName ), isEnableLoop( isEnableLoop ) {}
		Bundle( ID id, const std::string &fileName, bool isEnableLoop ) : id( id ), fileName( fileName ), isEnableLoop( isEnableLoop ) {}
	};

	const std::array<Bundle, ID::MUSIC_COUNT> bandles =
	{
		{	// ID, FilePath, isEnableLoop
			// { ID::BGM,					"./Data/Sounds/BGM/Title.wav",				true  },
			
			{ ID::ItemChoose,			"./Data/Sounds/SE/UI/Choose.wav",			false },
			{ ID::ItemDecision,			"./Data/Sounds/SE/UI/Decision.wav",			false },
		},
	};

	bool result = true, successed = true;

	for ( size_t i = 0; i < ID::MUSIC_COUNT; ++i )
	{
		result = Load( bandles[i].id, bandles[i].fileName.c_str(), bandles[i].isEnableLoop );
		if ( !result ) { successed = false; }
	}

	return successed;
}

#if USE_IMGUI
#include "Donya/Easing.h"
#endif // USE_IMGUI
void Framework::DebugShowInformation()
{
#if USE_IMGUI

#if SHOW_EASING_BEHAVIORS

	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( "Easing Test" ) )
		{
			using namespace Donya::Easing;

			static float time = 0.0f;
			ImGui::SliderFloat( u8"時間", &time, 0.0f, 1.0f );
			ImGui::Text( "" );
			static Type type = Type::In;
			{
				int iType = scast<int>( type );

				std::string caption = "Type : ";
				if ( type == Type::In    ) { caption += "In";    }
				if ( type == Type::Out   ) { caption += "Out";   }
				if ( type == Type::InOut ) { caption += "InOut"; }

				ImGui::SliderInt( caption.c_str(), &iType, 0, 2 );

				type = scast<Type>( iType );
			}

			constexpr int EASING_KIND_COUNT = GetKindCount();
			for ( unsigned int i = 0; i < EASING_KIND_COUNT; ++i )
			{
				float result = Ease( scast<Kind>( i ), type, time );
				ImGui::SliderFloat( KindName( i ), &result, -2.0f, 2.0f );
			}

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // SHOW_EASING_BEHAVIORS

#endif // USE_IMGUI
}

#pragma region OLD_Framework

//#if USE_IMGUI
//
//extern LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam );
//
//#endif // USE_IMGUI
//
//using namespace DirectX;
//
//static constexpr char *ImGuiWindowName = "File Information";
//
//OldFramework::OldFramework( HWND hwnd ) :
//	hWnd( hwnd ),
//	camera(),
//	light(),
//	meshes(),
//	pressMouseButton( NULL ),
//	isCaptureWindow( false ),
//	isSolidState( true ),
//	// mutex(),
//	pLoadThread( nullptr ),
//	pCurrentLoading( nullptr ),
//	currentLoadingFileNameUTF8(),
//	reservedAbsFilePaths(),
//	reservedFileNamesUTF8()
//{
//	DragAcceptFiles( hWnd, TRUE );
//}
//OldFramework::~OldFramework()
//{
//	if ( isCaptureWindow )
//	{
//		ReleaseMouseCapture();
//	}
//
//	meshes.clear();
//	meshes.shrink_to_fit();
//
//	if ( pLoadThread && pLoadThread->joinable() )
//	{
//		pLoadThread->join();
//	}
//};
//
//LRESULT CALLBACK OldFramework::HandleMessage( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
//{
//#ifdef USE_IMGUI
//	if ( ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) )
//	{
//		return 1;
//	}
//
//#endif
//
//	switch ( msg )
//	{
//	case WM_CREATE:
//		break;
//	case WM_DESTROY:
//		PostQuitMessage( 0 );
//		break;
//	case WM_DROPFILES:
//		{
//			constexpr size_t FILE_PATH_LENGTH = 512U;
//
//			HDROP	hDrop		= ( HDROP )wParam;
//			size_t	fileCount	= DragQueryFile( hDrop, -1, NULL, NULL );
//
//			// std::string errorMessage{};
//			std::unique_ptr<char[]> filename = std::make_unique<char[]>( FILE_PATH_LENGTH );
//			
//			for ( size_t i = 0; i < fileCount; ++i )
//			{
//				DragQueryFileA( hDrop, i, filename.get(), FILE_PATH_LENGTH );
//
//				ReserveLoadFile( std::string{ filename.get() } );
//
//				/*
//				meshes.push_back( {} );
//				bool result = meshes.back().loader.Load( filename.get(), &errorMessage );
//				if ( !result )
//				{
//					MessageBoxA( hWnd, errorMessage.c_str(), "File Load Failed", MB_OK );
//					continue;
//				}
//				// else
//				Donya::SkinnedMesh::Create( &meshes.back().loader, &meshes.back().mesh );
//				*/
//			}
//
//			DragFinish( hDrop );
//		}
//		break;
//	case WM_ENTERSIZEMOVE:
//		{
//			// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
//			highResoTimer.Stop();
//		}
//		break;
//	case WM_EXITSIZEMOVE:
//		{
//			// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
//			// Here we reset everything based on the new window dimensions.
//			highResoTimer.Start();
//		}
//		break;
//	case WM_KEYDOWN:
//		{
//			if ( wParam == VK_ESCAPE )
//			{
//				PostMessage( hWnd, WM_CLOSE, 0, 0 );
//			}
//		}
//		break;
//	#pragma region Mouse Process
//	case WM_MOUSEMOVE:
//		Donya::Mouse::UpdateMouseCoordinate( lParam );
//		if ( isCaptureWindow )
//		{
//			PutLimitMouseMoveArea();
//		}
//		break;
//	case WM_MOUSEWHEEL:
//		Donya::Mouse::CalledMouseWheelMessage( /* isVertical = */ true, wParam, lParam );
//		break;
//	case WM_MOUSEHWHEEL:
//		Donya::Mouse::CalledMouseWheelMessage( /* isVertical = */ false, wParam, lParam );
//		break;
//	case WM_LBUTTONDOWN:
//		if ( !pressMouseButton )
//		{
//			pressMouseButton = VK_LBUTTON;
//			SetMouseCapture();
//		}
//		break;
//	case WM_MBUTTONDOWN:
//		if ( !pressMouseButton )
//		{
//			pressMouseButton = VK_MBUTTON;
//			SetMouseCapture();
//		}
//		break;
//	case WM_RBUTTONDOWN:
//		if ( !pressMouseButton )
//		{
//			pressMouseButton = VK_RBUTTON;
//			SetMouseCapture();
//		}
//		break;
//	case WM_LBUTTONUP:
//		if ( isCaptureWindow )
//		{
//			if ( pressMouseButton == VK_LBUTTON || !pressMouseButton )
//			{
//				ReleaseMouseCapture();
//			}
//
//			pressMouseButton = NULL;
//		}
//		break;
//	case WM_MBUTTONUP:
//		if ( isCaptureWindow )
//		{
//			if ( pressMouseButton == VK_MBUTTON || !pressMouseButton )
//			{
//				ReleaseMouseCapture();
//			}
//
//			pressMouseButton = NULL;
//		}
//		break;
//	case WM_RBUTTONUP:
//		if ( isCaptureWindow )
//		{
//			if ( pressMouseButton == VK_RBUTTON || !pressMouseButton )
//			{
//				ReleaseMouseCapture();
//			}
//
//			pressMouseButton = NULL;
//		}
//		break;
//	#pragma endregion
//	case WM_PAINT:
//		{
//			PAINTSTRUCT ps;
//			HDC hdc;
//			hdc = BeginPaint( hWnd, &ps );
//			EndPaint( hWnd, &ps );
//			break;
//		}
//	default:
//		return DefWindowProc( hWnd, msg, wParam, lParam );
//	}
//	return 0;
//}
//
//void OldFramework::CalcFrameStats()
//{
//	// Code computes the average frames per second, and also the 
//	// average time it takes to render one frame.  These stats 
//	// are appended to the window caption bar.
//	static int frames = 0;
//	static float timeTlapsed = 0.0f;
//
//	frames++;
//
//	// Compute averages over one second period.
//	if ( ( highResoTimer.TimeStamp() - timeTlapsed ) >= 1.0f )
//	{
//		float fps	= scast<float>( frames ); // fps = frameCnt / 1
//		float mspf	= 1000.0f / fps;
//		std::ostringstream oss;
//		oss.precision( 6 );
//		oss	<< TITLE_BAR_CAPTION << " : "
//			<< "[FPS : " << fps << "] "
//			<< "[Frame Time : " << mspf << " (ms)]";
//		SetWindowTextA( hWnd, oss.str().c_str() );
//
//		// Reset for next average.
//		frames = 0;
//		timeTlapsed += 1.0f;
//	}
//}
//
//int OldFramework::Run()
//{
//	if ( !Init() ) { return 0; }
//	// else
//
//#ifdef USE_IMGUI
//
//	IMGUI_CHECKVERSION();
//	ImGui::CreateContext();
//
//	ImGui_ImplWin32_Init( hWnd );
//	ImGui_ImplDX11_Init( Donya::GetDevice(), Donya::GetImmediateContext() );
//	// ImGui::StyleColorsClassic();
//	// ImGui::StyleColorsLight();
//	ImGui::StyleColorsDark();
//
//	ImGuiIO &io = ImGui::GetIO();
//	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\meiryo.ttc", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
//	// io.Fonts->AddFontFromFileTTF( "c:\\Windows\\Fonts\\consolab.ttf", 10.0f, NULL, io.Fonts->GetGlyphRangesJapanese() );
//	// io.Fonts->AddFontFromFileTTF(".\\Inconsolata-Bold.ttf", 12.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
//
//#endif
//
//	MSG msg{};
//
//	while ( WM_QUIT != msg.message )
//	{
//		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
//		{
//			TranslateMessage( &msg );
//			DispatchMessage( &msg );
//		}
//		else
//		{
//			Donya::Keyboard::Update();
//
//			highResoTimer.Tick();
//			CalcFrameStats();
//			Update( highResoTimer.TimeInterval() );
//
//			Donya::Mouse::ResetMouseWheelRot();
//
//			Render( highResoTimer.TimeInterval() );
//		}
//	}
//
//	Donya::Resource::ReleaseAllCachedResources();
//
//#ifdef USE_IMGUI
//
//	ImGui_ImplDX11_Shutdown();
//	ImGui_ImplWin32_Shutdown();
//	ImGui::DestroyContext();
//
//#endif
//
//	BOOL isFullScreen = FALSE;
//	dxgiSwapChain->GetFullscreenState( &isFullScreen, 0 );
//	if ( isFullScreen )
//	{
//		dxgiSwapChain->SetFullscreenState( FALSE, 0 );
//	}
//
//	Donya::Uninit();
//
//	return static_cast<int>( msg.wParam );
//}
//
//bool OldFramework::Init()
//{
//	#pragma region DirectX
//
//	Donya::Init( nullptr );	// Doing initialize of ID3D11Device and ID3D11DeviceContext.
//
//	HRESULT hr = S_OK;
//
//	// Create Swapchain
//	{
//		DXGI_SWAP_CHAIN_DESC sd;
//		ZeroMemory( &sd, sizeof( sd ) );
//		sd.BufferCount							= 1;
//		sd.BufferDesc.Width						= Common::ScreenWidth();	// Size of back-buffer.
//		sd.BufferDesc.Height					= Common::ScreenHeight();	// Size of back-buffer.
//		sd.BufferDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM;
//		sd.BufferDesc.RefreshRate.Numerator		= 60;	// 
//		sd.BufferDesc.RefreshRate.Denominator	= 1;	// 分母
//		sd.BufferUsage							= DXGI_USAGE_RENDER_TARGET_OUTPUT;
//		sd.OutputWindow							= hWnd;
//		sd.SampleDesc.Count						= 1;
//		sd.SampleDesc.Quality					= 0;
//		sd.Windowed								= TRUE;
//		UINT sampleCount = 1;
//		UINT msaaQualityLevel;
//		hr = Donya::GetDevice()->CheckMultisampleQualityLevels( sd.BufferDesc.Format, sampleCount, &msaaQualityLevel );
//		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateMultisampleQualityLevels" );
//		sd.SampleDesc.Count		= sampleCount;
//		sd.SampleDesc.Quality	= msaaQualityLevel - 1;
//
//
//		Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
//		hr = CreateDXGIFactory( __uuidof( IDXGIFactory ), reinterpret_cast<void**>( dxgiFactory.GetAddressOf() ) );
//		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateDXGIFactory" );
//
//		{
//			// default adapter.
//			Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
//			hr = dxgiFactory->EnumAdapters( 0, dxgiAdapter.GetAddressOf() );
//			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : EnumAdapters" );
//		}
//
//		hr = dxgiFactory->CreateSwapChain
//		(
//			Donya::GetDevice(),
//			&sd,
//			dxgiSwapChain.GetAddressOf()
//		);
//		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateSwapChain" );
//	}
//
//
//	D3D11_TEXTURE2D_DESC d3dRenderTargetDesc = { 0 };
//	{
//		Microsoft::WRL::ComPtr<ID3D11Texture2D> d3dBackBuffer;
//		hr = dxgiSwapChain->GetBuffer
//		(
//			0, 
//			__uuidof(ID3D11Texture2D),
//			reinterpret_cast<void **>( d3dBackBuffer.GetAddressOf() )
//		);
//		/*
//		GetBuffer()により， d3dBackBuffer の参照カウントが１増える
//		なのでComPtrを使わない場合は，Release()を呼び出して参照カウントを減らさなければならない
//		*/
//		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : GetBuffer" );
//
//		hr = Donya::GetDevice()->CreateRenderTargetView
//		(
//			d3dBackBuffer.Get(),
//			nullptr,
//			d3dRenderTargetView.GetAddressOf()
//		);
//		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateRenderTargetView" );
//
//		d3dBackBuffer->GetDesc( &d3dRenderTargetDesc );
//	}
//	D3D11_TEXTURE2D_DESC d3dDepthStencilDesc = d3dRenderTargetDesc;
//	{
//		Microsoft::WRL::ComPtr<ID3D11Texture2D> d3dTexture2D;
//		d3dDepthStencilDesc.MipLevels		= 1;
//		d3dDepthStencilDesc.ArraySize		= 1;
//		d3dDepthStencilDesc.Format			= DXGI_FORMAT_R24G8_TYPELESS;
//		d3dDepthStencilDesc.Usage			= D3D11_USAGE_DEFAULT;
//		d3dDepthStencilDesc.BindFlags		= D3D11_BIND_DEPTH_STENCIL;
//		d3dDepthStencilDesc.CPUAccessFlags	= 0;
//		d3dDepthStencilDesc.MiscFlags		= 0;
//		hr = Donya::GetDevice()->CreateTexture2D
//		(
//			&d3dDepthStencilDesc,
//			NULL,
//			d3dTexture2D.GetAddressOf()
//		);
//		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateTexture2D" );
//
//		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
//		depthStencilViewDesc.Format				= DXGI_FORMAT_D24_UNORM_S8_UINT;
//		depthStencilViewDesc.ViewDimension		= D3D11_DSV_DIMENSION_TEXTURE2D;
//		depthStencilViewDesc.Flags				= 0;
//		depthStencilViewDesc.Texture2D.MipSlice	= 0;
//		hr = Donya::GetDevice()->CreateDepthStencilView
//		(
//			d3dTexture2D.Get(),
//			&depthStencilViewDesc,
//			d3dDepthStencilView.GetAddressOf()
//		);
//		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateDepthStencilView" );
//	}
//
//	Donya::GetImmediateContext()->OMSetRenderTargets
//	(
//		1,
//		d3dRenderTargetView.GetAddressOf(),
//		d3dDepthStencilView.Get()
//	);
//
//	// Viewport
//	{
//		D3D11_VIEWPORT viewPort; // レンダリングされる画面範囲
//		viewPort.TopLeftX	= 0.0f;
//		viewPort.TopLeftY	= 0.0f;
//		viewPort.Width		= scast<float>( d3dRenderTargetDesc.Width  );
//		viewPort.Height		= scast<float>( d3dRenderTargetDesc.Height );
//		viewPort.MinDepth	= D3D11_MIN_DEPTH;
//		viewPort.MaxDepth	= D3D11_MAX_DEPTH;
//
//		Donya::GetImmediateContext()->RSSetViewports( 1, &viewPort );
//	}
//
//	#pragma endregion
//
//	camera.SetToHomePosition( { 16.0f, 16.0f, -16.0f }, { 0.0f, 0.0f, 0.0f } );
//	camera.SetPerspectiveProjectionMatrix( Common::ScreenWidthF() / Common::ScreenHeightF() );
//
//	return true;
//}
//
//void OldFramework::Update( float elapsedTime/*Elapsed seconds from last frame*/ )
//{
//#ifdef USE_IMGUI
//
//	ImGui_ImplDX11_NewFrame();
//	ImGui_ImplWin32_NewFrame();
//	ImGui::NewFrame();
//
//#endif
//
//#if DEBUG_MODE
//
//	if ( Donya::Keyboard::Trigger( 'C' ) && ( Donya::Keyboard::Press( VK_LCONTROL ) || Donya::Keyboard::Press( VK_RCONTROL ) ) )
//	{
//		bool breakPoint{};
//	}
//
//	// bool isAccept = meshes.empty();
//	bool isAccept = true;
//
//	if ( isAccept )
//	{
//		std::string prePath  = "D:\\D-Download\\ASSET_Models\\Free\\Distribution_FBX\\BLue Falcon";
//		std::string number{};
//		if ( Donya::Keyboard::Press( 'B' ) && Donya::Keyboard::Trigger( '1' ) ) { number = "001"; }
//		if ( Donya::Keyboard::Press( 'B' ) && Donya::Keyboard::Trigger( '2' ) ) { number = "002"; }
//		if ( Donya::Keyboard::Press( 'B' ) && Donya::Keyboard::Trigger( '3' ) ) { number = "003"; }
//		if ( Donya::Keyboard::Press( 'B' ) && Donya::Keyboard::Trigger( '4' ) ) { number = "004"; }
//		if ( Donya::Keyboard::Press( 'B' ) && Donya::Keyboard::Trigger( '5' ) ) { number = "005"; }
//		std::string postPath = "_cube.fbx";
//
//		if ( number != "" )
//		{
//			std::string filePath = prePath + number + postPath;
//
//			ReserveLoadFile( filePath );
//		}
//	}
//	if ( Donya::Keyboard::Press( 'B' ) && Donya::Keyboard::Trigger( 'F' ) && isAccept )
//	{
//		constexpr const char *BLUE_FALCON = "D:\\D-Download\\ASSET_Models\\Free\\Distribution_FBX\\BLue Falcon\\Blue Falcon.FBX";
//
//		ReserveLoadFile( BLUE_FALCON );
//	}
//	if ( Donya::Keyboard::Press( 'J' ) && Donya::Keyboard::Press( 'I' ) && Donya::Keyboard::Trigger( 'G' ) && isAccept )
//	{
//		constexpr const char *JIGGLYPUFF = "D:\\D-Download\\ASSET_Models\\Free\\Jigglypuff\\Fixed\\JigglypuffNEW.FBX";
//
//		ReserveLoadFile( JIGGLYPUFF );
//	}
//	if ( Donya::Keyboard::Press( 'O' ) && Donya::Keyboard::Press( 'B' ) && Donya::Keyboard::Trigger( 'J' ) && isAccept )
//	{
//		constexpr const char *OBJ_TEST = "D:\\D-Download\\ASSET_Models\\Free\\Distribution_OBJ\\Mr.Incredible\\Mr.Incredible.obj";
//
//		ReserveLoadFile( OBJ_TEST );
//	}
//	if ( Donya::Keyboard::Trigger( 'Q' ) && !meshes.empty() )
//	{
//		meshes.pop_back();
//	}
//
//#endif // DEBUG_MODE
//	
//	if ( Donya::Keyboard::Trigger( 'T' ) )
//	{
//		Donya::TogguleShowStateOfImGui();
//	}
//
//	if ( Donya::Keyboard::Trigger( 'R' ) )
//	{
//		camera.SetToHomePosition( { 16.0f, 16.0f, -16.0f }, { 0.0f, 0.0f, 0.0f } );
//	}
//
//	if ( Donya::Keyboard::Trigger( 'F' ) && !Donya::Keyboard::Press( 'B' ) )
//	{
//		isSolidState = !isSolidState;
//	}
//
//	AppendModelIfLoadFinished();
//
//	StartLoadIfVacant();
//
//	ShowNowLoadingModels();
//
//	Donya::Vector3 origin{ 0.0f, 0.0f, 0.0f };
//	camera.Update( origin );
//
//#if USE_IMGUI
//
//	if ( ImGui::BeginIfAllowed() )
//	{
//		ShowMouseInfo();
//		
//		camera.ShowImGuiNode();
//		ImGui::Text( "" );
//
//		ChangeLightByImGui();
//		ImGui::Text( "" );
//
//		if ( ImGui::Button( "Open FBX File" ) )
//		{
//			OpenCommonDialogAndFile();
//		}
//		ImGui::Text( "" );
//
//		ShowModelInfo();
//		ImGui::Text( "" );
//
//		ImGui::End();
//	}
//
//#endif // USE_IMGUI
//}
//
//#if DEBUG_MODE
//#include "Donya/StaticMesh.h"
//#endif // DEBUG_MODE
//void OldFramework::Render( float elapsedTime/*Elapsed seconds from last frame*/ )
//{
//	// ClearRenderTargetView, ClearDepthStencilView
//	{
//		const FLOAT fillColor[4] = { 0.45f, 0.45f, 0.45f, 1.0f };	// RGBA
//		Donya::GetImmediateContext()->ClearRenderTargetView
//		(
//			d3dRenderTargetView.Get(),
//			fillColor
//		);
//
//		Donya::GetImmediateContext()->ClearDepthStencilView
//		(
//			d3dDepthStencilView.Get(),
//			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
//			1.0f,	// 1.0f で Zbuffer を埋め尽くす
//			0		// 0.0f で Stencil を埋め尽くす
//		);
//	}
//
//	/*
//	Donya::GetImmediateContext()->OMSetRenderTargets
//	(
//		1,
//		d3dRenderTargetView.GetAddressOf(),
//		d3dDepthStencilView.Get()
//	);
//	*/
//	
//	XMMATRIX W{};
//	{
//		static float scale	= 0.1f; // 0.1f;
//		static float angleX	= 0.0f; // -10.0f;
//		static float angleY	= 0.0f; // -160.0f;
//		static float angleZ	= 0.0f; // 0;
//		static float moveX	= 0.0f; // 2.0f;
//		static float moveY	= 0.0f; // -2.0f;
//		static float moveZ	= 0.0f; // 0;
//
//		if ( 0 )
//		{
//			constexpr float SCALE_ADD = 0.0012f;
//			constexpr float ANGLE_ADD = 0.12f;
//			constexpr float MOVE_ADD  = 0.04f;
//
//			if ( Donya::Keyboard::Press( 'W'		) ) { scale  += SCALE_ADD; }
//			if ( Donya::Keyboard::Press( 'S'		) ) { scale  -= SCALE_ADD; }
//			if ( Donya::Keyboard::Press( VK_UP		) ) { angleX += ANGLE_ADD; }
//			if ( Donya::Keyboard::Press( VK_DOWN	) ) { angleX -= ANGLE_ADD; }
//			if ( Donya::Keyboard::Press( VK_LEFT	) ) { angleY += ANGLE_ADD; }
//			if ( Donya::Keyboard::Press( VK_RIGHT	) ) { angleY -= ANGLE_ADD; }
//			if ( Donya::Keyboard::Press( 'A'		) ) { angleZ += ANGLE_ADD; }
//			if ( Donya::Keyboard::Press( 'D'		) ) { angleZ -= ANGLE_ADD; }
//			if ( Donya::Keyboard::Press( 'I'		) ) { moveY  += MOVE_ADD;  }
//			if ( Donya::Keyboard::Press( 'K'		) ) { moveY  -= MOVE_ADD;  }
//			if ( Donya::Keyboard::Press( 'L'		) ) { moveX  += MOVE_ADD;  }
//			if ( Donya::Keyboard::Press( 'J'		) ) { moveX  -= MOVE_ADD;  }
//		}
//
//		XMMATRIX S	= XMMatrixScaling( scale, scale, scale );
//		XMMATRIX RX	= XMMatrixRotationX( ToRadian( angleX ) );
//		XMMATRIX RY	= XMMatrixRotationY( ToRadian( angleY ) );
//		XMMATRIX RZ	= XMMatrixRotationZ( ToRadian( angleZ ) );
//		XMMATRIX R	= ( RZ * RY ) * RX;
//		XMMATRIX T	= XMMatrixTranslation( moveX, moveY, moveZ );
//
//		W = S * R * T;
//	}
//
//	XMMATRIX V = camera.CalcViewMatrix();
//
//	XMFLOAT4X4 worldViewProjection{};
//	{
//		XMMATRIX projPerspective = camera.GetProjectionMatrix();
//
//		XMStoreFloat4x4
//		(
//			&worldViewProjection,
//			DirectX::XMMatrixMultiply( W, DirectX::XMMatrixMultiply( V, projPerspective ) )
//		);
//	}
//
//	XMFLOAT4X4 world{};
//	XMStoreFloat4x4( &world, W );
//
//	XMFLOAT4 cameraPos{};
//	{
//		XMFLOAT3 ref = camera.GetPos();
//		cameraPos.x = ref.x;
//		cameraPos.y = ref.y;
//		cameraPos.z = ref.z;
//		cameraPos.w = 1.0f;
//	}
//
//	for ( auto &it : meshes )
//	{
//		it.mesh.Render( worldViewProjection, world, cameraPos, light.color, light.direction, isSolidState );
//	}
//
//#if DEBUG_MODE
//	{
//		auto InitializedStaticMesh = [&]( std::string fullPath )
//		{
//			Donya::Loader loader{};
//			loader.Load( fullPath, nullptr );
//
//			return Donya::StaticMesh::Create( loader );
//		};
//		static std::shared_ptr<Donya::StaticMesh> pStaticMeshFBX = InitializedStaticMesh( "D:\\Captures\\Player.bin" );
//		static std::shared_ptr<Donya::StaticMesh> pStaticMeshOBJ = InitializedStaticMesh( "D:\\Captures\\Box.bin" );
//		if ( pStaticMeshFBX )
//		// if ( pStaticMeshOBJ )
//		{
//			// pStaticMeshFBX->Render( worldViewProjection, world, light.direction, light.color, cameraPos );
//			// pStaticMeshOBJ->Render( worldViewProjection, world, light.direction, light.color, cameraPos );
//		}
//	}
//#endif // DEBUG_MODE
//
//#if USE_IMGUI
//
//	ImGui::Render();
//
//	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
//
//#endif
//
//	HRESULT hr = dxgiSwapChain->Present( 0, 0 );
//	_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Present()" );
//}
//
//void OldFramework::ReserveLoadFile( std::string filePath )
//{
//	/*
//	一時メモ：
//	Donyaのストレージから新しいものが来るたびにひっぱってきて，
//	すべてに対しCanLoadFile()を検証，合格したなら予約リストに追加する。
//	*/
//
//	auto CanLoadFile = []( std::string filePath )->bool
//	{
//		constexpr std::array<const char *, 5> EXTENSIONS
//		{
//			".obj", ".OBJ",
//			".fbx", ".FBX",
//			".bin"
//		};
//
//		for ( size_t i = 0; i < EXTENSIONS.size(); ++i )
//		{
//			if ( filePath.find( EXTENSIONS[i] ) != std::string::npos )
//			{
//				return true;
//			}
//		}
//
//		return false;
//	};
//
//	if ( CanLoadFile( filePath ) )
//	{
//		reservedAbsFilePaths.push( filePath );
//
//		const std::string fileName = Donya::ExtractFileNameFromFullPath( filePath );
//		if ( !fileName.empty() )
//		{
//			reservedFileNamesUTF8.push( Donya::MultiToUTF8( fileName ) );
//		}
//		else
//		{
//			reservedFileNamesUTF8.push( Donya::MultiToUTF8( filePath ) );
//		}
//	}
//}
//
//void OldFramework::StartLoadIfVacant()
//{
//	if ( pCurrentLoading )		{ return; }
//	if ( reservedAbsFilePaths.empty() )	{ return; }
//	// else
//
//	auto Load = []( std::string filePath, AsyncLoad *pElement )
//	{
//		if ( !pElement ) { return; }
//		// else
//
//		HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
//
//		if ( FAILED( hr ) )
//		{
//			std::lock_guard<std::mutex> lock( pElement->flagMutex );
//
//			pElement->isFinished  = true;
//			pElement->isSucceeded = false;
//			return;
//		}
//
//		bool createResult = false; // Will be change by below process, if load succeeded.
//
//		// Load model, using lock_guard by pLoadMutex.
//		{
//			Donya::Loader tmpHeavyLoad{}; // For reduce time of lock.
//			bool loadResult = tmpHeavyLoad.Load( filePath, nullptr );
//
//			std::lock_guard<std::mutex> lock( pElement->meshMutex );
//
//			// bool loadResult  = pElement->meshInfo.loader.Load( fullPath, nullptr );
//			pElement->meshInfo.loader = tmpHeavyLoad; // Takes only assignment-time.
//			if ( loadResult )
//			{
//				createResult = Donya::SkinnedMesh::Create
//				(
//					&pElement->meshInfo.loader,
//					&pElement->meshInfo.mesh
//				);
//
//			}
//		}
//
//		std::lock_guard<std::mutex> lock( pElement->flagMutex );
//
//		pElement->isFinished  = true;
//		pElement->isSucceeded = createResult;
//
//		CoUninitialize();
//	};
//
//	const std::string loadFilePath = reservedAbsFilePaths.front();
//	reservedAbsFilePaths.pop();
//	reservedFileNamesUTF8.pop();
//
//	currentLoadingFileNameUTF8 = Donya::ExtractFileNameFromFullPath( loadFilePath );
//
//	pCurrentLoading = std::make_unique<AsyncLoad>();
//	pLoadThread = std::make_unique<std::thread>( Load, loadFilePath, pCurrentLoading.get() );
//}
//
//void OldFramework::AppendModelIfLoadFinished()
//{
//	if ( !pCurrentLoading ) { return; }
//	// else
//
//	{
//		std::lock_guard<std::mutex> flagLock( pCurrentLoading->flagMutex );
//
//		if ( !pCurrentLoading->isFinished ) { return; }
//		// else
//
//		if ( pLoadThread && pLoadThread->joinable() )
//		{
//			pLoadThread->join();
//		}
//
//		if ( pCurrentLoading->isSucceeded )
//		{
//			std::lock_guard<std::mutex> meshLock( pCurrentLoading->meshMutex );
//
//			meshes.emplace_back( pCurrentLoading->meshInfo );
//		}
//	}
//
//	pCurrentLoading.reset( nullptr );
//}
//
//void OldFramework::ShowNowLoadingModels()
//{
//#if USE_IMGUI
//
//	if ( !pCurrentLoading ) { return; }
//	// else
//
//	const Donya::Vector2 WINDOW_POS{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };
//	const Donya::Vector2 WINDOW_SIZE{ 360.0f, 180.0f };
//	auto Convert = []( const Donya::Vector2 &vec )
//	{
//		return ImVec2{ vec.x, vec.y };
//	};
//
//	ImGui::SetNextWindowPos( Convert( WINDOW_POS ), ImGuiCond_Once );
//	ImGui::SetNextWindowSize( Convert( WINDOW_SIZE ), ImGuiCond_Once );
//
//	if ( ImGui::BeginIfAllowed( "Loading Files" ) )
//	{
//		std::queue<std::string> fileListUTF8 = reservedFileNamesUTF8;
//		ImGui::Text( "Reserving load file list : %d", fileListUTF8.size() + 1 );
//
//		ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), ImVec2( 0, 0 ) );
//
//		std::string fileNameUTF8 = currentLoadingFileNameUTF8;
//		ImGui::Text( "Now:[%s]", fileNameUTF8.c_str() );
//		while ( !fileListUTF8.empty() )
//		{
//			fileNameUTF8 = fileListUTF8.front();
//			ImGui::Text( "[%s]", fileNameUTF8.c_str() );
//			fileListUTF8.pop();
//		}
//
//		ImGui::EndChild();
//
//		ImGui::End();
//	}
//
//#endif // USE_IMGUI
//}
//
//bool OldFramework::OpenCommonDialogAndFile()
//{
//	char chosenFilesFullPath[MAX_PATH]	= { 0 };
//	char chosenFileName[MAX_PATH]		= { 0 };
//
//	OPENFILENAMEA ofn{ 0 };
//	ofn.lStructSize		= sizeof( OPENFILENAME );
//	ofn.hwndOwner		= hWnd;
//	ofn.lpstrFilter		= "FBX-file(*.fbx)\0*.fbx\0"
//						  "OBJ-file(*.obj)\0*.obj\0"
//						  "Binary-file(*.bin)\0*.bin\0"
//						  "\0";
//	ofn.lpstrFile		= chosenFilesFullPath;
//	ofn.nMaxFile		= MAX_PATH;
//	ofn.lpstrFileTitle	= chosenFileName;
//	ofn.nMaxFileTitle	= MAX_PATH;
//	ofn.Flags			= OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR; // If not set OFN_NOCHANGEDIR flag, the current directory will be changed, so the SkinnedMesh can't use current directory.
//
//	// TODO:Support multiple files.
//
//	auto result = GetOpenFileNameA( &ofn );
//	if ( !result ) { return false; }
//	// else
//
//	std::string filePath( chosenFilesFullPath );
//
//	ReserveLoadFile( filePath );
//	/*
//	std::string errorMessage{};
//	meshes.push_back( {} );
//	meshes.back().loader.Load( filePath, &errorMessage );
//	Donya::SkinnedMesh::Create( &meshes.back().loader, &meshes.back().mesh );
//	*/
//
//	return true;
//}
//
//std::string GetSaveFileNameByCommonDialog( const HWND &hWnd )
//{
//	char fileNameBuffer[MAX_PATH]	= { 0 };
//	char titleBuffer[MAX_PATH]		= { 0 };
//
//	OPENFILENAMEA ofn{ 0 };
//	ofn.lStructSize		= sizeof( OPENFILENAME );
//	ofn.hwndOwner		= hWnd;
//	ofn.lpstrFilter		= "Binary-file(*.bin)\0*.bin\0"
//						  "\0";
//	ofn.lpstrFile		= fileNameBuffer;
//	ofn.nMaxFile		= MAX_PATH;
//	ofn.lpstrFileTitle	= titleBuffer;
//	ofn.nMaxFileTitle	= MAX_PATH;
//	ofn.Flags			= OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR; // If not set OFN_NOCHANGEDIR flag, the current directory will be changed, so the SkinnedMesh can't use current directory.
//
//	auto result = GetSaveFileNameA( &ofn );
//	if ( !result ) { return std::string{}; }
//	// else
//
//	std::string filePath( ofn.lpstrFile );
//	return filePath;
//}
//
//void OldFramework::SetMouseCapture()
//{
//	SetCapture( hWnd );
//	isCaptureWindow = true;
//}
//void OldFramework::ReleaseMouseCapture()
//{
//	bool result = ReleaseCapture();
//	isCaptureWindow = false;
//
//#if DEBUG_MODE
//
//	static unsigned int errorCount = 0;
//	if ( !result ) { errorCount++; }
//
//#endif // DEBUG_MODE
//}
//
//void OldFramework::PutLimitMouseMoveArea()
//{
//	constexpr int ADJUST = 8;
//	const POINT MOUSE_SIZE = Donya::Mouse::GetMouseSize();
//
//	// [0]:Left, [1]:Right.
//	const std::array<int, 2> EDGE_X
//	{
//		0 + MOUSE_SIZE.x,
//		Common::ScreenWidth() - MOUSE_SIZE.x
//	};
//
//	// [0]:Up, [1]:Down.
//	const std::array<int, 2> EDGE_Y
//	{
//		0 + MOUSE_SIZE.y,
//		Common::ScreenHeight() - MOUSE_SIZE.y
//	};
//
//	int mx{}, my{};
//	Donya::Mouse::GetMouseCoord( &mx, &my );
//
//	bool isReset = false;
//
//	if ( mx < EDGE_X[0] )
//	{
//		mx = EDGE_X[1] - ADJUST;
//		isReset = true;
//	}
//	else
//	if ( EDGE_X[1] < mx )
//	{
//		mx = EDGE_X[0] + ADJUST;
//		isReset = true;
//	}
//
//	if ( my < EDGE_Y[0] )
//	{
//		my = EDGE_Y[1] - ADJUST;
//		isReset = true;
//	}
//	else
//	if ( EDGE_Y[1] < my )
//	{
//		my = EDGE_Y[0] + ADJUST;
//		isReset = true;
//	}
//
//	if ( isReset )
//	{
//		POINT client = GetClientCoordinate( hWnd );
//
//		SetCursorPos( client.x + mx, client.y + my );
//	}
//}
//
//void OldFramework::ShowMouseInfo()
//{
//#if USE_IMGUI
//
//	if ( ImGui::BeginIfAllowed() )
//	{
//		int x{}, y{};
//		Donya::Mouse::GetMouseCoord( &x, &y );
//
//		ImGui::Text( u8"マウス位置[X:%d][Y%d]", x, y );
//		ImGui::Text( u8"マウスホイール[%d]", Donya::Mouse::GetMouseWheelRot() );
//
//		ImGui::End();
//	}
//
//#endif // USE_IMGUI
//}
//void OldFramework::ShowModelInfo()
//{
//#if USE_IMGUI
//
//	if ( ImGui::BeginIfAllowed() )
//	{
//		size_t modelCount = meshes.size();
//		ImGui::Text( "Model Count:[%d]", modelCount );
//
//		for ( auto &it = meshes.begin(); it != meshes.end(); )
//		{
//			std::string nodeCaption = "[" + it->loader.GetOnlyFileName() + "]";
//			if ( ImGui::TreeNode( nodeCaption.c_str() ) )
//			{
//				if ( ImGui::Button( "Remove" ) )
//				{
//					it = meshes.erase( it );
//
//					ImGui::TreePop();
//					continue;
//				}
//				// else
//
//				if ( ImGui::Button( "Save" ) )
//				{
//					std::string saveName = GetSaveFileNameByCommonDialog( hWnd );
//					if ( saveName == std::string{} )
//					{
//						// Error.
//						char breakpoint = 0;
//					}
//					else
//					{
//						if ( saveName.find( ".bin" ) == std::string::npos )
//						{
//							saveName += ".bin";
//						}
//						it->loader.SaveByCereal( saveName );
//					}
//				}
//
//				it->loader.EnumPreservingDataToImGui( ImGuiWindowName );
//				ImGui::TreePop();
//			}
//
//			++it;
//		}
//
//		ImGui::End();
//	}
//
//#endif // USE_IMGUI
//}
//void OldFramework::ChangeLightByImGui()
//{
//#if USE_IMGUI 
//
//	if ( ImGui::BeginIfAllowed() )
//	{
//		ImGui::ColorEdit4( "Light Color", &light.color.x );
//		ImGui::SliderFloat3( "Light Direction", &light.direction.x, -8.0f, 8.0f );
//
//		ImGui::End();
//	}
//
//#endif // USE_IMGUI
//}

// OLD_Framework
#pragma endregion
