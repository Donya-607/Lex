#include "Framework.h"

#include <array>
#include <algorithm>
#include <thread>

#include "Benchmark.h"
#include "Camera.h"
#include "Common.h"
#include "Donya.h"
#include "Keyboard.h"
#include "Loader.h"
#include "Mouse.h"
#include "Resource.h"
#include "UseImGui.h"
#include "Useful.h"
#include "WindowsUtil.h"

#if USE_IMGUI

extern LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam );

#endif // USE_IMGUI

using namespace DirectX;

static constexpr char *ImGuiWindowName = "File Information";

Framework::Framework( HWND hwnd ) :
	hWnd( hwnd ),
	camera(),
	light(),
	meshes(),
	pressMouseButton( NULL ),
	isCaptureWindow( false ),
	isSolidState( true ),
	// mutex(),
	pLoadThread( nullptr ),
	pCurrentLoading( nullptr ),
	currentLoadingFilePath(),
	absFilePaths()
{
	DragAcceptFiles( hWnd, TRUE );
}
Framework::~Framework()
{
	if ( isCaptureWindow )
	{
		ReleaseMouseCapture();
	}

	meshes.clear();
	meshes.shrink_to_fit();

	if ( pLoadThread && pLoadThread->joinable() )
	{
		pLoadThread->join();
	}
};

LRESULT CALLBACK Framework::HandleMessage( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
#ifdef USE_IMGUI
	if ( ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) )
	{
		return 1;
	}

#endif

	switch ( msg )
	{
	case WM_CREATE:
		break;
	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;
	case WM_DROPFILES:
		{
			constexpr size_t FILE_PATH_LENGTH = 512U;

			HDROP	hDrop		= ( HDROP )wParam;
			size_t	fileCount	= DragQueryFile( hDrop, -1, NULL, NULL );

			std::string errorMessage{};
			std::unique_ptr<char[]> filename = std::make_unique<char[]>( FILE_PATH_LENGTH );
			
			for ( size_t i = 0; i < fileCount; ++i )
			{
				DragQueryFileA( hDrop, i, filename.get(), FILE_PATH_LENGTH );

				ReserveLoadFile( std::string{ filename.get() } );

				/*
				meshes.push_back( {} );
				bool result = meshes.back().loader.Load( filename.get(), &errorMessage );
				if ( !result )
				{
					MessageBoxA( hWnd, errorMessage.c_str(), "File Load Failed", MB_OK );
					continue;
				}
				// else
				Donya::SkinnedMesh::Create( &meshes.back().loader, &meshes.back().mesh );
				*/
			}

			DragFinish( hDrop );
		}
		break;
	case WM_ENTERSIZEMOVE:
		{
			// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
			highResoTimer.Stop();
		}
		break;
	case WM_EXITSIZEMOVE:
		{
			// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
			// Here we reset everything based on the new window dimensions.
			highResoTimer.Start();
		}
		break;
	case WM_KEYDOWN:
		{
			if ( wParam == VK_ESCAPE )
			{
				PostMessage( hWnd, WM_CLOSE, 0, 0 );
			}
		}
		break;
	#pragma region Mouse Process
	case WM_MOUSEMOVE:
		Donya::Mouse::UpdateMouseCoordinate( lParam );
		if ( isCaptureWindow )
		{
			PutLimitMouseMoveArea();
		}
		break;
	case WM_MOUSEWHEEL:
		Donya::Mouse::CalledMouseWheelMessage( /* isVertical = */ true, wParam, lParam );
		break;
	case WM_MOUSEHWHEEL:
		Donya::Mouse::CalledMouseWheelMessage( /* isVertical = */ false, wParam, lParam );
		break;
	case WM_LBUTTONDOWN:
		if ( !pressMouseButton )
		{
			pressMouseButton = VK_LBUTTON;
			SetMouseCapture();
		}
		break;
	case WM_MBUTTONDOWN:
		if ( !pressMouseButton )
		{
			pressMouseButton = VK_MBUTTON;
			SetMouseCapture();
		}
		break;
	case WM_RBUTTONDOWN:
		if ( !pressMouseButton )
		{
			pressMouseButton = VK_RBUTTON;
			SetMouseCapture();
		}
		break;
	case WM_LBUTTONUP:
		if ( isCaptureWindow )
		{
			if ( pressMouseButton == VK_LBUTTON || !pressMouseButton )
			{
				ReleaseMouseCapture();
			}

			pressMouseButton = NULL;
		}
		break;
	case WM_MBUTTONUP:
		if ( isCaptureWindow )
		{
			if ( pressMouseButton == VK_MBUTTON || !pressMouseButton )
			{
				ReleaseMouseCapture();
			}

			pressMouseButton = NULL;
		}
		break;
	case WM_RBUTTONUP:
		if ( isCaptureWindow )
		{
			if ( pressMouseButton == VK_RBUTTON || !pressMouseButton )
			{
				ReleaseMouseCapture();
			}

			pressMouseButton = NULL;
		}
		break;
	#pragma endregion
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			hdc = BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );
			break;
		}
	default:
		return DefWindowProc( hWnd, msg, wParam, lParam );
	}
	return 0;
}

