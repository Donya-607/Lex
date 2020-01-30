#include "SceneLex.h"

#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Donya/CBuffer.h"
#include "Donya/Color.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"				// Use GetFPS().
#include "Donya/GeometricPrimitive.h"	// For debug draw collision.
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Quaternion.h"
#include "Donya/Sprite.h"
#include "Donya/Shader.h"
#include "Donya/Useful.h"
#include "Donya/UseImgui.h"
#include "Donya/Vector.h"

#include "Camera.h"
#include "Common.h"
#include "Grid.h"
#include "Loader.h"
#include "Motion.h"
#include "SkinnedMesh.h"

#pragma comment( lib, "comdlg32.lib" ) // For common-dialog.

using namespace DirectX;

struct SceneLex::Impl
{
public:
	static constexpr unsigned int MAX_PATH_COUNT = MAX_PATH;
public:
	struct DirectionalLight
	{
		Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };		// RGBA.
		Donya::Vector4 direction{ 0.0f, 0.0f, 1.0f, 0.0f };
	};
	struct MeshAndInfo
	{
		Donya::Loader		loader{};
		Donya::SkinnedMesh	mesh{};
		Donya::MotionChunk	motions{};
		Donya::Animator		animator{};
		float	currentElapsedTime{};
		float	motionAccelPercent{ 1.0f };		// Normal is 1.0f.
		bool	enableMotionInterpolation{ false };
		bool	dontWannaDraw{ false };
	public:
		bool CreateByLoader()
		{
			bool result{};
			bool succeeded = true;

			result = Donya::SkinnedMesh::Create( loader, &mesh );
			if ( !result ) { succeeded = false; }

			result = Donya::MotionChunk::Create( loader, &motions );
			if ( !result ) { succeeded = false; }

			animator.Init();

			return succeeded;
		}
	};
	struct AsyncLoad
	{
		std::mutex	meshMutex{};
		std::mutex	flagMutex{};
		MeshAndInfo	meshInfo{};
		bool		isFinished{};	// Is finished the loading process ?
		bool		isSucceeded{};	// Is Succeeded the loading process ?
	};
	struct CameraUsage
	{
		float			zNear{ 0.1f };
		float			zFar { 1000.0f };
		float			FOV{ ToRadian( 30.0f ) };
		float			slerpFactor{ 0.1f };				// 0.0f ~ 1.0f.
		float			virtualDistance{ 1.0f };			// The distance to virtual screen that align to Common::ScreenSize() from camera. Calc when detected a click.
		float			rotateSpeed{};
		Donya::Vector3	moveSpeed{};
		bool			reverseMoveHorizontal{};
		bool			reverseMoveVertical{};
		bool			reverseRotateHorizontal{};
		bool			reverseRotateVertical{};
	};
	struct CBufferPerFrame
	{
		DirectX::XMFLOAT4 eyePosition;
		DirectX::XMFLOAT4 dirLightColor;
		DirectX::XMFLOAT4 dirLightDirection;
	};
	struct CBufferPerModel
	{
		DirectX::XMFLOAT4 materialColor;
	};
public:

	// TODO : To be serialize these member.

	ICamera							iCamera;
	DirectionalLight				directionalLight;
	Donya::Vector4					materialColor;
	Donya::Vector4					bgColor;

	int								nowPressMouseButton;	// [None:0][Left:VK_LBUTTON][Middle:VK_MBUTTON][Right:VK_RBUTTON]
	Donya::Int2						prevMouse;
	Donya::Int2						currMouse;

	CameraUsage						cameraOp;

	float							loadSamplingFPS;		// Use to Loader's sampling FPS.
	std::vector<MeshAndInfo>		models;

	GridLine						grid;

	Donya::CBuffer<CBufferPerFrame>	cbPerFrame;
	Donya::CBuffer<CBufferPerModel>	cbPerModel;
	Donya::VertexShader				VSSkinnedMesh;
	Donya::PixelShader				PSSkinnedMesh;

	std::unique_ptr<std::thread>	pLoadThread{};
	std::unique_ptr<AsyncLoad>		pCurrentLoading;
	std::string						currentLoadingFileNameUTF8;	// For UI.
	std::queue<std::string>			reservedAbsFilePaths;
	std::queue<std::string>			reservedFileNamesUTF8;		// For UI.

	bool							drawWireFrame;
	bool							drawOriginCube;
