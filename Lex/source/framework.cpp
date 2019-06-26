#include "Framework.h"

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

#if USE_IMGUI

extern LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam );

#endif // USE_IMGUI

using namespace DirectX;

namespace Do = Donya;

Framework::Framework( HWND hwnd ) :
	hWnd( hwnd ),
	pCamera( nullptr ), meshes(),
	isFillDraw( true )
{
	DragAcceptFiles( hWnd, TRUE );
}
Framework::~Framework()
{
	meshes.clear();
	meshes.shrink_to_fit();
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
				meshes.push_back( {} );

				DragQueryFileA( hDrop, i, filename.get(), FILE_PATH_LENGTH );
				bool result = meshes.back().loader.Load( filename.get(), &errorMessage );
				if ( !result )
				{
					MessageBoxA( hWnd, errorMessage.c_str(), "File Load Failed", MB_OK );
					continue;
				}
				// else
				Donya::SkinnedMesh::Create( &meshes.back().loader, &meshes.back().pMesh );
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
	case WM_MOUSEMOVE:
		Donya::Mouse::UpdateMouseCoordinate( lParam );
		break;
	case WM_MOUSEWHEEL:
		Donya::Mouse::CalledMouseWheelMessage( /* isVertical = */ false, wParam, lParam );
		break;
	case WM_MOUSEHWHEEL:
		Donya::Mouse::CalledMouseWheelMessage( /* isVertical = */ true, wParam, lParam );
		break;
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
	//ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();
	ImGui::StyleColorsDark();

	ImGuiIO &io = ImGui::GetIO();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\meiryo.ttc", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	io.Fonts->AddFontFromFileTTF( "c:\\Windows\\Fonts\\consolab.ttf", 10.0f, NULL, io.Fonts->GetGlyphRangesJapanese() );
	//io.Fonts->AddFontFromFileTTF(".\\Inconsolata-Bold.ttf", 12.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

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

	std::string projectDir = "./";
	Donya::Resource::RegisterDirectoryOfVertexShader( ( projectDir + "Shader/" ).c_str() );
	Donya::Resource::RegisterDirectoryOfPixelShader ( ( projectDir + "Shader/" ).c_str() );

	pCamera = std::make_unique<Do::Camera>();

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

	if ( Donya::Keyboard::State( 'C' ) )
	{
		bool breakPoint{};
	}

#endif // DEBUG_MODE

	if ( GetAsyncKeyState( 'F' ) & 1 )
	{
		isFillDraw = !isFillDraw;
	}

	pCamera->Update();

#if USE_IMGUI

	const char *LoadFileWindowName = "File Information";
	if ( ImGui::BeginIfAllowed( LoadFileWindowName ) )
	{
		if ( ImGui::Button( "Open FBX File" ) )
		{
			OpenCommonDialogAndFile();
		}

		ImGui::Text( "" );

		for ( auto &it = meshes.begin(); it != meshes.end(); )
		{
			std::string nodeCaption ="[" + it->loader.GetFileName() + "]";
			if( ImGui::TreeNode( nodeCaption.c_str() ) )
			{
				if ( ImGui::Button( "Remove" ) )
				{
					it = meshes.erase( it );

					ImGui::TreePop();
					continue;
				}
				// else

				it->loader.EnumPreservingDataToImGui( LoadFileWindowName );
				ImGui::TreePop();
			}

			++it;
		}

		ImGui::End();
	}

#endif // USE_IMGUI
}