void Framework::CalcFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
	static int frames = 0;
	static float timeTlapsed = 0.0f;

	frames++;

	// Compute averages over one second period.
	if ( ( highResoTimer.TimeStamp() - timeTlapsed ) >= 1.0f )
	{
		float fps	= scast<float>( frames ); // fps = frameCnt / 1
		float mspf	= 1000.0f / fps;
		std::ostringstream oss;
		oss.precision( 6 );
		oss	<< TITLE_BAR_CAPTION << " : "
			<< "[FPS : " << fps << "] "
			<< "[Frame Time : " << mspf << " (ms)]";
		SetWindowTextA( hWnd, oss.str().c_str() );

		// Reset for next average.
		frames = 0;
		timeTlapsed += 1.0f;
	}
}

int Framework::Run()
{
	if ( !Init() ) { return 0; }
	// else

#ifdef USE_IMGUI

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init( hWnd );
	ImGui_ImplDX11_Init( Donya::GetDevice(), Donya::GetImmediateContext() );
	// ImGui::StyleColorsClassic();
	// ImGui::StyleColorsLight();
	ImGui::StyleColorsDark();

	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\meiryo.ttc", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	// io.Fonts->AddFontFromFileTTF( "c:\\Windows\\Fonts\\consolab.ttf", 10.0f, NULL, io.Fonts->GetGlyphRangesJapanese() );
	// io.Fonts->AddFontFromFileTTF(".\\Inconsolata-Bold.ttf", 12.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

#endif

	MSG msg{};

	while ( WM_QUIT != msg.message )
	{
		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			Donya::Keyboard::Update();

			highResoTimer.Tick();
			CalcFrameStats();
			Update( highResoTimer.timeInterval() );

			Donya::Mouse::ResetMouseWheelRot();

			Render( highResoTimer.timeInterval() );
		}
	}

	Donya::Resource::ReleaseAllCachedResources();

#ifdef USE_IMGUI

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

#endif

	BOOL isFullScreen = FALSE;
	dxgiSwapChain->GetFullscreenState( &isFullScreen, 0 );
	if ( isFullScreen )
	{
		dxgiSwapChain->SetFullscreenState( FALSE, 0 );
	}

	Donya::Uninit();

	return static_cast<int>( msg.wParam );
}

