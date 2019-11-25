#include "SceneLex.h"

#include <queue>
#include <thread>
#include <vector>

//#include "Donya/Camera.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"				// Use GetFPS().
#include "Donya/GeometricPrimitive.h"	// For debug draw collision.
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Quaternion.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/UseImgui.h"
#include "Donya/Vector.h"

#include "Camera.h"
#include "Common.h"
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
public:
	ICamera							iCamera;
	DirectionalLight				directionalLight;
	Donya::Vector4					mtlColor;

	int								nowPressMouseButton;	// [None:0][Left:VK_LBUTTON][Middle:VK_MBUTTON][Right:VK_RBUTTON]
	Donya::Int2						prevMouse;
	Donya::Int2						currMouse;

	float							cameraVirtualDistance;	// The distance to virtual screen that align to Common::ScreenSize() from camera. Calc when detected a click.
	float							cameraRotateSpeed;
	Donya::Vector3					cameraMoveSpeed;

	std::vector<MeshAndInfo>		models;

	std::unique_ptr<std::thread>	pLoadThread{};
	std::unique_ptr<AsyncLoad>		pCurrentLoading;
	std::string						currentLoadingFileNameUTF8;	// For UI.
	std::queue<std::string>			reservedAbsFilePaths;
	std::queue<std::string>			reservedFileNamesUTF8;		// For UI.

	bool							drawWireFrame;
public:
	Impl() :
		iCamera(), directionalLight(), mtlColor( 1.0f, 1.0f, 1.0f, 1.0f ),
		nowPressMouseButton(), prevMouse(), currMouse(),
		cameraVirtualDistance( 1.0f ), cameraRotateSpeed(), cameraMoveSpeed(),
		models(),
		pLoadThread( nullptr ), pCurrentLoading( nullptr ),
		currentLoadingFileNameUTF8(), reservedAbsFilePaths(), reservedFileNamesUTF8(),
		drawWireFrame( false )
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
		MouseUpdate();

		iCamera.Init( ICamera::Mode::Satellite );
		iCamera.SetZRange( 0.1f, 1000.0f );
		iCamera.SetFOV( ToRadian( 30.0f ) );
		iCamera.SetScreenSize( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
		iCamera.SetPosition( { 0.0f, 0.0f, -64.0f } );
		iCamera.SetFocusPoint( { 0.0f, 0.0f, 0.0f } );
		iCamera.SetProjectionPerspective();

		CalcDistToVirtualScreen();

		constexpr float MOVE_SPEED	= 1.0f;
		constexpr float FRONT_SPEED	= 3.0f;
		constexpr float ROT_SPEED	= ToRadian( 1.0f );

		cameraRotateSpeed = ROT_SPEED;
		cameraMoveSpeed.x = MOVE_SPEED;
		cameraMoveSpeed.y = MOVE_SPEED;
		cameraMoveSpeed.z = FRONT_SPEED;
	}
	void Uninit()
	{
		iCamera.Uninit();
	}

	void Update( float elapsedTime )
	{
		MouseUpdate();

	#if USE_IMGUI

		UseImGui();

	#endif // USE_IMGUI

	#if DEBUG_MODE

		if ( Donya::Keyboard::Press( VK_MENU ) )
		{
			if ( Donya::Keyboard::Trigger( 'C' ) )
			{
				bool breakPoint{};
			}
			if ( Donya::Keyboard::Trigger( 'R' ) )
			{
				iCamera.SetPosition( { 0.0f, 0.0f, -64.0f } );
				iCamera.SetFocusPoint( { 0.0f, 0.0f, 0.0f } );
				iCamera.SetOrientation( { 0.0f, 0.0f, 0.0f, 1.0f } );
			}
			if ( Donya::Keyboard::Trigger( 'T' ) )
			{
				Donya::ToggleShowStateOfImGui();
			}

			// bool isAccept = meshes.empty();
			bool isAccept = true;
			if ( Donya::Keyboard::Press( 'B' ) && Donya::Keyboard::Trigger( 'F' ) && isAccept )
			{
				constexpr const char *BLUE_FALCON = "D:\\D-Download\\ASSET_Models\\Free\\Distribution_FBX\\BLue Falcon\\Blue Falcon.FBX";
				ReserveLoadFileIfLoadable( BLUE_FALCON );
			}

			if ( Donya::Keyboard::Trigger( 'Q' ) && !models.empty() )
			{
				models.pop_back();
			}
		}

	#endif // DEBUG_MODE

		AppendModelIfLoadFinished();

		FetchDraggedFilePaths();
		StartLoadIfVacant();
		
		ShowNowLoadingModels();
		UpdateModels( elapsedTime );
		
		CameraUpdate();
	}

	void Draw( float elapsedTime )
	{
		Donya::Vector4x4 W = Donya::Vector4x4::Identity();
		Donya::Vector4x4 V = iCamera.CalcViewMatrix();
		// Donya::Vector4x4 V = Donya::Vector4x4::Identity();
		Donya::Vector4x4 P = iCamera.GetProjectionMatrix();

		Donya::Vector4x4 WVP = W * V * P;

		Donya::Vector4 cameraPos{ iCamera.GetPosition(), 1.0f };

		for ( auto &it : models )
		{
			it.mesh.Render
			(
				it.motions,
				it.animator,
				WVP, W,
				cameraPos,
				mtlColor,
				directionalLight.color,
				directionalLight.direction,
				( drawWireFrame ) ? false : true
			);
		}

	#if DEBUG_MODE

		{
			auto InitializedCube = []()
			{
				Donya::Geometric::Cube cube{};
				cube.Init();
				return cube;
			};
			static Donya::Geometric::Cube cube = InitializedCube();

			cube.Render( nullptr, true, true, WVP, W, directionalLight.direction, mtlColor );
		}

	#endif // DEBUG_MODE
	}