public:
	Impl() :
		iCamera(), directionalLight(),
		materialColor( 1.0f, 1.0f, 1.0f, 1.0f ), bgColor( 0.5f, 0.5f, 0.5f, 1.0f ),
		nowPressMouseButton(), prevMouse(), currMouse(),
		cameraOp(),
		loadSamplingFPS( 0.0f ), models(),
		grid(),
		cbPerFrame(), cbPerModel(), VSSkinnedMesh(), PSSkinnedMesh(),
		pLoadThread( nullptr ), pCurrentLoading( nullptr ),
		currentLoadingFileNameUTF8(), reservedAbsFilePaths(), reservedFileNamesUTF8(),
		drawWireFrame( false ), drawOriginCube( true )
	{}
	~Impl()
	{
		models.clear();
		models.shrink_to_fit();

		if ( pLoadThread && pLoadThread->joinable() )
		{
			pLoadThread->join();
		}
	}
public:
	void Init()
	{
		bool result = ShaderInit();
		if ( !result )
		{
			_ASSERT_EXPR( 0, L"Failed : Create some shaders." );
			exit( -1 );
			return;
		}
		// else

		CameraInit();

		MouseUpdate();

		grid.Init();
	}
	void Uninit()
	{
		iCamera.Uninit();

		grid.Uninit();
	}

	void Update( float elapsedTime )
	{
		MouseUpdate();

	#if USE_IMGUI

		UseImGui();

	#endif // USE_IMGUI

		if ( Donya::Keyboard::Press( VK_MENU ) )
		{
			if ( Donya::Keyboard::Trigger( 'R' ) )
			{
				SetDefaultCameraPosition();
			}

	#if DEBUG_MODE
			if ( Donya::Keyboard::Trigger( 'C' ) )
			{
				bool breakPoint{};
			}
			if ( Donya::Keyboard::Trigger( 'T' ) )
			{
				Donya::ToggleShowStateOfImGui();
			}

			// bool isAccept = meshes.empty();
			bool isAccept = true;
			if ( Donya::Keyboard::Press( 'B' ) && Donya::Keyboard::Trigger( 'F' ) && isAccept )
			{	// For hands-free access.
				constexpr const char *BLUE_FALCON = "D:\\D-Download\\ASSET_Models\\Free\\Distribution_FBX\\BLue Falcon\\Blue Falcon.FBX";
				ReserveLoadFileIfLoadable( BLUE_FALCON );
			}

			if ( Donya::Keyboard::Trigger( 'Q' ) && !models.empty() )
			{
				models.pop_back();
			}
	#endif // DEBUG_MODE

		}

		AppendModelIfLoadFinished();

		FetchDraggedFilePaths();
		StartLoadIfVacant();
		
		ShowNowLoadingModels();
		UpdateModels( elapsedTime );
		
		CameraUpdate();

		grid.Update();
	}

	void Draw( float elapsedTime )
	{
		{
			const FLOAT colors[4]{ bgColor.x, bgColor.y, bgColor.z, bgColor.w };
			Donya::ClearViews( colors );
		}

		const Donya::Vector4x4 W   = Donya::Vector4x4::Identity();
		const Donya::Vector4x4 V   = iCamera.CalcViewMatrix();
		const Donya::Vector4x4 P   = iCamera.GetProjectionMatrix();
		const Donya::Vector4x4 WVP = W * V * P;
		const Donya::Vector4   cameraPos{ iCamera.GetPosition(), 1.0f };

		grid.Draw( V * P );
		
		cbPerFrame.data.eyePosition			= cameraPos.XMFloat();
		cbPerFrame.data.dirLightColor		= directionalLight.color;
		cbPerFrame.data.dirLightDirection	= directionalLight.direction;
		cbPerFrame.Activate( 0, /* setVS = */ true, /* setPS = */ true );

		cbPerModel.data.materialColor		= materialColor;
		cbPerModel.data.materialColor.w		= Donya::Color::FilteringAlpha( materialColor.w );
		cbPerModel.Activate( 1, /* setVS = */ true, /* setPS = */ true );

		VSSkinnedMesh.Activate();
		PSSkinnedMesh.Activate();

		Donya::SkinnedMesh::CBSetOption optionPerMesh{};
		Donya::SkinnedMesh::CBSetOption optionPerSubset{};
		optionPerMesh.setSlot		= 2;
		optionPerMesh.setVS			= true;
		optionPerMesh.setPS			= true;
		optionPerSubset.setSlot		= 3;
		optionPerSubset.setVS		= true;
		optionPerSubset.setPS		= true;

		for ( auto &it : models )
		{
			if ( it.dontWannaDraw ) { continue; }
			// else

			it.mesh.Render
			(
				it.motions,
				it.animator,
				WVP, W,
				optionPerMesh,
				optionPerSubset,
				/* psSetSamplerSlot    = */ 0,
				/* psSetDiffuseMapSlot = */ 0,
				( drawWireFrame ) ? false : true
			);
		}

		PSSkinnedMesh.Deactivate();
		VSSkinnedMesh.Deactivate();

		cbPerFrame.Deactivate();
		cbPerModel.Deactivate();

		// Show a cube to origin with unit scale.
		if ( drawOriginCube )
		{
			static Donya::Geometric::Cube cube = Donya::Geometric::CreateCube();

			constexpr Donya::Vector4 COLOR{ 0.8f, 1.0f, 0.9f, 0.6f };
			cube.Render( nullptr, true, true, WVP, W, directionalLight.direction, COLOR );
		}
	}