bool Framework::Init()
{
	#pragma region DirectX

	Donya::Init( nullptr );	// Doing initialize of ID3D11Device and ID3D11DeviceContext.

	HRESULT hr = S_OK;

	// Create Swapchain
	{
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory( &sd, sizeof( sd ) );
		sd.BufferCount							= 1;
		sd.BufferDesc.Width						= Common::ScreenWidth();	// Size of back-buffer.
		sd.BufferDesc.Height					= Common::ScreenHeight();	// Size of back-buffer.
		sd.BufferDesc.Format					= DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator		= 60;	// 
		sd.BufferDesc.RefreshRate.Denominator	= 1;	// 分母
		sd.BufferUsage							= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow							= hWnd;
		sd.SampleDesc.Count						= 1;
		sd.SampleDesc.Quality					= 0;
		sd.Windowed								= TRUE;
		UINT sampleCount = 1;
		UINT msaaQualityLevel;
		hr = Donya::GetDevice()->CheckMultisampleQualityLevels( sd.BufferDesc.Format, sampleCount, &msaaQualityLevel );
		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateMultisampleQualityLevels" );
		sd.SampleDesc.Count		= sampleCount;
		sd.SampleDesc.Quality	= msaaQualityLevel - 1;


		Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
		hr = CreateDXGIFactory( __uuidof( IDXGIFactory ), reinterpret_cast<void**>( dxgiFactory.GetAddressOf() ) );
		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateDXGIFactory" );

		{
			// default adapter.
			Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
			hr = dxgiFactory->EnumAdapters( 0, dxgiAdapter.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : EnumAdapters" );
		}

		hr = dxgiFactory->CreateSwapChain
		(
			Donya::GetDevice(),
			&sd,
			dxgiSwapChain.GetAddressOf()
		);
		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateSwapChain" );
	}


	D3D11_TEXTURE2D_DESC d3dRenderTargetDesc = { 0 };
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> d3dBackBuffer;
		hr = dxgiSwapChain->GetBuffer
		(
			0, 
			__uuidof(ID3D11Texture2D),
			reinterpret_cast<void **>( d3dBackBuffer.GetAddressOf() )
		);
		/*
		GetBuffer()により， d3dBackBuffer の参照カウントが１増える
		なのでComPtrを使わない場合は，Release()を呼び出して参照カウントを減らさなければならない
		*/
		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : GetBuffer" );

		hr = Donya::GetDevice()->CreateRenderTargetView
		(
			d3dBackBuffer.Get(),
			nullptr,
			d3dRenderTargetView.GetAddressOf()
		);
		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateRenderTargetView" );

		d3dBackBuffer->GetDesc( &d3dRenderTargetDesc );
	}
	D3D11_TEXTURE2D_DESC d3dDepthStencilDesc = d3dRenderTargetDesc;
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> d3dTexture2D;
		d3dDepthStencilDesc.MipLevels		= 1;
		d3dDepthStencilDesc.ArraySize		= 1;
		d3dDepthStencilDesc.Format			= DXGI_FORMAT_R24G8_TYPELESS;
		d3dDepthStencilDesc.Usage			= D3D11_USAGE_DEFAULT;
		d3dDepthStencilDesc.BindFlags		= D3D11_BIND_DEPTH_STENCIL;
		d3dDepthStencilDesc.CPUAccessFlags	= 0;
		d3dDepthStencilDesc.MiscFlags		= 0;
		hr = Donya::GetDevice()->CreateTexture2D
		(
			&d3dDepthStencilDesc,
			NULL,
			d3dTexture2D.GetAddressOf()
		);
		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateTexture2D" );

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		depthStencilViewDesc.Format				= DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension		= D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Flags				= 0;
		depthStencilViewDesc.Texture2D.MipSlice	= 0;
		hr = Donya::GetDevice()->CreateDepthStencilView
		(
			d3dTexture2D.Get(),
			&depthStencilViewDesc,
			d3dDepthStencilView.GetAddressOf()
		);
		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateDepthStencilView" );
	}

	Donya::GetImmediateContext()->OMSetRenderTargets
	(
		1,
		d3dRenderTargetView.GetAddressOf(),
		d3dDepthStencilView.Get()
	);

	// Viewport
	{
		D3D11_VIEWPORT viewPort; // レンダリングされる画面範囲
		viewPort.TopLeftX	= 0.0f;
		viewPort.TopLeftY	= 0.0f;
		viewPort.Width		= scast<float>( d3dRenderTargetDesc.Width  );
		viewPort.Height		= scast<float>( d3dRenderTargetDesc.Height );
		viewPort.MinDepth	= D3D11_MIN_DEPTH;
		viewPort.MaxDepth	= D3D11_MAX_DEPTH;

		Donya::GetImmediateContext()->RSSetViewports( 1, &viewPort );
	}

	#pragma endregion

	camera.SetToHomePosition( { 0.0f, 0.0f, -16.0f} );
	camera.SetPerspectiveProjectionMatrix( Common::ScreenWidthF() / Common::ScreenHeightF() );

	return true;
}