private:
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
	
		cameraVirtualDistance = cameraScreenSize.y / ( 2.0f * tanf( FOV * 0.5f ) );
	}

	Donya::Vector3 ScreenToWorld( const Donya::Vector2 &screenPos )
	{
		 //see http://marupeke296.com/ALG_No7_MoveCameraWithCursor.html

		const Donya::Vector3	cameraPos		= iCamera.GetPosition();
		const Donya::Vector3	cameraFocus		= iCamera.GetFocusPoint();
		const Donya::Quaternion	cameraPosture	= iCamera.GetOrientation();
	
		Donya::Vector3 wsScreenPos{ screenPos.x, screenPos.y, cameraVirtualDistance };
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

	void CameraUpdate()
	{
		ICamera::Controller controller{};
		controller.SetNoOperation();

		constexpr float SLERP_FACTOR = 0.2f; // TODO : To be changeable this.
		controller.slerpPercent = SLERP_FACTOR;

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
				// TODO : To be changeable this moving direction(normal or inverse).

				moveVelocity.x = csMouseMove.x * cameraMoveSpeed.x;
				moveVelocity.y = csMouseMove.y * cameraMoveSpeed.y;
			}

			moveVelocity.z = scast<float>( Donya::Mouse::WheelRot() ) * cameraMoveSpeed.z;
		}

		float roll{}, pitch{}, yaw{};
		if ( nowPressMouseButton == VK_LBUTTON )
		{
			// TODO : To be changeable this moving direction(normal or inverse).

			yaw   = csMouseMove.x * cameraRotateSpeed;
			pitch = csMouseMove.y * cameraRotateSpeed;
			roll  = 0.0f; // Unused.
		}

		controller.moveVelocity		= moveVelocity;
		controller.roll				= roll;
		controller.pitch			= pitch;
		controller.yaw				= yaw;
		controller.slerpPercent		= SLERP_FACTOR;
		controller.moveInLocalSpace	= true;

		iCamera.Update( controller );
		
		/*
		auto MakeControlStructWithMouse = []()->ICamera::Controller
		{
			if ( !Donya::Keyboard::Press( VK_MENU ) || Donya::IsMouseHoveringImGuiWindow() )
			{
				ICamera::Controller noop{};
				noop.SetNoOperation();
				noop.slerpPercent = 0.2f;
				return noop;
			}
			// else

			static Donya::Int2 prevMouse{};
			static Donya::Int2 currMouse{};

			prevMouse = currMouse;

			auto nowMouse = Donya::Mouse::Coordinate();
			currMouse.x = scast<int>( nowMouse.x );
			currMouse.y = scast<int>( nowMouse.y );

			bool isInputMouseButton = Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) || Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) || Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT );
			bool isDriveMouse = ( prevMouse != currMouse ) || Donya::Mouse::WheelRot() || isInputMouseButton;
			if ( !isDriveMouse )
			{
				ICamera::Controller noop{};
				noop.SetNoOperation();
				noop.slerpPercent = 0.2f;
				return noop;
			}
			// else

			Donya::Vector3 diff{};
			{
				Donya::Vector2 vec2 = ( currMouse - prevMouse ).Float();

				diff.x = vec2.x;
				diff.y = vec2.y;
			}

			Donya::Vector3 movement{};
			Donya::Vector3 rotation{};

			if ( Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) )
			{
				constexpr float ROT_AMOUNT = ToRadian( 1.0f );
				rotation.x = diff.x * ROT_AMOUNT;
				rotation.y = diff.y * ROT_AMOUNT;
			}
			else
			if ( Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) )
			{
				constexpr float MOVE_SPEED = 0.1f;
				movement.x =  diff.x * MOVE_SPEED;
				movement.y = -diff.y * MOVE_SPEED;
			}

			constexpr float FRONT_SPEED = 3.5f;
			movement.z = FRONT_SPEED * scast<float>( Donya::Mouse::WheelRot() );

			Donya::Quaternion rotYaw = Donya::Quaternion::Make( Donya::Vector3::Up(), rotation.x );

			Donya::Vector3 right = Donya::Vector3::Right();
			right = rotYaw.RotateVector( right );
			Donya::Quaternion rotPitch = Donya::Quaternion::Make( right, rotation.y );

			Donya::Quaternion rotQ = rotYaw * rotPitch;

			static Donya::Vector3 front = Donya::Vector3::Front();

			if ( !rotation.IsZero() )
			{
				front = rotQ.RotateVector( front );
				front.Normalize();
			}

			ICamera::Controller ctrl{};
			ctrl.moveVelocity		= movement;
			ctrl.rotation			= {};
			ctrl.slerpPercent		= 0.2f;
			ctrl.moveInLocalSpace	= true;

			return ctrl;
		};
		*/
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

		auto Load = []( std::string filePath, AsyncLoad *pElement )
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
		pLoadThread		= std::make_unique<std::thread>( Load, loadFilePath, pCurrentLoading.get() );
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
			it.animator.Update( elapsedTime );
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
			if ( ImGui::TreeNode( u8"情報" ) )
			{
				ImGui::Text( "FPS[%f]", Donya::GetFPS() );
				ImGui::Text( "" );

				int x{}, y{};
				Donya::Mouse::Coordinate( &x, &y );

				ImGui::Text( u8"マウス位置[X:%d][Y%d]", x, y );
				ImGui::Text( u8"マウスホイール[%d]", Donya::Mouse::WheelRot() );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"環境設定" ) )
			{
				if ( ImGui::TreeNode( u8"カメラ" ) )
				{
					iCamera.ShowImGuiNode();

					if ( ImGui::TreeNode( u8"速さ" ) )
					{
						ImGui::DragFloat3( u8"移動速度", &cameraMoveSpeed.x, 0.2f );
						ImGui::DragFloat ( u8"回転速度", &cameraRotateSpeed, ToRadian( 1.0f ) );

						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
				
				constexpr float DIRECTION_RANGE = 8.0f;
				ImGui::SliderFloat3( u8"方向性ライト・向き",		&directionalLight.direction.x, -DIRECTION_RANGE, DIRECTION_RANGE );
				ImGui::ColorEdit4  ( u8"方向性ライト・カラー",	&directionalLight.color.x );
				ImGui::ColorEdit4  ( u8"マテリアル・カラー",		&mtlColor.x );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"モデル一覧" ) )
			{
				size_t modelCount = models.size();
				ImGui::Text( u8"モデル数:[%d]", modelCount );

				std::string fileNameCaption{};
				for ( auto &it = models.begin(); it != models.end(); )
				{
					fileNameCaption = "[" + it->loader.GetOnlyFileName() + "]";
					if ( ImGui::TreeNode( fileNameCaption.c_str() ) )
					{
						if ( ImGui::Button( u8"取り除く" ) )
						{
							it = models.erase( it );

							ImGui::TreePop();
							continue;
						}
						// else

						if ( ImGui::Button( u8"保存" ) )
						{
							std::string saveName = GetSaveFileNameByCommonDialog();
							if ( saveName.empty() )
							{
								// Error.
								char breakpoint = 0;
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
