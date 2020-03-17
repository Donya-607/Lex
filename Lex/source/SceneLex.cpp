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

#include "Model.h"
#include "ModelCommon.h"
#include "ModelMotion.h"
#include "ModelPose.h"
#include "ModelRenderer.h"

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
	struct Renderer
	{
		std::unique_ptr<Donya::Model::StaticRenderer>	pStatic;
		std::unique_ptr<Donya::Model::SkinningRenderer>	pSkinning;
	};
	struct MeshAndInfo
	{
		struct Model
		{
			std::shared_ptr<Donya::Model::StaticModel>		pStatic;
			std::shared_ptr<Donya::Model::SkinningModel>	pSkinning;
		};

		Donya::Loader				loader{};
		Model						model{};
		Donya::Model::MotionHolder	holder{};
		Donya::Model::Animator		animator{};
		Donya::Model::Pose			pose{};
		Donya::Vector3				scale		{ 1.0f, 1.0f, 1.0f };
		Donya::Vector3				rotation	{ 0.0f, 0.0f, 0.0f };	// Pitch, Yaw, Roll. Degree.
		Donya::Vector3				translation	{ 0.0f, 0.0f, 0.0f };
		std::string	addMotionResultInfo{};
		int			usingMotionIndex{ 0 };
		float		currentElapsedTime{};
		float		motionAccelPercent{ 1.0f };		// Normal is 1.0f.
		bool		dontWannaDraw{ false };
		bool		useSkinningVersion{ true };
		bool		playLoopMotion{ true };

		// Old version. not supporting now.
		/*
		Donya::SkinnedMesh	mesh{};
		Donya::MotionChunk	motions{};
		Donya::Animator		animator{};
		*/
	public:
		bool CreateByLoader()
		{
			bool result{};
			bool succeeded = true;

			const auto modelSource = loader.GetModelSource();

			model.pStatic	= std::make_shared<Donya::Model::StaticModel>  ( Donya::Model::StaticModel::Create  ( modelSource, loader.GetFileDirectory() ) );
			model.pSkinning	= std::make_shared<Donya::Model::SkinningModel>( Donya::Model::SkinningModel::Create( modelSource, loader.GetFileDirectory() ) );
			if ( !model.pStatic->WasInitializeSucceeded()   ) { succeeded = false; }
			if ( !model.pSkinning->WasInitializeSucceeded() ) { succeeded = false; }

			// Assign to holder.
			holder.AppendSource( modelSource );

			animator.ResetTimer();
			
			pose.AssignSkeletal( modelSource.skeletal );

			return succeeded;

			// Old version. not supporting now.
			/*
			result = Donya::SkinnedMesh::Create( loader, &mesh );
			if ( !result ) { succeeded = false; }
			result = Donya::MotionChunk::Create( loader, &motions );
			if ( !result ) { succeeded = false; }
			animator.Init();
			*/
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
	Renderer						renderer;

	GridLine						grid;

	Donya::CBuffer<Donya::Model::Constants::PerModel::Common>	mcbPerModel{};
	Donya::CBuffer<CBufferPerFrame>	cbPerFrame;
	Donya::CBuffer<CBufferPerModel>	cbPerModel;
	Donya::VertexShader				VSSkinnedMesh;
	Donya::PixelShader				PSSkinnedMesh;

	std::unique_ptr<std::thread>	pLoadThread{};
	std::unique_ptr<AsyncLoad>	pCurrentLoading;
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
		renderer(),
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
		bool result;
		result = ShaderInit();
		if ( !result )
		{
			_ASSERT_EXPR( 0, L"Failed : Create some shaders." );
			exit( -1 );
			return;
		}
		// else

		result = RendererInit();
		if ( !result )
		{
			_ASSERT_EXPR( 0, L"Failed : Create some renderers." );
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
		ClearBackGround();

		const Donya::Vector4x4 V   = iCamera.CalcViewMatrix();
		const Donya::Vector4x4 P   = iCamera.GetProjectionMatrix();
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

		Donya::Vector4x4 S{}, R{}, T{}, W{};
		Donya::Vector4x4 WVP = W * V * P;
		PSSkinnedMesh.Deactivate();
		VSSkinnedMesh.Deactivate();

		cbPerFrame.Deactivate();
		cbPerModel.Deactivate();

		for ( const auto &it : models )
		{
			if ( it.dontWannaDraw ) { continue; }
			// else

			DrawModelSource( it );
		}

		DrawOriginCube( V * P );

		// Old version. not supporting now.
		/*
		for ( auto &it : models )
		{
			if ( it.dontWannaDraw ) { continue; }
			// else
			const Donya::Vector3 eulerRadians
			{
				ToRadian( it.rotation.x ),
				ToRadian( it.rotation.y ),
				ToRadian( it.rotation.z ),
			};
			S = Donya::Vector4x4::MakeScaling( it.scale );
			R = Donya::Vector4x4::MakeRotationEuler( eulerRadians );
			T = Donya::Vector4x4::MakeTranslation( it.translation );
			W = S * R * T;
			WVP = W * V * P;
			it.mesh.Render
			(
				it.motions,
				it.animator,
				WVP, W,
				optionPerMesh,
				optionPerSubset,
				/ psSetSamplerSlot     /=  0,
				/ psSetDiffuseMapSlot  /=  0,
				( drawWireFrame ) ? false : true
			);
		}
		*/
	}
private:
	void ClearBackGround() const
	{
		const FLOAT colors[4]{ bgColor.x, bgColor.y, bgColor.z, bgColor.w };
		Donya::ClearViews( colors );
	}

	void DrawModelSource( const MeshAndInfo &data )
	{
		const Donya::Vector4x4 V = iCamera.CalcViewMatrix();
		const Donya::Vector4x4 P = iCamera.GetProjectionMatrix();
		const Donya::Vector4   cameraPos{ iCamera.GetPosition(), 1.0f };

		// Activate CB per frame.
		{
			Donya::Model::Constants::PerScene::Common constantsScene{};
			constantsScene.directionalLight.direction	= directionalLight.direction;
			constantsScene.directionalLight.color		= directionalLight.color;
			constantsScene.eyePosition					= cameraPos;
			constantsScene.viewProjMatrix				= V * P;
			
			const Donya::Vector3 eulerRadians
			{
				ToRadian( data.rotation.x ),
				ToRadian( data.rotation.y ),
				ToRadian( data.rotation.z ),
			};
			Donya::Model::Constants::PerModel::Common constantsModel{};
			constantsModel.drawColor = Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
			constantsModel.worldMatrix =
				Donya::Vector4x4::MakeScaling( data.scale ) *
				Donya::Vector4x4::MakeRotationEuler( eulerRadians ) *
				Donya::Vector4x4::MakeTranslation( data.translation );

			Donya::Model::Renderer::Default::UpdateCBufferScene( constantsScene );
			Donya::Model::Renderer::Default::UpdateCBufferModel( constantsModel );
			Donya::Model::Renderer::Default::ActivateCBufferScene();
			Donya::Model::Renderer::Default::ActivateCBufferModel();
		}
		// Activate rendering states.
		{
			Donya::Model::Renderer::Default::ActivateDepthStencil();
			Donya::Model::Renderer::Default::ActivateRasterizer();
			Donya::Model::Renderer::Default::ActivateSampler();
		}
		// Activate shaders.
		if ( data.useSkinningVersion )
		{
			Donya::Model::Renderer::Default::ActivateVertexShaderSkinning();
			Donya::Model::Renderer::Default::ActivatePixelShaderSkinning();
		}
		else
		{
			Donya::Model::Renderer::Default::ActivateVertexShaderStatic();
			Donya::Model::Renderer::Default::ActivatePixelShaderStatic();
		}

		const auto descPerMesh		= Donya::Model::Renderer::Default::DescCBufferPerMesh();
		const auto descPerSubset	= Donya::Model::Renderer::Default::DescCBufferPerSubset();
		const auto descDiffuse		= Donya::Model::Renderer::Default::DescDiffuseMap();
		if ( data.useSkinningVersion )
		{
			renderer.pSkinning->Render
			(
				*data.model.pSkinning,
				data.pose,
				descPerMesh,
				descPerSubset,
				descDiffuse
			);
		}
		else
		{
			renderer.pStatic->Render
			(
				*data.model.pStatic,
				data.pose,
				descPerMesh,
				descPerSubset,
				descDiffuse
			);
		}

		// Deactivate shaders.
		if ( data.useSkinningVersion )
		{
			Donya::Model::Renderer::Default::DeactivateVertexShaderSkinning();
			Donya::Model::Renderer::Default::DeactivatePixelShaderSkinning();
		}
		else
		{
			Donya::Model::Renderer::Default::DeactivateVertexShaderStatic();
			Donya::Model::Renderer::Default::DeactivatePixelShaderStatic();
		}
		// Deactivate rendering states.
		{
			Donya::Model::Renderer::Default::DeactivateDepthStencil();
			Donya::Model::Renderer::Default::DeactivateRasterizer();
			Donya::Model::Renderer::Default::DeactivateSampler();
		}
		// Deactivate CB.
		{
			Donya::Model::Renderer::Default::DeactivateCBufferModel();
			Donya::Model::Renderer::Default::DeactivateCBufferScene();
		}
	}

	void DrawOriginCube(  const Donya::Vector4x4 &matVP ) const
	{
		if ( !drawOriginCube ) { return; }
		// else
		
		// Show a cube to origin with unit scale.

		static Donya::Geometric::Cube cube = Donya::Geometric::CreateCube();

		constexpr Donya::Vector4 COLOR{ 0.8f, 1.0f, 0.9f, 0.6f };
		constexpr Donya::Vector4x4 W = Donya::Vector4x4::Identity();
		cube.Render( nullptr, true, true, W * matVP, W, directionalLight.direction, COLOR );
	}
private:
	bool ShaderInit()
	{
		bool succeeded = true;
		bool result{};

		result = mcbPerModel.Create();
		if ( !result ) { succeeded = false; }
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
	bool RendererInit()
	{
		renderer.pStatic	= std::make_unique<Donya::Model::StaticRenderer>();
		renderer.pSkinning	= std::make_unique<Donya::Model::SkinningRenderer>();
		if ( !renderer.pStatic   ) { return false; }
		if ( !renderer.pSkinning ) { return false; }
		// else
		return true;
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
			( LOOK_POINT - DEFAULT_POS ).Unit(),
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
		auto  CanLoadFile = []( std::string filePath )->bool
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
		if ( fileName.empty() ) // If the extract function failed.
		{
			reservedFileNamesUTF8.push( Donya::MultiToUTF8( filePath ) );
		}
		else
		{
			reservedFileNamesUTF8.push( Donya::MultiToUTF8( fileName ) );
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
			ImGui::Text( "Reserving load file list : %d", fileListUTF8.size() + 1/*Current file*/ );

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
			it.currentElapsedTime	= it.animator.GetInternalElapsedTime();

			const auto useMotion	= it.holder.GetMotion( it.usingMotionIndex );
			const auto currentPose	= it.animator.CalcCurrentPose( useMotion );
			if ( it.pose.HasCompatibleWith( currentPose ) )
			{
				it.pose.AssignSkeletal( currentPose );
				it.pose.UpdateTransformMatrices();
			}
		}
	}

	std::string GetOpenFileNameByCommonDialog()
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
		if ( !result ) { return std::string{}; }
		// else

		std::string filePath{ ofn.lpstrFile };
		return filePath;
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
				constexpr float DIRECTION_RANGE = 8.0f;
				ImGui::SliderFloat3( u8"方向性ライト・向き",		&directionalLight.direction.x, -DIRECTION_RANGE, DIRECTION_RANGE );
				ImGui::ColorEdit4  ( u8"方向性ライト・カラー",	&directionalLight.color.x );
				ImGui::ColorEdit4  ( u8"マテリアル・カラー",		&materialColor.x );
				ImGui::ColorEdit4  ( u8"背景・カラー",			&bgColor.x );
				ImGui::Checkbox( u8"原点に単位立方体を表示する",	&drawOriginCube );
				ImGui::Text( "" );

				ImGui::SliderFloat( u8"ロード時：サンプルＦＰＳ", &loadSamplingFPS, 0.0f, 120.0f );
				ImGui::Text( "" );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"カメラ設定" ) )
			{
				iCamera.ShowImGuiNode();

				if ( ImGui::TreeNode( u8"操作方法" ) )
				{
					constexpr std::array<const char *, 5> CAPTION
					{
						u8"カメラ操作はすべて，ＡＬＴキーを押しながらになります。",
						u8"マウスホイール　　　　：ズーム（ドリー）イン・アウト",
						u8"左クリック　　　＋移動：回転移動",
						u8"ホイール押し込み＋移動：平行移動",
						u8"Ｒキー　　　　　　　　：位置のリセット",
					};
					for ( const auto &it : CAPTION )
					{
						ImGui::Text( it );
					}

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( u8"設定" ) )
				{
					ImGui::DragFloat  ( u8"Near",		&cameraOp.zNear, 0.01f, 0.0f );
					ImGui::DragFloat  ( u8"Far",		&cameraOp.zFar,  1.00f, 0.0f );
					ImGui::SliderFloat( u8"視野角",		&cameraOp.FOV, 0.0f, ToRadian( 360.0f ) );
					iCamera.SetZRange( cameraOp.zNear, cameraOp.zFar );
					iCamera.SetFOV( cameraOp.FOV );
					iCamera.SetProjectionPerspective();

					ImGui::SliderFloat( u8"補間係数",	&cameraOp.slerpFactor, 0.0f, 1.0f );
					ImGui::DragFloat3 ( u8"移動速度",	&cameraOp.moveSpeed.x, 0.2f );
					ImGui::DragFloat  ( u8"回転速度",	&cameraOp.rotateSpeed, ToRadian( 1.0f ) );
					ImGui::Checkbox( u8"反転・横移動",	&cameraOp.reverseMoveHorizontal   );
					ImGui::Checkbox( u8"反転・縦移動",	&cameraOp.reverseMoveVertical     );
					ImGui::Checkbox( u8"反転・横回転",	&cameraOp.reverseRotateHorizontal );
					ImGui::Checkbox( u8"反転・縦回転",	&cameraOp.reverseRotateVertical   );

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
				
			ShowModelNode( u8"モデル一覧" );

			ImGui::End();
		}

	#endif // USE_IMGUI
	}
#if USE_IMGUI
	void ShowModelNode( const std::string &caption )
	{
		if ( !ImGui::TreeNode( caption.c_str() ) ) { return; }
		// else

		ImGui::Text( u8"モデル数：[%d]", models.size() );

		int  uniqueIndex		= 0;
		auto MakeCaption		= [&uniqueIndex]( const Donya::Loader &source )
		{
			std::string fileName = "[" + source.GetOnlyFileName() + "]";
			std::string postfix = "##" + std::to_string( uniqueIndex++ );
			return fileName + postfix;
		};

		auto ShowDrawConfig		= [&]( MeshAndInfo &target )
		{
			if ( !ImGui::TreeNode( u8"描画設定" ) ) { return; }
			// else
			
			ImGui::Checkbox( u8"隠す", &target.dontWannaDraw );

			ImGui::DragFloat3( u8"スケール", &target.scale.x, 0.1f );
			ImGui::DragFloat3( u8"回転量（Degree）", &target.rotation.x, 1.0f );
			ImGui::DragFloat3( u8"平行移動", &target.translation.x, 1.0f );

			ImGui::TreePop();
		};
		auto ShowMotionConfig	= [&]( MeshAndInfo &target )
		{
			if ( !ImGui::TreeNode( u8"モーション設定" ) ) { return; }
			// else
			
			ImGui::Checkbox( u8"スキン版を使用する", &target.useSkinningVersion );
			ImGui::Text( "" );

			ImGui::DragFloat( u8"モーション再生速度（倍率）",	&target.motionAccelPercent, 0.01f, 0.0f );
			ImGui::DragFloat( u8"モーションの経過時間",		&target.currentElapsedTime, 0.01f, 0.0f );
			ImGui::Checkbox ( u8"モーションをループ再生する",	&target.playLoopMotion );
			target.animator.SetInternalElapsedTime( target.currentElapsedTime );
			( target.playLoopMotion )
			? target.animator.EnableWrapAround()
			: target.animator.DisableWrapAround();

			ImGui::Text( u8"登録されているモーションの数：%d", target.holder.GetMotionCount() );

			if ( ImGui::TreeNode( u8"モーションを追加する" ) )
			{
				ImGui::Text( u8"他のロード済みモデルからの追加になります" );

				auto ShowPart = [&]( const MeshAndInfo &source )
				{
					if ( &target == &source ) { return; } // Except the myself.
					if ( !ImGui::TreeNode( MakeCaption( source.loader ).c_str() ) ) { return; }
					// else

					const auto modelSource = source.loader.GetModelSource();
					ImGui::Text( u8"モーションの数：[%d]", modelSource.motions.size() );

					const std::string prompt{ u8"このモーション群を追加する" };

					if ( modelSource.motions.empty() )
					{
						ImGui::TextDisabled( prompt.c_str() );
						ImGui::Text( "モーションを持っていません" );
					}
					else if ( ImGui::Button( prompt.c_str() ) )
					{
						target.holder.AppendSource( modelSource );
					}

					ImGui::TreePop();
				};

				for ( const auto &itr : models )
				{
					ShowPart( itr );
				}

				if ( !target.addMotionResultInfo.empty() )
				{
					ImGui::Text( target.addMotionResultInfo.c_str() );
					if ( ImGui::Button( u8"モーション追加の結果情報を削除" ) )
					{
						target.addMotionResultInfo = "";
					}
				}

				ImGui::TreePop();
			}

			const size_t motionCount = target.holder.GetMotionCount();
			if ( 2 <= motionCount )
			{
				ImGui::SliderInt( u8"使用するモーション番号", &target.usingMotionIndex, 0, motionCount - 1 );
				target.usingMotionIndex = std::max( 0, std::min( scast<int>( motionCount ) - 1, target.usingMotionIndex ) );
			}
			else
			{
				ImGui::Text( u8"使用するモーション番号：０" );
				target.usingMotionIndex = 0;
			}

			ImGui::TreePop();
		};
		auto ShowLoaderConfig	= [&]( MeshAndInfo &target )
		{
			if ( !ImGui::TreeNode( u8"詳細" ) ) { return; }
			// else

			target.loader.AdjustParameterByImGuiNode();
			target.loader.EnumPreservingDataToImGui();

			ImGui::TreePop();
		};

		for ( auto &it = models.begin(); it != models.end(); )
		{
			if ( !ImGui::TreeNode( MakeCaption( it->loader ).c_str() ) )
			{
				++it;
				continue;
			}
			// else

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

			ShowDrawConfig( *it );
			ShowMotionConfig( *it );
			ShowLoaderConfig( *it );

			ImGui::TreePop();
			++it;
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI
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