void Framework::Update( float elapsedTime/*Elapsed seconds from last frame*/ )
{
#ifdef USE_IMGUI

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

#endif

#if DEBUG_MODE

	if ( Donya::Keyboard::Trigger( 'C' ) && ( Donya::Keyboard::Press( VK_LCONTROL ) || Donya::Keyboard::Press( VK_RCONTROL ) ) )
	{
		bool breakPoint{};
	}

	if ( Donya::Keyboard::Trigger( 'T' ) )
	{
		Donya::TogguleShowStateOfImGui();
	}

	if ( Donya::Keyboard::Trigger( 'R' ) )
	{
		camera.SetToHomePosition( { 0.0f, 0.0f, -16.0f } );
	}

	// bool isAccept = meshes.empty();
	bool isAccept = true;

	if ( isAccept )
	{
		std::string prePath  = "D:\\D-Download\\ASSET_Models\\Free\\Distribution_FBX\\BLue Falcon";
		std::string number{};
		if ( Donya::Keyboard::Trigger( '1' ) ) { number = "001"; }
		if ( Donya::Keyboard::Trigger( '2' ) ) { number = "002"; }
		if ( Donya::Keyboard::Trigger( '3' ) ) { number = "003"; }
		if ( Donya::Keyboard::Trigger( '4' ) ) { number = "004"; }
		if ( Donya::Keyboard::Trigger( '5' ) ) { number = "005"; }
		std::string postPath = "_cube.fbx";

		if ( number != "" )
		{
			std::string filePath = prePath + number + postPath;

			ReserveLoadFile( filePath );
		}
	}
	if ( Donya::Keyboard::Press( 'B' ) && Donya::Keyboard::Trigger( 'F' ) && isAccept )
	{
		constexpr const char *BLUE_FALCON = "D:\\D-Download\\ASSET_Models\\Free\\Distribution_FBX\\BLue Falcon\\Blue Falcon.FBX";

		ReserveLoadFile( BLUE_FALCON );
	}
	if ( Donya::Keyboard::Press( 'J' ) && Donya::Keyboard::Press( 'I' ) && Donya::Keyboard::Trigger( 'G' ) && isAccept )
	{
		constexpr const char *JIGGLYPUFF = "D:\\D-Download\\ASSET_Models\\Free\\Jigglypuff\\Fixed\\JigglypuffNEW.FBX";

		ReserveLoadFile( JIGGLYPUFF );
	}
	if ( Donya::Keyboard::Trigger( 'Q' ) && !meshes.empty() )
	{
		meshes.pop_back();
	}

#endif // DEBUG_MODE

	if ( Donya::Keyboard::Trigger( 'F' ) && !Donya::Keyboard::Press( 'B' ) )
	{
		isSolidState = !isSolidState;
	}

	AppendModelIfLoadFinished();

	StartLoadIfVacant();

	ShowNowLoadingModels();

	Donya::Vector3 origin{ 0.0f, 0.0f, 0.0f };
	camera.Update( origin );

#if USE_IMGUI && DEBUG_MODE

	if ( ImGui::BeginIfAllowed() )
	{
		ShowMouseInfo();
		
		camera.ShowParametersToImGui();
		ImGui::Text( "" );

		ChangeLightByImGui();
		ImGui::Text( "" );

		if ( ImGui::Button( "Open FBX File" ) )
		{
			OpenCommonDialogAndFile();
		}
		ImGui::Text( "" );

		ShowModelInfo();
		ImGui::Text( "" );

		ImGui::End();
	}

#endif // USE_IMGUI && DEBUG_MODE
}

