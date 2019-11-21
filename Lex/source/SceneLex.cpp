#include "SceneLex.h"

#include <queue>
#include <thread>
#include <vector>

//#include "Donya/Camera.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"		// Use GetFPS().
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
	Camera							camera;
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
		camera(), directionalLight(), mtlColor( 1.0f, 1.0f, 1.0f, 1.0f ),
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
		camera.SetToHomePosition
		(
			{ 16.0f, 16.0f, -16.0f },
			{ 0.0f, 0.0f, 0.0f }
		);
		camera.SetPerspectiveProjectionMatrix
		(
			Common::ScreenWidthF() / Common::ScreenHeightF()
		);
	}
	void Uninit()
	{
		
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


		CameraUpdate();
	}

	void Draw( float elapsedTime )
	{
		Donya::Vector4x4 W = Donya::Vector4x4::Identity();
		Donya::Vector4x4 V = camera.CalcViewMatrix();
		Donya::Vector4x4 P = camera.GetProjectionMatrix();

		Donya::Vector4x4 WVP = W * V * P;

		Donya::Vector4 cameraPos{ camera.GetPos(), 1.0f };

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
	}
private:
	void CameraUpdate()
	{
		auto MakeControlStructWithMouse = []()
		{
			if ( !Donya::Keyboard::Press( VK_MENU ) )
			{
				Donya::Camera::Controller noop{};
				noop.SetNoOperation();
				return noop;
			}

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
				Donya::Camera::Controller noop{};
				noop.SetNoOperation();
				return noop;
			}

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
				movement.x = diff.x * MOVE_SPEED;
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

			Donya::Camera::Controller ctrl{};
			ctrl.moveVelocity = movement;
			ctrl.rotation = rotation;
			ctrl.slerpPercent = 1.0f;
			ctrl.moveAtLocalSpace = true;

			return ctrl;
		};
		pCamera->Update( MakeControlStructWithMouse() );
		if ( Donya::Keyboard::Trigger( 'R' ) )
		{
			pCamera->SetPosition( { 0.0f, 360.0f, -512.0f } );
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

	void UseImGui()
	{
	#if USE_IMGUI

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
				camera.ShowImGuiNode();
				
				constexpr float DIRECTION_RANGE = 8.0f;
				ImGui::SliderFloat3( u8"���������C�g�E����",		&directionalLight.direction.x, -DIRECTION_RANGE, DIRECTION_RANGE );
				ImGui::ColorEdit4  ( u8"���������C�g�E�J���[",	&directionalLight.color.x );
				ImGui::ColorEdit4  ( u8"�}�e���A���E�J���[",		&mtlColor.x );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"���f���ꗗ" ) )
			{
				size_t modelCount = models.size();
				ImGui::Text( u8"���f����:[%d]", modelCount );

				std::string fileNameCaption{};
				for ( auto &it = models.begin(); it != models.end(); )
				{
					fileNameCaption = "[" + it->loader.GetOnlyFileName() + "]";
					if ( ImGui::TreeNode( fileNameCaption.c_str() ) )
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