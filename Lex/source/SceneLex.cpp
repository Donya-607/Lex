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
#include "Donya/RenderingStates.h"
#include "Donya/Sprite.h"
#include "Donya/Shader.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"
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
#include "ModelPrimitive.h"
#include "ModelRenderer.h"

#pragma comment( lib, "comdlg32.lib" ) // For common-dialog.

using namespace DirectX;

namespace
{
	static constexpr D3D11_DEPTH_STENCIL_DESC	DepthStencilDesc()
	{
		D3D11_DEPTH_STENCIL_DESC standard{};
		standard.DepthEnable		= TRUE;
		standard.DepthWriteMask		= D3D11_DEPTH_WRITE_MASK_ALL;
		standard.DepthFunc			= D3D11_COMPARISON_LESS;
		standard.StencilEnable		= FALSE;
		return standard;
	}
	static constexpr D3D11_RASTERIZER_DESC		RasterizerDesc()
	{
		D3D11_RASTERIZER_DESC standard{};
		standard.FillMode				= D3D11_FILL_SOLID;
		standard.CullMode				= D3D11_CULL_BACK;
		standard.FrontCounterClockwise	= TRUE;
		standard.DepthBias				= 0;
		standard.DepthBiasClamp			= 0;
		standard.SlopeScaledDepthBias	= 0;
		standard.DepthClipEnable		= TRUE;
		standard.ScissorEnable			= FALSE;
		standard.MultisampleEnable		= FALSE;
		standard.AntialiasedLineEnable	= TRUE;
		return standard;
	}
	static constexpr D3D11_SAMPLER_DESC			SamplerDesc()
{
	D3D11_SAMPLER_DESC standard{};
	/*
	standard.MipLODBias		= 0;
	standard.MaxAnisotropy	= 16;
	*/
	standard.Filter				= D3D11_FILTER_ANISOTROPIC;
	standard.AddressU			= D3D11_TEXTURE_ADDRESS_WRAP;
	standard.AddressV			= D3D11_TEXTURE_ADDRESS_WRAP;
	standard.AddressW			= D3D11_TEXTURE_ADDRESS_WRAP;
	standard.ComparisonFunc		= D3D11_COMPARISON_ALWAYS;
	standard.MinLOD				= 0;
	standard.MaxLOD				= D3D11_FLOAT32_MAX;
	return standard;
}
}

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
	struct CBuffer
	{
		Donya::CBuffer<Donya::Model::Constants::PerScene::Common> perScene;
		Donya::CBuffer<Donya::Model::Constants::PerModel::Common> perModel;
	};
	struct Shader
	{
		Donya::VertexShader VS;
		Donya::PixelShader  PS;
	};
	struct Renderer
	{
		std::unique_ptr<Donya::Model::StaticRenderer>	pStatic;
		std::unique_ptr<Donya::Model::SkinningRenderer>	pSkinning;
		std::unique_ptr<Donya::Model::CubeRenderer>		pCube;
		std::unique_ptr<Donya::Model::SphereRenderer>	pSphere;
	};
	struct Primitive
	{
		Donya::Model::Cube		cube;
		Donya::Model::Sphere	sphere;
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
		Donya::Model::Source		source{};
		Donya::Model::MotionHolder	holder{};
		Donya::Model::Animator		animator{};
		Donya::Model::Pose			pose{};
		Donya::Model::PolygonGroup	polyGroup{};
		Donya::Vector3				scale		{ 1.0f, 1.0f, 1.0f };
		Donya::Vector3				rotation	{ 0.0f, 0.0f, 0.0f };	// Pitch, Yaw, Roll. Degree.
		Donya::Vector3				translation	{ 0.0f, 0.0f, 0.0f };
		int			usingMotionIndex{ 0 };
		float		currentElapsedTime{};
		float		motionAccelPercent{ 1.0f };	// Magnification.
		bool		dontWannaDraw{ false };
		bool		useSkinningVersion{ true };
		bool		playLoopMotion{ true };
	public:
		bool CreateByLoader()
		{
			bool result{};
			bool succeeded = true;

			source = loader.GetModelSource();

			model.pStatic	= std::make_shared<Donya::Model::StaticModel>  ( Donya::Model::StaticModel::Create  ( source, loader.GetFileDirectory() ) );
			model.pSkinning	= std::make_shared<Donya::Model::SkinningModel>( Donya::Model::SkinningModel::Create( source, loader.GetFileDirectory() ) );
			if ( !model.pStatic->WasInitializeSucceeded()   ) { succeeded = false; }
			if ( !model.pSkinning->WasInitializeSucceeded() ) { succeeded = false; }

			// Assign to holder.
			holder.AppendSource( source );

			animator.ResetTimer();
			
			pose.AssignSkeletal( source.skeletal );

			polyGroup = loader.GetPolygonGroup();

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
	ICamera							iCamera{};
	DirectionalLight				directionalLight{};
	Donya::Vector4					materialColor{ 1.0f, 1.0f, 1.0f, 1.0f };
	Donya::Vector4					bgColor{ 0.5f, 0.5f, 0.5f, 1.0f };

	int								nowPressMouseButton{};	// [None:0][Left:VK_LBUTTON][Middle:VK_MBUTTON][Right:VK_RBUTTON]
	Donya::Int2						prevMouse{};
	Donya::Int2						currMouse{};

	CameraUsage						cameraOp{};

	int								idDSState	= 0;
	int								idRSState	= 0;
	int								idPSSampler	= 0;

	float							loadSamplingFPS = 0.0f;		// Use to Loader's sampling FPS.
	std::vector<MeshAndInfo>		models{};
	Renderer						renderer{};

	std::unique_ptr<Primitive>		pPrimitive{ nullptr };

	GridLine						grid{};

	CBuffer							cbuffer{};
	Shader							shaderStatic{};
	Shader							shaderSkinning{};

	std::unique_ptr<std::thread>	pLoadThread{ nullptr };
	std::unique_ptr<AsyncLoad>		pCurrentLoading{ nullptr };
	std::string						currentLoadingFileNameUTF8{};	// For UI.
	std::queue<std::string>			reservedAbsFilePaths{};
	std::queue<std::string>			reservedFileNamesUTF8{};		// For UI.

	bool							drawWireFrame{ false };
	bool							drawOriginCube{ true };
	bool							useRaycast{ false };
public:
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

		result = RenderingStatusInit();
		if ( !result )
		{
			_ASSERT_EXPR( 0, L"Failed : Create some rendering states." );
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

		result = PrimitiveInit();
		if ( !result )
		{
			_ASSERT_EXPR( 0, L"Failed : Create some primitives." );
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
		
		const Donya::Vector4x4 VP = iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix();
		const Donya::Vector4   cameraPos{ iCamera.GetPosition(), 1.0f };

		grid.Draw( VP );
		
		DrawModels( cameraPos, VP );
		
		DrawOriginCube( VP );
		
		DrawRaycast( VP );
	}
private:
	void ClearBackGround() const
	{
		const FLOAT colors[4]{ bgColor.x, bgColor.y, bgColor.z, bgColor.w };
		Donya::ClearViews( colors );
	}

	void DrawLine( const Donya::Vector3 &from, const Donya::Vector3 &to, const Donya::Vector4x4 &VP, const Donya::Vector4 &color ) const
	{
		auto CreateLine = []( size_t instanceCount )
		{
			Donya::Geometric::Line instance{ instanceCount };
			instance.Init();
			return std::move( instance );
		};
		static Donya::Geometric::Line line = CreateLine( 4U );

		line.Reserve( from, to, color );
		line.Flush( VP );
	}
	void DrawSphere( const Donya::Vector4x4 &W, const Donya::Vector4x4 &VP, const Donya::Vector4 &color, float lightBias = 0.5f ) const
	{
		Donya::Model::Sphere::Constant constant{};
		constant.matWorld		= W;
		constant.matViewProj	= VP;
		constant.drawColor		= color;
		constant.lightDirection = directionalLight.direction.XYZ();
		constant.lightBias		= lightBias;

		renderer.pSphere->ActivateVertexShader();
		renderer.pSphere->ActivatePixelShader();
		renderer.pSphere->ActivateDepthStencil();
		renderer.pSphere->ActivateRasterizer();

		renderer.pSphere->UpdateConstant( constant );
		renderer.pSphere->ActivateConstant();

		renderer.pSphere->Draw( pPrimitive->sphere );

		renderer.pSphere->DeactivateConstant();

		renderer.pSphere->DeactivateRasterizer();
		renderer.pSphere->DeactivateDepthStencil();
		renderer.pSphere->DeactivatePixelShader();
		renderer.pSphere->DeactivateVertexShader();

		// static Donya::Geometric::Sphere sphere = Donya::Geometric::CreateSphere();
		// sphere.Render( nullptr, true, true, W * VP, W, directionalLight.direction, color );
	}
	void DrawCube( const Donya::Vector4x4 &W, const Donya::Vector4x4 &VP, const Donya::Vector4 &color, float lightBias = 0.5f ) const
	{
		Donya::Model::Cube::Constant constant{};
		constant.matWorld		= W;
		constant.matViewProj	= VP;
		constant.drawColor		= color;
		constant.lightDirection = directionalLight.direction.XYZ();
		constant.lightBias		= lightBias;

		renderer.pCube->ActivateVertexShader();
		renderer.pCube->ActivatePixelShader();
		renderer.pCube->ActivateDepthStencil();
		renderer.pCube->ActivateRasterizer();

		renderer.pCube->UpdateConstant( constant );
		renderer.pCube->ActivateConstant();

		renderer.pCube->Draw( pPrimitive->cube );

		renderer.pCube->DeactivateConstant();

		renderer.pCube->DeactivateRasterizer();
		renderer.pCube->DeactivateDepthStencil();
		renderer.pCube->DeactivatePixelShader();
		renderer.pCube->DeactivateVertexShader();

		// static Donya::Geometric::Cube cube = Donya::Geometric::CreateCube();
		// cube.Render( nullptr, true, true, W *VP, W, directionalLight.direction, color );
	}

	void DrawModels( const Donya::Vector4 &cameraPos, const Donya::Vector4x4 &VP )
	{
		if ( models.empty() ) { return; }
		// else

		using namespace Donya::Model;

		Donya::DepthStencil::Activate( idDSState );
		Donya::Rasterizer::Activate( idRSState );
		Donya::Sampler::Activate( idPSSampler, 0, /* setVSS = */ false, /* setPS = */ true );

		cbuffer.perScene.data.directionalLight.color		= directionalLight.color;
		cbuffer.perScene.data.directionalLight.direction	= directionalLight.direction;
		cbuffer.perScene.data.eyePosition					= cameraPos;
		cbuffer.perScene.data.viewProjMatrix				= VP;
		cbuffer.perScene.Activate( 0, /* setVS = */ true, /* setPS = */ true );

		cbuffer.perModel.data.drawColor		= materialColor;
		cbuffer.perModel.data.drawColor.w	= Donya::Color::FilteringAlpha( materialColor.w );

		auto ActivateShader		= [&]( bool skinningVer )
		{
			if ( skinningVer )
			{
				shaderSkinning.VS.Activate();
				shaderSkinning.PS.Activate();
			}
			else
			{
				shaderStatic.VS.Activate();
				shaderStatic.PS.Activate();
			}
		};
		auto DeactivateShader	= [&]( bool skinningVer )
		{
			if ( skinningVer )
			{
				shaderSkinning.VS.Deactivate();
				shaderSkinning.PS.Deactivate();
			}
			else
			{
				shaderStatic.VS.Deactivate();
				shaderStatic.PS.Deactivate();
			}
		};

		auto Render				= [&]( const MeshAndInfo &target, const RegisterDesc &descPerMesh, const RegisterDesc &descPerSubset, const RegisterDesc &descDiffuseMap, bool skinningVer )
		{
			if ( skinningVer )
			{
				renderer.pSkinning->Render
				(
					*target.model.pSkinning,
					target.pose,
					descPerMesh,
					descPerSubset,
					descDiffuseMap
				);
			}
			else
			{
				renderer.pStatic->Render
				(
					*target.model.pStatic,
					target.pose,
					descPerMesh,
					descPerSubset,
					descDiffuseMap
				);
			}
		};

		auto MakeQuaternion		= []( const MeshAndInfo &source )
		{
			return Donya::Quaternion::Make
			(
				ToRadian( source.rotation.x ),
				ToRadian( source.rotation.y ),
				ToRadian( source.rotation.z )
			);
		};

		const auto descMesh		= RegisterDesc::Make( 2, /* setVS = */ true, /* setPS = */ false );
		const auto descSubset	= RegisterDesc::Make( 3, /* setVS = */ false, /* setPS = */ true );
		const auto descDiffuse	= RegisterDesc::Make( 0, /* setVS = */ false, /* setPS = */ true );

		Donya::Vector4x4 W;
		for ( const auto &it : models )
		{
			if ( it.dontWannaDraw ) { continue; }
			// else

			W = Donya::Vector4x4::MakeTransformation
			(
				it.scale,
				MakeQuaternion( it ),
				it.translation
			);
			cbuffer.perModel.data.worldMatrix	= W;
			cbuffer.perModel.Activate( 1, /* setVS = */ true, /* setPS = */ true );

			ActivateShader( it.useSkinningVersion );

			Render( it, descMesh, descSubset, descDiffuse, it.useSkinningVersion );

			DeactivateShader( it.useSkinningVersion );

			cbuffer.perModel.Deactivate();
		}

		cbuffer.perScene.Deactivate();

		Donya::Sampler::Deactivate();
		Donya::Rasterizer::Deactivate();
		Donya::DepthStencil::Deactivate();
	}
	void DrawModelByDefault( const MeshAndInfo &data )
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

	void DrawRaycast( const Donya::Vector4x4 &VP )
	{
		if ( !useRaycast || models.empty() ) { return; }
		// else

		constexpr float				SPHERE_SCALE = 16.0f;
		constexpr Donya::Vector4	LINE_COLOR	{ 0.0f, 1.0f, 0.0f, 0.8f };
		constexpr Donya::Vector4	NORMAL_COLOR{ 0.0f, 0.0f, 1.0f, 0.8f };
		constexpr Donya::Vector4	HIT_COLOR	{ 0.5f, 1.0f, 0.5f, 0.8f };
		const Donya::Int2    ssCenter{ Common::HalfScreenWidth(), Common::HalfScreenHeight() };

		const Donya::Vector3 drawOrigin	= CalcWorldMousePos( ssCenter,  0.1f );
		const Donya::Vector3 rayStart	= CalcWorldMousePos( currMouse, 0.0f );
		const Donya::Vector3 rayEnd		= CalcWorldMousePos( currMouse, 1.0f );

		const auto result = CalcRaycastPos( models.front(), rayStart, rayEnd );
		if ( !result.wasHit )
		{
			DrawLine( drawOrigin, rayEnd, VP, LINE_COLOR );
			return;
		}
		// else

		{
			const Donya::Vector3 &lineStart	= result.intersection;
			const Donya::Vector3 lineEnd	= lineStart + ( result.nearestPolygon.normal * SPHERE_SCALE * 2.0f );
			DrawLine( lineStart, lineEnd, VP, NORMAL_COLOR );
		}

		const Donya::Vector3 &intersection = result.intersection;
		Donya::Vector4x4 W{};
		W._11 = SPHERE_SCALE;
		W._22 = SPHERE_SCALE;
		W._33 = SPHERE_SCALE;
		W._41 = intersection.x;
		W._42 = intersection.y;
		W._43 = intersection.z;
		DrawSphere( W, VP, HIT_COLOR );
		DrawLine( drawOrigin, intersection, VP, HIT_COLOR );
	}

	void DrawOriginCube(  const Donya::Vector4x4 &matVP ) const
	{
		if ( !drawOriginCube ) { return; }
		// else
		
		// Show a cube to origin with unit scale.

		constexpr Donya::Vector4 COLOR{ 0.8f, 1.0f, 0.8f, 0.6f };
		constexpr Donya::Vector4x4 W = Donya::Vector4x4::Identity();
		DrawCube( W, matVP, COLOR );
	}
private:
	bool ShaderInit()
	{
		bool succeeded = true;
		bool result{};

		result = cbuffer.perScene.Create();
		if ( !result ) { succeeded = false; }
		result = cbuffer.perModel.Create();
		if ( !result ) { succeeded = false; }

		constexpr const char *VSFilePathStatic		= "./Data/Shader/ModelStaticVS.cso";
		constexpr const char *VSFilePathSkinning	= "./Data/Shader/ModelSkinningVS.cso";
		constexpr const char *PSFilePath			= "./Data/Shader/ModelPS.cso";
		constexpr auto IEDescsPos	= Donya::Model::Vertex::Pos::GenerateInputElements( 0 );
		constexpr auto IEDescsTex	= Donya::Model::Vertex::Tex::GenerateInputElements( 1 );
		constexpr auto IEDescsBone	= Donya::Model::Vertex::Bone::GenerateInputElements( 2 );

		auto Append = []( auto &dest, const auto &source )
		{
			dest.insert( dest.end(), source.begin(), source.end() );
		};

		std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsStatic{};
		Append( IEDescsStatic, IEDescsPos );
		Append( IEDescsStatic, IEDescsTex );
		std::vector<D3D11_INPUT_ELEMENT_DESC> IEDescsSkinning{ IEDescsStatic };
		Append( IEDescsSkinning, IEDescsBone );
		
		result = shaderStatic.VS.CreateByCSO( VSFilePathStatic, IEDescsStatic );
		if ( !result ) { succeeded = false; }
		result = shaderSkinning.VS.CreateByCSO( VSFilePathSkinning, IEDescsSkinning );
		if ( !result ) { succeeded = false; }
		result = shaderStatic.PS.CreateByCSO( PSFilePath );
		if ( !result ) { succeeded = false; }
		result = shaderSkinning.PS.CreateByCSO( PSFilePath );
		if ( !result ) { succeeded = false; }

		return succeeded;
	}
	bool RenderingStatusInit()
	{
		using FindFunction = std::function<bool( int )>;

		auto  AssignStateIdentifier = []( int *pIdentifier, const FindFunction &IsAlreadyExists )
		{
			*pIdentifier = 0;

			for ( int i = 0; i < INT_MAX; ++i )
			{
				if ( IsAlreadyExists( i ) ) { continue; }
				// else

				*pIdentifier = i;
				return;
			}
		};
		
		using Bundle = std::pair<int *, FindFunction>;
		constexpr  size_t  STATE_COUNT = 3;
		std::array<Bundle, STATE_COUNT> bundles
		{
			std::make_pair( &idDSState,		Donya::DepthStencil::IsAlreadyExists	),
			std::make_pair( &idRSState,		Donya::Rasterizer::IsAlreadyExists		),
			std::make_pair( &idPSSampler,	Donya::Sampler::IsAlreadyExists			),
		};
		for ( size_t i = 0; i < STATE_COUNT; ++i )
		{
			AssignStateIdentifier
			(
				bundles[i].first,
				bundles[i].second
			);
		}

		auto  CreateDepthStencil	= []( int id )
		{
			return Donya::DepthStencil::CreateState( id, DepthStencilDesc() );
		};
		auto  CreateRasterizer		= []( int id )
		{
			return Donya::Rasterizer::CreateState( id, RasterizerDesc() );
		};
		auto  CreateSampler			= []( int id )
		{
			return Donya::Sampler::CreateState( id, SamplerDesc() );
		};

		bool result		= true;
		bool succeeded	= true;
		result = CreateDepthStencil( idDSState );
		if ( !result ) { succeeded = false; }
		result = CreateRasterizer( idRSState );
		if ( !result ) { succeeded = false; }
		result = CreateSampler( idPSSampler );
		if ( !result ) { succeeded = false; }

		return succeeded;
	}
	bool RendererInit()
	{
		renderer.pStatic	= std::make_unique<Donya::Model::StaticRenderer>();
		renderer.pSkinning	= std::make_unique<Donya::Model::SkinningRenderer>();
		renderer.pCube		= std::make_unique<Donya::Model::CubeRenderer>();
		renderer.pSphere	= std::make_unique<Donya::Model::SphereRenderer>();
		if ( !renderer.pStatic			) { return false; }
		if ( !renderer.pSkinning		) { return false; }
		if ( !renderer.pCube			) { return false; }
		if ( !renderer.pSphere			) { return false; }
		if ( !renderer.pCube->Create()	) { return false; }
		if ( !renderer.pSphere->Create()) { return false; }
		// else
		return true;
	}
	bool PrimitiveInit()
	{
		pPrimitive = std::make_unique<Primitive>();
		if ( !pPrimitive ) { return false; }
		// else

		bool succeeded = true;
		if ( !pPrimitive->cube.Create() ) { succeeded = false; }
		if ( !pPrimitive->sphere.Create() ) { succeeded = false; }

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
	Donya::Vector4x4 CalcScreenToWorldMatrix()
	{
		const Donya::Vector4x4 V = iCamera.CalcViewMatrix();
		const Donya::Vector4x4 P = iCamera.GetProjectionMatrix();

		Donya::Vector4x4 VP{};
		VP._11 = Common::HalfScreenWidthF();
		VP._22 = -Common::HalfScreenHeightF();
		VP._41 = Common::HalfScreenWidthF();
		VP._42 = Common::HalfScreenHeightF();
		Donya::Vector4x4 invVP = VP.Inverse();

		return invVP * P.Inverse() * V.Inverse();
	}
	Donya::Vector3 CalcWorldMousePos( const Donya::Int2 &mousePos, float thirdParam )
	{
		const Donya::Vector4x4 matScreenToWorld = CalcScreenToWorldMatrix();

		auto MakeTransformed = []( const Donya::Vector3 &from, const Donya::Vector4x4 &M )
		{
			Donya::Vector4 transformed = M.Mul( from, 1.0f );
			transformed /= transformed.w;

			return transformed.XYZ();
		};

		const Donya::Vector3 ssPos	= Donya::Vector3{ mousePos.Float(), thirdParam };
		const Donya::Vector3 wsPos	= MakeTransformed( ssPos, matScreenToWorld );

		return wsPos;
	}

	Donya::Model::RaycastResult CalcRaycastPos( const MeshAndInfo &target, const Donya::Vector3 &wsRayStart, const Donya::Vector3 &wsRayEnd ) const
	{
		auto MakeWorldMatrix = []( const MeshAndInfo &source )
		{
			const Donya::Quaternion rotation = Donya::Quaternion::Make
			(
				ToRadian( source.rotation.x ),
				ToRadian( source.rotation.y ),
				ToRadian( source.rotation.z )
			);
			return Donya::Vector4x4::MakeTransformation( source.scale, rotation, source.translation );
		};

		const auto raycast = target.polyGroup.RaycastWorldSpace( MakeWorldMatrix( target ), wsRayStart, wsRayEnd );
		return raycast;
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
				bool loadSucceeded = tmpHeavyLoad.Load( filePath );

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
			if ( it.holder.GetMotionCount() < 1 ) { continue; }
			// else

			const auto &useMotion	= it.holder.GetMotion( it.usingMotionIndex );
			const auto currentPose	= it.animator.CalcCurrentPose( useMotion );
			if ( it.pose.HasCompatibleWith( currentPose ) )
			{
				it.pose.AssignSkeletal( currentPose );
			}

			it.animator.SetRepeatRange( useMotion );
			it.animator.Update( elapsedTime * it.motionAccelPercent );
			it.currentElapsedTime	= it.animator.GetInternalElapsedTime();
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

				ImGui::Checkbox( u8"レイキャストを使う", &useRaycast );
				if ( useRaycast ) { ImGui::Text( u8"レイは モデル[0]番 に対して飛ばします" ); }
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
			std::string fileName = "[" + source.GetFileName() + "]";
			std::string postfix = "##" + std::to_string( uniqueIndex++ );
			return fileName + postfix;
		};

		auto ShowDrawConfig		= [&]( MeshAndInfo &target )
		{
			if ( !ImGui::TreeNode( u8"描画設定" ) ) { return; }
			// else

			ImGui::Text( u8"※保存対象には含まれません" );
			
			ImGui::Checkbox  ( u8"隠す", &target.dontWannaDraw );

			ImGui::DragFloat3( u8"スケール",			&target.scale.x,		0.1f );
			ImGui::DragFloat3( u8"回転量（Degree）",	&target.rotation.x,		1.0f );
			ImGui::DragFloat3( u8"平行移動",			&target.translation.x,	1.0f );

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
			? target.animator.EnableLoop()
			: target.animator.DisableLoop();
			ImGui::Text( u8"モーションの再生が終わった瞬間か：%s", target.animator.WasEnded() ? u8"True" : u8"False" );

			// Maybe I will implement this.
			/*
			static int endCount = 0; if ( target.animator.WasEnded() ) { endCount++; }
			ImGui::Text( u8"EndCount : %d", endCount );

			static float timeRangeL = 0.0f;
			static float timeRangeR = 1.0f;
			ImGui::DragFloat( u8"再生範囲：Min", &timeRangeL, 0.01f );
			ImGui::DragFloat( u8"再生範囲：Max", &timeRangeR, 0.01f );
			if ( timeRangeL < 0.0f        ) { timeRangeL = 0.0f; }
			if ( timeRangeR <= timeRangeL ) { timeRangeR = timeRangeL + 0.001f; }
			target.animator.SetRepeatRange( timeRangeL, timeRangeR );
			*/

			ImGui::Text( u8"登録されているモーションの数：%d", target.holder.GetMotionCount() );

			// I should apply the appending/erasing of motion to loader's source also, because of the Lex outputs the loader.
			// But the accessing to loader's source is heavy, so now I do not apply that. That application will do at saving timing.

			auto AppendMotion = []( MeshAndInfo &target, const Donya::Model::Animation::Motion &addition )
			{
				target.holder.AppendMotion( addition );
				target.source.motions.emplace_back( addition );
			};
			auto EraseMotion  = []( MeshAndInfo &target, size_t eraseIndex )
			{
				target.holder.EraseMotion( scast<int>( eraseIndex ) );

				auto &sourceMotions = target.source.motions;
				sourceMotions.erase( sourceMotions.begin() + eraseIndex );
			};

			if ( ImGui::TreeNode( u8"モーションを追加する" ) )
			{
				ImGui::Text( u8"他のロード済みモデルからの追加になります" );

				auto ShowPart = [&]( const MeshAndInfo &from )
				{
					if ( &target == &from ) { return; } // Except the myself.
					// else

					const std::string srcModelName = MakeCaption( from.loader );
					const std::string srcMotionCount = u8"モーション数：" + std::to_string( from.source.motions.size() );
					if ( !ImGui::TreeNode( ( srcModelName + u8"・" + srcMotionCount ).c_str() ) ) { return; }
					// else

					if ( from.source.motions.empty() )
					{
						ImGui::Text( u8"モーションを持っていません" );
						return;
					}
					// else

					const std::string prefix{ u8"追加：" };
					std::string buttonCaption;
					for ( const auto &it : from.source.motions )
					{
						if ( it.keyFrames.empty() )
						{
							ImGui::Text( u8"中身が空っぽです：%s", it.name.c_str() ); continue;
						}
						// else

						if ( !target.pose.HasCompatibleWith( it.keyFrames.front() ) )
						{
							ImGui::Text( u8"追加先の骨格との互換性がありません：%s", it.name.c_str() ); continue;
						}
						// else

						buttonCaption = prefix + it.name;
						if ( !ImGui::Button( buttonCaption.c_str() ) ) { continue; }
						// else

						AppendMotion( target, it );
					}

					ImGui::TreePop();
				};

				for ( const auto &itr : models )
				{
					ShowPart( itr );
				}

				ImGui::TreePop();
			}
			if ( ImGui::TreeNode( u8"モーションを削除する" ) )
			{
				const std::string prefix{ u8"削除：" };;
				const size_t motionCount = target.holder.GetMotionCount();

				std::string buttonCaption{};
				size_t eraseIndex = motionCount;
				for ( size_t i = 0; i < motionCount; ++i )
				{
					buttonCaption = prefix + target.source.motions[i].name;
					if ( !ImGui::Button( buttonCaption.c_str() ) ) { continue; }
					// else

					eraseIndex = i;
				}

				if ( eraseIndex < motionCount )
				{
					EraseMotion( target, eraseIndex );
				}

				ImGui::TreePop();
			}

			const size_t motionCount = target.source.motions.size();
			if ( 2 <= motionCount )
			{
				const size_t max = motionCount - 1;
				ImGui::SliderInt( u8"描画するモーション番号", &target.usingMotionIndex, 0, max );
				target.usingMotionIndex = std::max( 0, std::min( scast<int>( max ), target.usingMotionIndex ) );
			}
			else
			{
				ImGui::Text( u8"描画するモーション番号：０" );
				target.usingMotionIndex = 0;
			}
			ImGui::Text( u8"描画するモーション名：%s", ( !motionCount ? "[EMPTY]" : target.source.motions[target.usingMotionIndex].name.c_str() ) );

			ImGui::TreePop();
		};
		auto ShowSourceConfig	= [&]( MeshAndInfo &target )
		{
			if ( !ImGui::TreeNode( u8"調整項目" ) ) { return; }
			// else

			if ( ImGui::TreeNode( u8"座標系変換行列" ) )
			{
				Donya::Vector4x4 &coordConv = target.source.coordinateConversion;
				const auto prevMatrix = coordConv;
				ImGui::SliderFloat4( "11, 12, 13, 14", &coordConv._11, -1.0f, 1.0f );
				ImGui::SliderFloat4( "21, 22, 23, 24", &coordConv._21, -1.0f, 1.0f );
				ImGui::SliderFloat4( "31, 32, 33, 34", &coordConv._31, -1.0f, 1.0f );
				ImGui::SliderFloat4( "41, 42, 43, 44", &coordConv._41, -1.0f, 1.0f );

				if ( ImGui::Button( u8"Identity" ) )
				{
					coordConv = Donya::Vector4x4::Identity();
				}

				if ( coordConv != prevMatrix )
				{
					target.polyGroup.ApplyCoordinateConversion( coordConv );
				}

				// For apply to screen immediately.
				target.model.pSkinning->SetCoordinateConversion( coordConv );
				target.model.pStatic->SetCoordinateConversion( coordConv );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"レイキャスト時のカリング" ) )
			{
				using Mode = Donya::Model::PolygonGroup::CullMode;
				
				auto MakeModeName = []( const Mode &mode )
				{
					switch ( mode )
					{
					case Mode::Back:  return "Back";
					case Mode::Front: return "Front";
					default: break;
					}
					return "!ERROR-TYPE!";
				};
				ImGui::Text( u8"いま：%s", MakeModeName( target.polyGroup.GetCullMode() ) );

				if ( ImGui::Button( u8"Back"  ) ) { target.polyGroup.ApplyCullMode( Mode::Back  ); }
				if ( ImGui::Button( u8"Front" ) ) { target.polyGroup.ApplyCullMode( Mode::Front ); }

				ImGui::TreePop();
			}

			ImGui::TreePop();
		};
		auto ShowLoaderConfig	= [&]( MeshAndInfo &target )
		{
			if ( !ImGui::TreeNode( u8"詳細" ) ) { return; }
			// else

			target.loader.ShowEnumNode( u8"数値" );

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
				// Apply the changes of motion that when ShowMotionConfig().
				it->loader.SetModelSource( it->source );
				it->loader.SetPolygonGroup( it->polyGroup );

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
			ShowSourceConfig( *it );
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