void Framework::Render( float elapsedTime/*Elapsed seconds from last frame*/ )
{
	// ClearRenderTargetView, ClearDepthStencilView
	{
		const FLOAT fillColor[4] = { 0.1f, 0.2f, 0.1f, 1.0f };	// RGBA
		Donya::GetImmediateContext()->ClearRenderTargetView
		(
			d3dRenderTargetView.Get(),
			fillColor
		);

		Donya::GetImmediateContext()->ClearDepthStencilView
		(
			d3dDepthStencilView.Get(),
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
			1.0f,	// 1.0f で Zbuffer を埋め尽くす
			0		// 0.0f で Stencil を埋め尽くす
		);
	}

	/*
	Donya::GetImmediateContext()->OMSetRenderTargets
	(
		1,
		d3dRenderTargetView.GetAddressOf(),
		d3dDepthStencilView.Get()
	);
	*/
	
	XMMATRIX W{};
	{
		static float scale	= 0.1f; // 0.1f;
		static float angleX	= 0.0f; // -10.0f;
		static float angleY	= 0.0f; // -160.0f;
		static float angleZ	= 0.0f; // 0;
		static float moveX	= 0.0f; // 2.0f;
		static float moveY	= 0.0f; // -2.0f;
		static float moveZ	= 0.0f; // 0;

		if ( 0 )
		{
			constexpr float SCALE_ADD = 0.0012f;
			constexpr float ANGLE_ADD = 0.12f;
			constexpr float MOVE_ADD  = 0.04f;

			if ( Donya::Keyboard::Press( 'W'		) ) { scale  += SCALE_ADD; }
			if ( Donya::Keyboard::Press( 'S'		) ) { scale  -= SCALE_ADD; }
			if ( Donya::Keyboard::Press( VK_UP		) ) { angleX += ANGLE_ADD; }
			if ( Donya::Keyboard::Press( VK_DOWN	) ) { angleX -= ANGLE_ADD; }
			if ( Donya::Keyboard::Press( VK_LEFT	) ) { angleY += ANGLE_ADD; }
			if ( Donya::Keyboard::Press( VK_RIGHT	) ) { angleY -= ANGLE_ADD; }
			if ( Donya::Keyboard::Press( 'A'		) ) { angleZ += ANGLE_ADD; }
			if ( Donya::Keyboard::Press( 'D'		) ) { angleZ -= ANGLE_ADD; }
			if ( Donya::Keyboard::Press( 'I'		) ) { moveY  += MOVE_ADD;  }
			if ( Donya::Keyboard::Press( 'K'		) ) { moveY  -= MOVE_ADD;  }
			if ( Donya::Keyboard::Press( 'L'		) ) { moveX  += MOVE_ADD;  }
			if ( Donya::Keyboard::Press( 'J'		) ) { moveX  -= MOVE_ADD;  }
		}

		XMMATRIX S	= XMMatrixScaling( scale, scale, scale );
		XMMATRIX RX	= XMMatrixRotationX( ToRadian( angleX ) );
		XMMATRIX RY	= XMMatrixRotationY( ToRadian( angleY ) );
		XMMATRIX RZ	= XMMatrixRotationZ( ToRadian( angleZ ) );
		XMMATRIX R	= ( RZ * RY ) * RX;
		XMMATRIX T	= XMMatrixTranslation( moveX, moveY, moveZ );

		W = S * R * T;
	}

	XMMATRIX V = camera.CalcViewMatrix();

	XMFLOAT4X4 worldViewProjection{};
	{
		XMMATRIX projPerspective = camera.GetProjectionMatrix();

		XMStoreFloat4x4
		(
			&worldViewProjection,
			DirectX::XMMatrixMultiply( W, DirectX::XMMatrixMultiply( V, projPerspective ) )
		);
	}

	XMFLOAT4X4 world{};
	XMStoreFloat4x4( &world, W );

	XMFLOAT4 cameraPos{};
	{
		XMFLOAT3 ref = camera.GetPos();
		cameraPos.x = ref.x;
		cameraPos.y = ref.y;
		cameraPos.z = ref.z;
		cameraPos.w = 1.0f;
	}

	for ( auto &it : meshes )
	{
		it.mesh.Render( worldViewProjection, world, cameraPos, light.color, light.direction, isSolidState );
	}

#if USE_IMGUI

	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );

#endif

	HRESULT hr = dxgiSwapChain->Present( 0, 0 );
	_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Present()" );
}

void Framework::ReserveLoadFile( std::string filePath )
{
	auto CanLoadFile = []( std::string filePath )->bool
	{
		constexpr std::array<const char *, 4> EXTENSIONS
		{
			".obj", ".OBJ",
			".fbx", ".FBX"
		};

		for ( size_t i = 0; i < EXTENSIONS.size(); ++i )
		{
			if ( filePath.find( EXTENSIONS[i] ) != std::string::npos )
			{
				return true;
			}
		}

		return false;
	};

	if ( CanLoadFile( filePath ) )
	{
		absFilePaths.push( filePath );
	}
}

