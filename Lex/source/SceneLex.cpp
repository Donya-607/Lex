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
#include "SKinnedMesh.h"

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
		iCamera.Init( ICamera::Mode::Satellite );
		iCamera.SetZRange( 0.1f, 1000.0f );
		iCamera.SetFOV( ToRadian( 30.0f ) );
		iCamera.SetScreenSize( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
		iCamera.SetPosition( { 16.0f, 16.0f, -16.0f } );
		iCamera.SetFocusPoint( { 0.0f, 0.0f, 0.0f } );
		iCamera.SetProjectionPerspective();
	}
	void Uninit()
	{
		iCamera.Uninit();
	}

	void Update( float elapsedTime )
	{
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
				WVP, W,
				cameraPos,
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

			cube.Render( nullptr, true, true, WVP, W, directionalLight.direction, directionalLight.color );
		}

	#endif // DEBUG_MODE
	}
private:
	void CameraUpdate()
	{
		auto MakeControlStructWithMouse = []()->ICamera::Controller
		{
			if ( !Donya::Keyboard::Press( VK_MENU ) )
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
		iCamera.Update( MakeControlStructWithMouse() );
		if ( Donya::Keyboard::Trigger( 'R' ) )
		{
			iCamera.SetPosition( { 16.0f, 16.0f, -16.0f } );
		}
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

			pElement->isFinished = true;
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