void Framework::Render( float elapsedTime/*Elapsed seconds from last frame*/ )
{
#ifdef USE_IMGUI

	ImGui::Render();

#endif

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
	
	// Geometric-Cube, StaticMesh Render
	if ( 1 )
	{
		XMMATRIX matWorld{};
		{
			static float scale = 2.0f;
			if ( GetAsyncKeyState( 'W' ) < 0 ) { scale += 0.01f; }
			if ( GetAsyncKeyState( 'S' ) < 0 ) { scale -= 0.01f; }

			static float temporaryTimer = 0;
			temporaryTimer += 0.1f;
			if ( 360.0f <= temporaryTimer ) { temporaryTimer = 0; }

			static float angleX = 0;
			static float angleY = 0;
			static float angleZ = 0;
			if ( GetAsyncKeyState( VK_UP	) < 0 ) { angleX += 0.12f; }
			if ( GetAsyncKeyState( VK_DOWN	) < 0 ) { angleX -= 0.12f; }
			if ( GetAsyncKeyState( VK_LEFT	) < 0 ) { angleY += 0.12f; }
			if ( GetAsyncKeyState( VK_RIGHT	) < 0 ) { angleY -= 0.12f; }
			if ( GetAsyncKeyState( 'A'		) < 0 ) { angleZ += 0.12f; }
			if ( GetAsyncKeyState( 'D'		) < 0 ) { angleZ -= 0.12f; }

			XMMATRIX scaling		= XMMatrixScaling( scale, scale, scale );
			XMMATRIX rotX			= XMMatrixRotationX( ToRadian( angleX ) );
			XMMATRIX rotY			= XMMatrixRotationY( ToRadian( angleY ) );
			XMMATRIX rotZ			= XMMatrixRotationZ( ToRadian( angleZ ) );
			XMMATRIX rotation		= ( rotZ * rotY ) * rotX;
			XMMATRIX translation	= XMMatrixTranslation( 0.0f, 0.0f, -0.5f );

			matWorld = scaling * rotation * translation;
		}
		XMMATRIX matView = pCamera->CalcViewMatrix();

		XMFLOAT4X4 worldViewProjection{};
		{
			XMMATRIX matProjPerspective{};
			XMMATRIX matProjOrthographic{};
			{
				constexpr float FOV = ToRadian( 30.0f );
				float width		= Common::ScreenWidthF();
				float height	= Common::ScreenHeightF();
				float aspect	= width / height;
				float zNear		= 0.10f;
				float zFar		= 1000.0f;
			#if 1
				XMFLOAT4X4 projection{};
				projection = pCamera->AssignPerspectiveProjection( FOV, aspect, zNear, zFar );
				matProjPerspective = XMLoadFloat4x4( &projection );

				projection = pCamera->AssignOrthographicProjection( 16.0f, 9.0f, zNear, zFar );
				matProjOrthographic = XMLoadFloat4x4( &projection );
			#else
				matProjPerspective = XMMatrixPerspectiveFovLH( FOV, aspect, zNear, zFar );
				matProjOrthographic = XMMatrixOrthographicLH( 16.0f, 9.0f, zNear, zFar );
			#endif
			}

			XMStoreFloat4x4
			(
				&worldViewProjection,
				DirectX::XMMatrixMultiply( matWorld, DirectX::XMMatrixMultiply( matView, matProjOrthographic ) )
			);
		}

		XMFLOAT4X4 world{};
		XMStoreFloat4x4( &world, matWorld );

		XMFLOAT4 lightDirection{ 0.0f, -1.0f, 1.0f, 0.0f };
		XMFLOAT4 materialColor{ 0.7f, 1.0f, 0.9f, 1.0f };
		XMFLOAT4 cameraPos{};
		{
			XMFLOAT3 ref = pCamera->GetPosition();
			cameraPos.x = ref.x;
			cameraPos.y = ref.y;
			cameraPos.z = ref.z;
			cameraPos.w = 1.0f;
		}

		for ( auto &it : meshes )
		{
			if ( it.pMesh )
			{
				it.pMesh->Render( worldViewProjection, world, lightDirection, materialColor, isFillDraw );
			}
		}

		/*
		if ( 0 )
		{
			pGeomtrPrimitive->Render
			(
				worldViewProjection,
				world,
				lightDirection, materialColor,
				isFillDraw
			);
		}
		if ( 1 )
		{
			pStaticMesh->Render
			(
				worldViewProjection,
				world,
				lightDirection, materialColor,
				cameraPos,
				isFillDraw
			);
		}
		*/
	}

#ifdef USE_IMGUI

	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );

#endif

	HRESULT hr = dxgiSwapChain->Present( 0, 0 );
	_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Present()" );
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
	ofn.Flags			= OFN_FILEMUSTEXIST;

	// TODO:Support multiple files.

	auto result = GetOpenFileNameA( &ofn );
	if ( !result ) { return false; }
	// else

	std::string filePath( chosenFilesFullPath );
	std::string errorMessage{};

	meshes.push_back( {} );
	meshes.back().loader.Load( filePath, &errorMessage );
	Donya::SkinnedMesh::Create( &meshes.back().loader, &meshes.back().pMesh );

	return true;
}