void Framework::StartLoadIfVacant()
{
	if ( pCurrentLoading )		{ return; }
	if ( absFilePaths.empty() )	{ return; }
	// else

	auto Load =
	[]( std::string filePath, AsyncLoad *pElement )
	{
		if ( !pElement ) { return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );

		if ( FAILED( hr ) )
		{
			std::lock_guard<std::mutex> lock( pElement->flagMutex );

			pElement->isFinished  = true;
			pElement->isSucceeded = false;
			return;
		}

		bool createResult = false; // Will be change by below process, if load succeeded.

		// Load model, using lock_guard by pLoadMutex.
		{
			Donya::Loader tmpHeavyLoad{}; // For reduce time of lock.
			bool loadResult = tmpHeavyLoad.Load( filePath, nullptr );

			std::lock_guard<std::mutex> lock( pElement->meshMutex );

			// bool loadResult  = pElement->meshInfo.loader.Load( fullPath, nullptr );
			pElement->meshInfo.loader = tmpHeavyLoad; // Takes only assignment-time.
			if ( loadResult )
			{
				createResult = Donya::SkinnedMesh::Create
				(
					&pElement->meshInfo.loader,
					&pElement->meshInfo.mesh
				);

			}
		}

		std::lock_guard<std::mutex> lock( pElement->flagMutex );

		pElement->isFinished  = true;
		pElement->isSucceeded = createResult;

		CoUninitialize();
	};

	currentLoadingFilePath = absFilePaths.front();
	absFilePaths.pop();

	pCurrentLoading = std::make_unique<AsyncLoad>();
	pLoadThread = std::make_unique<std::thread>( Load, currentLoadingFilePath, pCurrentLoading.get() );
}

void Framework::AppendModelIfLoadFinished()
{
	if ( !pCurrentLoading ) { return; }
	// else

	{
		std::lock_guard<std::mutex> flagLock( pCurrentLoading->flagMutex );

		if ( !pCurrentLoading->isFinished ) { return; }
		// else

		if ( pLoadThread && pLoadThread->joinable() )
		{
			pLoadThread->join();
		}

		if ( pCurrentLoading->isSucceeded )
		{
			std::lock_guard<std::mutex> meshLock( pCurrentLoading->meshMutex );

			meshes.emplace_back( pCurrentLoading->meshInfo );
		}
	}

	pCurrentLoading.reset( nullptr );
}

void Framework::ShowNowLoadingModels()
{
#if USE_IMGUI

	if ( !pCurrentLoading ) { return; }
	// else

	const Donya::Vector2 WINDOW_POS{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };
	const Donya::Vector2 WINDOW_SIZE{ 360.0f, 180.0f };
	auto Convert = []( const Donya::Vector2 &vec )
	{
		return ImVec2{ vec.x, vec.y };
	};

	ImGui::SetNextWindowPos( Convert( WINDOW_POS ), ImGuiCond_Once );
	ImGui::SetNextWindowSize( Convert( WINDOW_SIZE ), ImGuiCond_Once );

	if ( ImGui::BeginIfAllowed( "Loading Files" ) )
	{
		HWND hwndForError = hWnd;
		auto MakeFileName = [hwndForError]( std::string fullPath )->std::string
		{
			const std::string fileDir = Donya::AcquireDirectoryFromFullPath( fullPath );
			if ( fileDir == "" ) { return ""; }
			// else
			const std::string fileName = fullPath.substr( fileDir.size() );

			return fileName;
		};
		auto MakeUTF8Name = [&MakeFileName]( std::string fullPath )->std::string
		{
			std::string fileName = MakeFileName( fullPath );
			if ( fileName == "" ) { return "Error"; }
			// else

			return Donya::MultiToUTF8( fileName );
		};

		std::queue<std::string> fileList = absFilePaths;
		ImGui::Text( "Reserving load file list : %d", fileList.size() + 1 );

		ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), ImVec2( 0, 0 ) );

		std::string fileNameUTF8 = MakeUTF8Name( currentLoadingFilePath );
		ImGui::Text( "Now:[%s]", fileNameUTF8.c_str() );
		while ( !fileList.empty() )
		{
			fileNameUTF8 = MakeUTF8Name( fileList.front() );
			ImGui::Text( "[%s]", fileNameUTF8.c_str() );
			fileList.pop();
		}

		ImGui::EndChild();

		ImGui::End();
	}

#endif // USE_IMGUI
}

bool Framework::OpenCommonDialogAndFile()
{
	char chosenFilesFullPath[MAX_PATH]	= { 0 };
	char chosenFileName[MAX_PATH]		= { 0 };

	OPENFILENAMEA ofn{ 0 };
	ofn.lStructSize		= sizeof( OPENFILENAME );
	ofn.hwndOwner		= hWnd;
	ofn.lpstrFilter		= "FBX-file(*.fbx)\0*.fbx\0"
						  "OBJ-file(*.obj)\0*.obj\0"
						  "\0";
	ofn.lpstrFile		= chosenFilesFullPath;
	ofn.nMaxFile		= MAX_PATH;
	ofn.lpstrFileTitle	= chosenFileName;
	ofn.nMaxFileTitle	= MAX_PATH;
	ofn.Flags			= OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR; // If not set OFN_NOCHANGEDIR flag, the current directory will be changed, so the SkinnedMesh can't use current directory.

	// TODO:Support multiple files.

	auto result = GetOpenFileNameA( &ofn );
	if ( !result ) { return false; }
	// else

	std::string filePath( chosenFilesFullPath );

	ReserveLoadFile( filePath );
	/*
	std::string errorMessage{};
	meshes.push_back( {} );
	meshes.back().loader.Load( filePath, &errorMessage );
	Donya::SkinnedMesh::Create( &meshes.back().loader, &meshes.back().mesh );
	*/

	return true;
}