private:
	bool ShaderInit()
	{
		bool succeeded = true;
		bool result{};

		result = cbPerFrame.Create();
		if ( !result ) { succeeded = false; }
		result = cbPerModel.Create();
		if ( !result ) { succeeded = false; }

		constexpr const char *VSFilePath = "./Data/Shader/SkinnedMeshVS.cso";
		constexpr const char *PSFilePath = "./Data/Shader/SkinnedMeshPS.cso";
		const std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc
		{
			{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BONES"		, 0, DXGI_FORMAT_R32G32B32A32_UINT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "WEIGHTS"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		result = VSSkinnedMesh.CreateByCSO( VSFilePath, inputElementDesc );
		if ( !result ) { succeeded = false; }

		result = PSSkinnedMesh.CreateByCSO( PSFilePath );
		if ( !result ) { succeeded = false; }

		return succeeded;
	}

	void MouseUpdate()
	{
		POINT pMouse = Donya::Mouse::Coordinate();
		prevMouse    = currMouse;
		currMouse.x  = scast<int>( pMouse.x );
		currMouse.y  = scast<int>( pMouse.y );

		{
			// If the mouse movement is big, I regard to "the mouse was looped" that, then discard the difference.

			Donya::Vector2 diff = ( currMouse - prevMouse ).Float();
			if ( Common::ScreenWidthF()  * 0.8f < fabsf( diff.x ) )
			{
				prevMouse.x = currMouse.x;
			}
			if ( Common::ScreenHeightF() * 0.8f < fabsf( diff.y ) )
			{
				prevMouse.y = currMouse.y;
			}
		}

		// HACK : This algorithm is not beautiful... :(
		bool isInputMouseButton = Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) || Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) || Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT );
		if ( isInputMouseButton )
		{
			if ( !nowPressMouseButton )
			{
				if ( Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) )
				{
					nowPressMouseButton = VK_LBUTTON;
				}
				else
				if ( Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) )
				{
					nowPressMouseButton = VK_MBUTTON;
				}
				else
				if ( Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT ) )
				{
					nowPressMouseButton = VK_RBUTTON;
				}
			}
		}
		else
		{
			nowPressMouseButton = NULL;
		}
	}
	void CalcDistToVirtualScreen()
	{
		// see http://marupeke296.com/ALG_No7_MoveCameraWithCursor.html
	
		const float FOV = iCamera.GetFOV();
		const Donya::Vector2 cameraScreenSize = iCamera.GetScreenSize();
	
		cameraOp.virtualDistance = cameraScreenSize.y / ( 2.0f * tanf( FOV * 0.5f ) );
	}

	Donya::Vector3 ScreenToWorld( const Donya::Vector2 &screenPos )
	{
		 //see http://marupeke296.com/ALG_No7_MoveCameraWithCursor.html

		const Donya::Vector3	cameraPos		= iCamera.GetPosition();
		const Donya::Vector3	cameraFocus		= iCamera.GetFocusPoint();
		const Donya::Quaternion	cameraPosture	= iCamera.GetOrientation();
	
		Donya::Vector3 wsScreenPos{ screenPos.x, screenPos.y, cameraOp.virtualDistance };
		wsScreenPos = cameraPosture.RotateVector( wsScreenPos );
	
		float rayLength{}; // This is the "a" of reference site.
		{
			Donya::Vector3 anyPosition	= Donya::Vector3::Zero(); // The position on plane of world space.
			Donya::Vector3 virNormal	= cameraPos - cameraFocus;
			float dotSample = Donya::Vector3::Dot( { anyPosition - cameraPos }, virNormal );
			float dotTarget = Donya::Vector3::Dot( { wsScreenPos - cameraPos }, virNormal );
	
			rayLength = dotSample / dotTarget;
		}
	
		const Donya::Vector3 vCameraToScreen = wsScreenPos - cameraPos;
		const Donya::Vector3 onPlanePos = cameraPos + ( vCameraToScreen * rayLength );
		return onPlanePos;
	}

	void CameraInit()
	{
		constexpr float DEFAULT_NEAR	= 0.1f;
		constexpr float DEFAULT_FAR		= 1000.0f;
		constexpr float DEFAULT_FOV		= ToRadian( 30.0f );
		constexpr float MOVE_SPEED		= 1.0f;
		constexpr float FRONT_SPEED		= 3.0f;
		constexpr float ROT_SPEED		= ToRadian( 1.0f );

		cameraOp.zNear			= DEFAULT_NEAR;
		cameraOp.zFar			= DEFAULT_FAR;
		cameraOp.FOV			= DEFAULT_FOV;
		cameraOp.rotateSpeed	= ROT_SPEED;
		cameraOp.moveSpeed.x	= MOVE_SPEED;
		cameraOp.moveSpeed.y	= MOVE_SPEED;
		cameraOp.moveSpeed.z	= FRONT_SPEED;

		// My preference.
		cameraOp.reverseRotateHorizontal = true;

		iCamera.Init( ICamera::Mode::Satellite );
		iCamera.SetZRange( cameraOp.zNear, cameraOp.zFar );
		iCamera.SetFOV( cameraOp.FOV );
		iCamera.SetScreenSize( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
		SetDefaultCameraPosition();
		iCamera.SetProjectionPerspective();

		CalcDistToVirtualScreen();
	}
	void CameraUpdate()
	{
		ICamera::Controller controller{};
		controller.SetNoOperation();

		controller.slerpPercent = cameraOp.slerpFactor;

		bool isDriveMouse		= ( currMouse != prevMouse ) || Donya::Mouse::WheelRot() || nowPressMouseButton;
		bool isAllowDrive		= Donya::Keyboard::Press( VK_MENU ) && !Donya::IsMouseHoveringImGuiWindow();
		if ( !isAllowDrive || !isDriveMouse )
		{
			iCamera.Update( controller );
			return;
		}
		// else

		Donya::Vector3 wsMouseMove{}; // World space.
		Donya::Vector3 csMouseMove{}; // Camera space.
		{
			Donya::Vector2 old = prevMouse.Float();
			Donya::Vector2 now = currMouse.Float();

			// If you want move to right, the camera must move to left.
			old.x *= -1.0f;
			now.x *= -1.0f;

			const Donya::Vector3 wsOld = ScreenToWorld( old );
			const Donya::Vector3 wsNow = ScreenToWorld( now );

			wsMouseMove = wsNow - wsOld;

			Donya::Quaternion invCameraRotation = iCamera.GetOrientation().Conjugate();
			csMouseMove = invCameraRotation.RotateVector( wsMouseMove );
		}

		Donya::Vector3 moveVelocity{};
		{
			if ( nowPressMouseButton == VK_MBUTTON )
			{
				moveVelocity.x = csMouseMove.x * cameraOp.moveSpeed.x;
				moveVelocity.y = csMouseMove.y * cameraOp.moveSpeed.y;

				if ( cameraOp.reverseMoveHorizontal ) { moveVelocity.x *= -1.0f; }
				if ( cameraOp.reverseMoveVertical   ) { moveVelocity.y *= -1.0f; }
			}

			moveVelocity.z = scast<float>( Donya::Mouse::WheelRot() ) * cameraOp.moveSpeed.z;
		}

		float roll{}, pitch{}, yaw{};
		if ( nowPressMouseButton == VK_LBUTTON )
		{
			yaw   = csMouseMove.x * cameraOp.rotateSpeed;
			pitch = csMouseMove.y * cameraOp.rotateSpeed;
			roll  = 0.0f; // Unused.

			if ( cameraOp.reverseRotateHorizontal ) { yaw   *= -1.0f; }
			if ( cameraOp.reverseRotateVertical   ) { pitch *= -1.0f; }
		}

		controller.moveVelocity		= moveVelocity;
		controller.roll				= roll;
		controller.pitch			= pitch;
		controller.yaw				= yaw;
		controller.moveInLocalSpace	= true;

		iCamera.Update( controller );
	}
	void SetDefaultCameraPosition()
	{
		constexpr Donya::Vector3 DEFAULT_POS = { 32.0f, 32.0f, -32.0f };
		constexpr Donya::Vector3 LOOK_POINT  = {  0.0f,  0.0f,   0.0f };
		iCamera.SetPosition   ( DEFAULT_POS	);
		iCamera.SetFocusPoint ( LOOK_POINT	);

		// The orientation is set by two step, this prevent a roll-rotation(Z-axis) in local space of camera.
		
		Donya::Quaternion lookAt  = Donya::Quaternion::LookAt
		(
			Donya::Quaternion::Identity(),
			( LOOK_POINT - DEFAULT_POS ).Normalized(),
			Donya::Quaternion::Freeze::Up
		);
		lookAt.RotateBy
		(
			Donya::Quaternion::Make
			(
				lookAt.LocalRight(),
				atanf( DEFAULT_POS.y / ( -DEFAULT_POS.z + EPSILON ) )
			)
		);

		iCamera.SetOrientation( lookAt		);
	}

	void FetchDraggedFilePaths()
	{
		std::vector<std::string> filePathStorage = Donya::FetchDraggedFilePaths();
		for ( const auto &it : filePathStorage )
		{
			ReserveLoadFileIfLoadable( it );
		}
	}
	void ReserveLoadFileIfLoadable( std::string filePath )
	{
		auto CanLoadFile = []( std::string filePath )->bool
		{
			constexpr std::array<const char *, 5> EXTENSIONS
			{
				".obj", ".OBJ",
				".fbx", ".FBX",
				".bin"
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

		if ( !CanLoadFile( filePath ) ) { return; }
		// else

		reservedAbsFilePaths.push( filePath );

		const std::string fileName = Donya::ExtractFileNameFromFullPath( filePath );
		if ( !fileName.empty() )
		{
			reservedFileNamesUTF8.push( Donya::MultiToUTF8( fileName ) );
		}
		else // If the extract function failed.
		{
			reservedFileNamesUTF8.push( Donya::MultiToUTF8( filePath ) );
		}
	}

	void StartLoadIfVacant()
	{
		if ( pCurrentLoading ) { return; }
		if ( reservedAbsFilePaths.empty() ) { return; }
		// else

		auto Load = []( std::string filePath, AsyncLoad *pElement, float samplingFPS )
		{
			if ( !pElement ) { return; }
			// else

			HRESULT hr = S_OK;
			hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
			if ( FAILED( hr ) )
			{
				std::lock_guard<std::mutex> lock( pElement->flagMutex );

				pElement->isFinished  = true;
				pElement->isSucceeded = false;
				return;
			}
			// else

			bool createResult = true; // Will be change by below process, if load succeeded.

			// Load model, using lock_guard by pLoadMutex.
			{
				Donya::Loader tmpHeavyLoad{}; // For reduce time of lock.
				tmpHeavyLoad.SetSamplingFPS( samplingFPS );
				bool loadSucceeded = tmpHeavyLoad.Load( filePath, nullptr );

				std::lock_guard<std::mutex> lock( pElement->meshMutex );

				// bool loadSucceeded  = pElement->meshInfo.loader.Load( fullPath, nullptr );
				pElement->meshInfo.loader = tmpHeavyLoad; // Takes only assignment-time.
				if ( loadSucceeded )
				{
					createResult = pElement->meshInfo.CreateByLoader();
				}
			}

			std::lock_guard<std::mutex> lock( pElement->flagMutex );

			pElement->isFinished  = true;
			pElement->isSucceeded = createResult;

			CoUninitialize();
		};

		const std::string loadFilePath = reservedAbsFilePaths.front();
		reservedAbsFilePaths.pop();
		reservedFileNamesUTF8.pop();

		currentLoadingFileNameUTF8 = Donya::ExtractFileNameFromFullPath( loadFilePath );

		pCurrentLoading	= std::make_unique<AsyncLoad>();
		pLoadThread		= std::make_unique<std::thread>( Load, loadFilePath, pCurrentLoading.get(), loadSamplingFPS );
	}
	void AppendModelIfLoadFinished()
	{
		if ( !pCurrentLoading ) { return; }
		// else

		// Set std::lock_guard's scope.
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

				models.emplace_back( pCurrentLoading->meshInfo );
			}
		}

		pCurrentLoading.reset( nullptr );
	}

	void ShowNowLoadingModels()
	{
	#if USE_IMGUI

		if ( !pCurrentLoading ) { return; }
		// else

		constexpr Donya::Vector2 WINDOW_POS {  32.0f, 632.0f };
		constexpr Donya::Vector2 WINDOW_SIZE{ 360.0f, 180.0f };

		SetNextImGuiWindow( WINDOW_POS, WINDOW_SIZE );

		if ( ImGui::BeginIfAllowed( "Loading Files" ) )
		{
			std::queue<std::string> fileListUTF8 = reservedFileNamesUTF8;
			ImGui::Text( "Reserving load file list : %d", fileListUTF8.size() + 1 );

			ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), ImVec2( 0, 0 ) );

			std::string fileNameUTF8 = currentLoadingFileNameUTF8;
			ImGui::Text( "Now:[%s]", fileNameUTF8.c_str() );
			while ( !fileListUTF8.empty() )
			{
				fileNameUTF8 = fileListUTF8.front();
				ImGui::Text( "[%s]", fileNameUTF8.c_str() );
				fileListUTF8.pop();
			}

			ImGui::EndChild();

			ImGui::End();
		}

	#endif // USE_IMGUI
	}
	void UpdateModels( float elapsedTime )
	{
		for ( auto &it : models )
		{
			it.animator.Update( elapsedTime * it.motionAccelPercent );
			it.currentElapsedTime = it.animator.GetCurrentElapsedTime();
		}
	}

	bool OpenCommonDialogAndFile()
	{
		char chosenFilesFullPath[MAX_PATH_COUNT] = { 0 };
		char chosenFileName[MAX_PATH_COUNT] = { 0 };

		OPENFILENAMEA ofn{ 0 };
		ofn.lStructSize		= sizeof( decltype( ofn ) );
		ofn.hwndOwner		= Donya::GetHWnd();
		ofn.lpstrFilter		= "FBX-file(*.fbx)\0*.fbx\0"
							  "OBJ-file(*.obj)\0*.obj\0"
							  "Binary-file(*.bin)\0*.bin\0"
							  "\0";
		ofn.lpstrFile		= chosenFilesFullPath;
		ofn.nMaxFile		= MAX_PATH_COUNT;
		ofn.lpstrFileTitle	= chosenFileName;
		ofn.nMaxFileTitle	= MAX_PATH_COUNT;
		ofn.Flags			= OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR; // If not set OFN_NOCHANGEDIR flag, the current directory will be changed, so the SkinnedMesh can't use current directory.

		// TODO:Support multiple files.

		auto  result = GetOpenFileNameA( &ofn );
		if ( !result ) { return false; }
		// else

		std::string filePath{ chosenFilesFullPath };

		ReserveLoadFileIfLoadable( filePath );

		return true;
	}
	std::string GetSaveFileNameByCommonDialog()
	{
		char fileNameBuffer[MAX_PATH_COUNT] = { 0 };
		char titleBuffer[MAX_PATH_COUNT] = { 0 };

		OPENFILENAMEA ofn{ 0 };
		ofn.lStructSize		= sizeof( decltype( ofn ) );
		ofn.hwndOwner		= Donya::GetHWnd();
		ofn.lpstrFilter		= "Binary-file(*.bin)\0*.bin\0"
							  "\0";
		ofn.lpstrFile		= fileNameBuffer;
		ofn.nMaxFile		= MAX_PATH_COUNT;
		ofn.lpstrFileTitle	= titleBuffer;
		ofn.nMaxFileTitle	= MAX_PATH_COUNT;
		ofn.Flags			= OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR; // If not set OFN_NOCHANGEDIR flag, the current directory will be changed, so the SkinnedMesh can't use current directory.

		auto result = GetSaveFileNameA( &ofn );
		if ( !result ) { return std::string{}; }
		// else

		std::string filePath{ ofn.lpstrFile };
		return filePath;
	}

	/// <summary>
	/// If set to { 0, 0 }, that parameter will be ignored.
	/// </summary>
	void SetNextImGuiWindow( const Donya::Vector2 &pos = { 0.0f, 0.0f }, const Donya::Vector2 &size = { 0.0f, 0.0f } )
	{
	#if USE_IMGUI

		auto Convert = []( const Donya::Vector2 &vec )
		{
			return ImVec2{ vec.x, vec.y };
		};

		if ( !pos.IsZero()  ) { ImGui::SetNextWindowPos ( Convert( pos  ), ImGuiCond_Once ); }
		if ( !size.IsZero() ) { ImGui::SetNextWindowSize( Convert( size ), ImGuiCond_Once ); }

	#endif // USE_IMGUI
	}

	void UseImGui()
	{
	#if USE_IMGUI

		constexpr Donya::Vector2 WINDOW_POS {  32.0f,  32.0f };
		constexpr Donya::Vector2 WINDOW_SIZE{ 720.0f, 600.0f };
		SetNextImGuiWindow( WINDOW_POS, WINDOW_SIZE );

		if ( ImGui::BeginIfAllowed() )
		{
			if ( ImGui::TreeNode( u8"���" ) )
			{
				ImGui::Text( "FPS[%f]", Donya::GetFPS() );
				ImGui::Text( "" );

				int x{}, y{};
				Donya::Mouse::Coordinate( &x, &y );

				ImGui::Text( u8"�}�E�X�ʒu[X:%d][Y%d]", x, y );
				ImGui::Text( u8"�}�E�X�z�C�[��[%d]", Donya::Mouse::WheelRot() );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"���ݒ�" ) )
			{
				constexpr float DIRECTION_RANGE = 8.0f;
				ImGui::SliderFloat3( u8"���������C�g�E����",		&directionalLight.direction.x, -DIRECTION_RANGE, DIRECTION_RANGE );
				ImGui::ColorEdit4  ( u8"���������C�g�E�J���[",	&directionalLight.color.x );
				ImGui::ColorEdit4  ( u8"�}�e���A���E�J���[",		&materialColor.x );
				ImGui::ColorEdit4  ( u8"�w�i�E�J���[",			&bgColor.x );
				ImGui::Checkbox( u8"���_�ɒP�ʗ����̂�\������",	&drawOriginCube );
				ImGui::Text( "" );

				ImGui::SliderFloat( u8"���[�h���F�T���v���e�o�r", &loadSamplingFPS, 0.0f, 120.0f );
				ImGui::Text( "" );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"�J�����ݒ�" ) )
			{
				iCamera.ShowImGuiNode();

				if ( ImGui::TreeNode( u8"������@" ) )
				{
					constexpr std::array<const char *, 5> CAPTION
					{
						u8"�J��������͂��ׂāC�`�k�s�L�[�������Ȃ���ɂȂ�܂��B",
						u8"�}�E�X�z�C�[���@�@�@�@�F�Y�[���i�h���[�j�C���E�A�E�g",
						u8"���N���b�N�@�@�@�{�ړ��F��]�ړ�",
						u8"�z�C�[���������݁{�ړ��F���s�ړ�",
						u8"�q�L�[�@�@�@�@�@�@�@�@�F�ʒu�̃��Z�b�g",
					};
					for ( const auto &it : CAPTION )
					{
						ImGui::Text( it );
					}

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( u8"�ݒ�" ) )
				{
					ImGui::DragFloat  ( u8"Near",		&cameraOp.zNear, 0.01f, 0.0f );
					ImGui::DragFloat  ( u8"Far",		&cameraOp.zFar,  1.00f, 0.0f );
					ImGui::SliderFloat( u8"����p",		&cameraOp.FOV, 0.0f, ToRadian( 360.0f ) );
					iCamera.SetZRange( cameraOp.zNear, cameraOp.zFar );
					iCamera.SetFOV( cameraOp.FOV );
					iCamera.SetProjectionPerspective();

					ImGui::SliderFloat( u8"��ԌW��",	&cameraOp.slerpFactor, 0.0f, 1.0f );
					ImGui::DragFloat3 ( u8"�ړ����x",	&cameraOp.moveSpeed.x, 0.2f );
					ImGui::DragFloat  ( u8"��]���x",	&cameraOp.rotateSpeed, ToRadian( 1.0f ) );
					ImGui::Checkbox( u8"���]�E���ړ�",	&cameraOp.reverseMoveHorizontal   );
					ImGui::Checkbox( u8"���]�E�c�ړ�",	&cameraOp.reverseMoveVertical     );
					ImGui::Checkbox( u8"���]�E����]",	&cameraOp.reverseRotateHorizontal );
					ImGui::Checkbox( u8"���]�E�c��]",	&cameraOp.reverseRotateVertical   );

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
				
			if ( ImGui::TreeNode( u8"���f���ꗗ" ) )
			{
				size_t modelCount = models.size();
				ImGui::Text( u8"���f����:[%d]", modelCount );

				std::string fileNameCaption{};
				std::string uniquePostfix{};	// Prevent matching a caption at TreeNode().
				int uniqueIndex = 0;
				for ( auto &it = models.begin(); it != models.end(); )
				{
					fileNameCaption = "[" + it->loader.GetOnlyFileName() + "]";
					uniquePostfix = "##" + std::to_string( uniqueIndex++ );
					if ( ImGui::TreeNode( ( fileNameCaption + uniquePostfix ).c_str() ) )
					{
						if ( ImGui::Button( u8"��菜��" ) )
						{
							it = models.erase( it );

							ImGui::TreePop();
							continue;
						}
						// else
						if ( ImGui::Button( u8"�ۑ�" ) )
						{
							std::string saveName = GetSaveFileNameByCommonDialog();
							if ( saveName.empty() )
							{
								// Cancel the save process.
							}
							else
							{
								if ( saveName.find( ".bin" ) == std::string::npos )
								{
									saveName += ".bin";
								}
								it->loader.SaveByCereal( saveName );
							}
						}
						ImGui::Text( "" );

						if ( ImGui::TreeNode( u8"�`��ݒ�" ) )
						{
							ImGui::Text( "�ꎞ�I�Ȑݒ�ɂȂ�܂�\n" );

							ImGui::Checkbox( u8"�B��", &it->dontWannaDraw );
							ImGui::Text( "" );

							ImGui::DragFloat( u8"���[�V�����Đ����x�i�{���j",	&it->motionAccelPercent, 0.01f, 0.0f );
							ImGui::DragFloat( u8"���[�V�����̃t���[��",		&it->currentElapsedTime, 0.01f, 0.0f );
							it->animator.SetCurrentElapsedTime( it->currentElapsedTime );
							ImGui::Checkbox( u8"���[�V�����̕�Ԃ�L���ɂ���", &it->enableMotionInterpolation );
							it->animator.SetInterpolateFlag( it->enableMotionInterpolation );

							ImGui::TreePop();
						}
						ImGui::Text( "" );

						it->loader.AdjustParameterByImGuiNode();
						it->loader.EnumPreservingDataToImGui();
						ImGui::TreePop();
					}

					++it;
				}

				ImGui::TreePop();
			}

			ImGui::End();
		}

	#endif // USE_IMGUI
	}
};

SceneLex::SceneLex() : pImpl( std::make_unique<Impl>() )
{}
SceneLex::~SceneLex()
{
	pImpl.reset( nullptr );
}

void SceneLex::Init()
{
	pImpl->Init();
}
void SceneLex::Uninit()
{
	pImpl->Uninit();
}

Scene::Result SceneLex::Update( float elapsedTime )
{
	pImpl->Update( elapsedTime );

	return ReturnResult();
}

void SceneLex::Draw( float elapsedTime )
{
	pImpl->Draw( elapsedTime );
}

Scene::Result SceneLex::ReturnResult()
{
	/*
	if ( Donya::Keyboard::Press( VK_RSHIFT ) && Donya::Keyboard::Trigger( VK_RETURN ) )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Lex;
		return change;
	}
	// else
	*/

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}