void Framework::SetMouseCapture()
{
	SetCapture( hWnd );
	isCaptureWindow = true;
}
void Framework::ReleaseMouseCapture()
{
	bool result = ReleaseCapture();
	isCaptureWindow = false;

#if DEBUG_MODE

	static unsigned int errorCount = 0;
	if ( !result ) { errorCount++; }

#endif // DEBUG_MODE
}

void Framework::PutLimitMouseMoveArea()
{
	constexpr int ADJUST = 8;
	const POINT MOUSE_SIZE = Donya::Mouse::GetMouseSize();

	// [0]:Left, [1]:Right.
	const std::array<int, 2> EDGE_X
	{
		0 + MOUSE_SIZE.x,
		Common::ScreenWidth() - MOUSE_SIZE.x
	};

	// [0]:Up, [1]:Down.
	const std::array<int, 2> EDGE_Y
	{
		0 + MOUSE_SIZE.y,
		Common::ScreenHeight() - MOUSE_SIZE.y
	};

	int mx{}, my{};
	Donya::Mouse::GetMouseCoord( &mx, &my );

	bool isReset = false;

	if ( mx < EDGE_X[0] )
	{
		mx = EDGE_X[1] - ADJUST;
		isReset = true;
	}
	else
	if ( EDGE_X[1] < mx )
	{
		mx = EDGE_X[0] + ADJUST;
		isReset = true;
	}

	if ( my < EDGE_Y[0] )
	{
		my = EDGE_Y[1] - ADJUST;
		isReset = true;
	}
	else
	if ( EDGE_Y[1] < my )
	{
		my = EDGE_Y[0] + ADJUST;
		isReset = true;
	}

	if ( isReset )
	{
		POINT client = GetClientCoordinate( hWnd );

		SetCursorPos( client.x + mx, client.y + my );
	}
}

void Framework::ShowMouseInfo()
{
#if USE_IMGUI && DEBUG_MODE

	if ( ImGui::BeginIfAllowed() )
	{
		int x{}, y{};
		Donya::Mouse::GetMouseCoord( &x, &y );

		ImGui::Text( "Mouse[X:%d][Y%d]", x, y );
		ImGui::Text( "Wheel[%d]", Donya::Mouse::GetMouseWheelRot() );

		ImGui::End();
	}

#endif // USE_IMGUI && DEBUG_MODE
}
void Framework::ShowModelInfo()
{
#if USE_IMGUI && DEBUG_MODE

	if ( ImGui::BeginIfAllowed() )
	{
		size_t modelCount = meshes.size();
		ImGui::Text( "Model Count:[%d]", modelCount );

		for ( auto &it = meshes.begin(); it != meshes.end(); )
		{
			std::string nodeCaption = "[" + it->loader.GetOnlyFileName() + "]";
			if ( ImGui::TreeNode( nodeCaption.c_str() ) )
			{
				if ( ImGui::Button( "Remove" ) )
				{
					it = meshes.erase( it );

					ImGui::TreePop();
					continue;
				}
				// else

				it->loader.EnumPreservingDataToImGui( ImGuiWindowName );
				ImGui::TreePop();
			}

			++it;
		}

		ImGui::End();
	}

#endif // USE_IMGUI && DEBUG_MODE
}
void Framework::ChangeLightByImGui()
{
#if USE_IMGUI && DEBUG_MODE

	if ( ImGui::BeginIfAllowed() )
	{
		ImGui::ColorEdit4( "Light Color", &light.color.x );
		ImGui::SliderFloat3( "Light Direction", &light.direction.x, -8.0f, 8.0f );

		ImGui::End();
	}

#endif // USE_IMGUI && DEBUG_MODE
}
