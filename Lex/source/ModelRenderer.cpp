#include "ModelRenderer.h"

#include <exception>
#include <tuple>

#include "Donya/Donya.h"			// GetDevice().
#include "Donya/RenderingStates.h"	// For default shading.
#include "Donya/Constant.h"			// Use scast macro.

#include "Model.h"

namespace Donya
{
	namespace Model
	{
		namespace Impl
		{
			template<typename  SomeConstant>
			void AssignCommon( SomeConstant *pDest, const Constants::PerMesh::Common &source )
			{
				pDest->common.adjustMatrix = source.adjustMatrix;
			}
			template<typename  SomeConstant>
			void AssignBone  ( SomeConstant *pDest, const Constants::PerMesh::Bone   &source )
			{
				pDest->bone.boneTransforms = source.boneTransforms;
			}
		
			bool StaticMeshConstant::CreateBuffer( ID3D11Device *pDevice )
			{
				return cbuffer.Create( pDevice );
			}
			void StaticMeshConstant::Update( const Constants::PerMesh::Common &source )
			{
				AssignCommon( &cbuffer.data, source );
			}
			void StaticMeshConstant::Activate( const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext ) const
			{
				cbuffer.Activate( desc.setSlot, desc.setVS, desc.setPS, pImmediateContext );
			}
			void StaticMeshConstant::Deactivate( ID3D11DeviceContext *pImmediateContext ) const
			{
				cbuffer.Deactivate( pImmediateContext );
			}

			bool SkinningMeshConstant::CreateBuffer( ID3D11Device *pDevice )
			{
				return cbuffer.Create( pDevice );
			}
			void SkinningMeshConstant::Update( const Constants::PerMesh::Common &srcCommon, const Constants::PerMesh::Bone &srcBone )
			{
				AssignCommon( &cbuffer.data, srcCommon );
				AssignBone  ( &cbuffer.data, srcBone   );
			}
			void SkinningMeshConstant::Activate( const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext ) const
			{
				cbuffer.Activate( desc.setSlot, desc.setVS, desc.setPS, pImmediateContext );
			}
			void SkinningMeshConstant::Deactivate( ID3D11DeviceContext *pImmediateContext ) const
			{
				cbuffer.Deactivate( pImmediateContext );
			}
		}
	
		namespace EmbeddedSourceCode
		{
			static constexpr const char *EntryPointVS	= "VSMain";
			static constexpr const char *EntryPointPS	= "PSMain";
			static constexpr const int   SlotSRV		= 0;
			static constexpr const int   SlotSampler	= 0;

			// HACK: These shader codes have some same codes(struct, cbuffer), so I can unite them by std::string, but I want to constexpr.

			/// <summary>
			/// PS is common on Static and Skinning.
			/// </summary>
			static constexpr const char *PixelShaderCode()
			{
				return
				"struct VS_OUT\n"
				"{\n"
				"	float4		svPos		: SV_POSITION;\n"
				"	float4		wsPos		: POSITION;\n"
				"	float4		normal		: NORMAL;\n"
				"	float2		texCoord	: TEXCOORD0;\n"
				"};\n"
				"struct DirectionalLight\n"
				"{\n"
				"	float4		color;\n"
				"	float4		direction;\n"
				"};\n"
				"cbuffer CBPerScene : register( b0 )\n"
				"{\n"
				"	DirectionalLight cbDirLight;\n"
				"	float4		cbEyePosition;\n"
				"	row_major\n"
				"	float4x4	cbViewProj;\n"
				"};\n"
				"cbuffer CBPerModel : register( b1 )\n"
				"{\n"
				"	float4		cbDrawColor;\n"
				"	row_major\n"
				"	float4x4	cbWorld;\n"
				"};\n"
				"cbuffer CBPerSubset : register( b3 )\n"
				"{\n"
				"	float4		cbAmbient;\n"
				"	float4		cbDiffuse;\n"
				"	float4		cbSpecular;\n"
				"};\n"
				// See https://tech.cygames.co.jp/archives/2339/
				"float4 SRGBToLinear( float4 colorSRGB )\n"
				"{\n"
				"	return pow( colorSRGB, 2.2f );\n"
				"}\n"
				// See https://tech.cygames.co.jp/archives/2339/
				"float4 LinearToSRGB( float4 colorLinear )\n"
				"{\n"
				"	return pow( colorLinear, 1.0f / 2.2f );\n"
				"}\n"
				"float HalfLambert( float3 nwsNormal, float3 nwsToLightVec )\n"
				"{\n"
				"	float lambert = dot( nwsNormal, nwsToLightVec );\n"
				"	return ( lambert * 0.5f ) + 0.5f;\n"
				"}\n"
				"float Phong( float3 nwsNormal, float3 nwsToLightVec, float3 nwsToEyeVec )\n"
				"{\n"
				"	float3 nwsReflection  = normalize( reflect( -nwsToLightVec, nwsNormal ) );\n"
				"	float  specularFactor = max( 0.0f, dot( nwsToEyeVec, nwsReflection ) );\n"
				"	return specularFactor;\n"
				"}\n"
				"float3 CalcLightInfluence( float4 lightColor, float3 nwsPixelToLightVec, float3 nwsPixelNormal, float3 nwsEyeVector )\n"
				"{\n"
				"	float3	ambientColor	= cbAmbient.rgb;\n"
				"	float	diffuseFactor	= HalfLambert( nwsPixelNormal, nwsPixelToLightVec );\n"
				// "		diffuseFactor	= pow( diffuseFactor, 2.0f );\n"
				"	float3	diffuseColor	= cbDiffuse.rgb * diffuseFactor;\n"
				"	float	specularFactor	= Phong( nwsPixelNormal, nwsPixelToLightVec, nwsEyeVector );\n"
				"	float3	specularColor	= cbSpecular.rgb * specularFactor * cbSpecular.w;\n"

				"	float3	Ka				= ambientColor;\n"
				"	float3	Kd				= diffuseColor;\n"
				"	float3	Ks				= specularColor;\n"
				"	float3	light			= lightColor.rgb * lightColor.w;\n"
				"	return	Ka + ( ( Kd + Ks ) * light );\n"
				"}\n"

				"Texture2D		diffuseMap			: register( t0 );\n"
				"SamplerState	diffuseMapSampler	: register( s0 );\n"

				"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
				"{\n"
				"			pin.normal		= normalize( pin.normal );\n"
			
				"	float3	nLightVec		= normalize( -cbDirLight.direction.rgb );	// Vector from position.\n"
				"	float4	nEyeVector		= cbEyePosition - pin.wsPos;				// Vector from position.\n"

				"	float4	diffuseMapColor	= diffuseMap.Sample( diffuseMapSampler, pin.texCoord );\n"
				"			diffuseMapColor	= SRGBToLinear( diffuseMapColor );\n"
				"	float	diffuseMapAlpha	= diffuseMapColor.a;\n"

				"	float3	totalLight		= CalcLightInfluence( cbDirLight.color, nLightVec, pin.normal.rgb, nEyeVector.rgb );\n"

				"	float3	resultColor		= diffuseMapColor.rgb * totalLight;\n"
				"	float4	outputColor		= float4( resultColor, diffuseMapAlpha );\n"
				"			outputColor		= outputColor * cbDrawColor;\n"

				"	return	LinearToSRGB( outputColor );\n"
				"}\n"
				;
			}
			static constexpr const char *PixelShaderName = "Donya::DefaultModelPS";

			static constexpr const char *SkinningCode()
			{
				return
				"struct VS_IN\n"
				"{\n"
				"	float4		pos			: POSITION;\n"
				"	float4		normal		: NORMAL;\n"
				"	float2		texCoord	: TEXCOORD0;\n"
				"	float4		weights		: WEIGHTS;\n"
				"	uint4		bones		: BONES;\n"
				"};\n"
				"struct VS_OUT\n"
				"{\n"
				"	float4		svPos		: SV_POSITION;\n"
				"	float4		wsPos		: POSITION;\n"
				"	float4		normal		: NORMAL;\n"
				"	float2		texCoord	: TEXCOORD0;\n"
				"};\n"
				"struct DirectionalLight\n"
				"{\n"
				"	float4		color;\n"
				"	float4		direction;\n"
				"};\n"
				"cbuffer CBPerScene : register( b0 )\n"
				"{\n"
				"	DirectionalLight cbDirLight;\n"
				"	float4		cbEyePosition;\n"
				"	row_major\n"
				"	float4x4	cbViewProj;\n"
				"};\n"
				"cbuffer CBPerModel : register( b1 )\n"
				"{\n"
				"	float4		cbDrawColor;\n"
				"	row_major\n"
				"	float4x4	cbWorld;\n"
				"};\n"
				"static const uint MAX_BONE_COUNT = 64U;\n"
				"cbuffer CBPerMesh : register( b2 )\n"
				"{\n"
				"	row_major\n"
				"	float4x4	cbAdjustMatrix;\n"
				"	row_major\n"
				"	float4x4	cbBoneTransforms[MAX_BONE_COUNT];\n"
				"};\n"
				"void ApplyBoneMatrices( float4 boneWeights, uint4 boneIndices, inout float4 inoutPosition, inout float4 inoutNormal )\n"
				"{\n"
				"	const float4 inPosition	= float4( inoutPosition.xyz, 1.0f );\n"
				"	const float4 inNormal	= float4( inoutNormal.xyz,   0.0f );\n"
				"	float3 resultPos		= { 0, 0, 0 };\n"
				"	float3 resultNormal		= { 0, 0, 0 };\n"
				"	float  weight			= 0;\n"
				"	row_major float4x4 transform = 0;\n"
				"	for ( int i = 0; i < 4/* float4 */; ++i )\n"
				"	{\n"
				"		weight			= boneWeights[i];\n"
				"		transform		= cbBoneTransforms[boneIndices[i]];\n"
				"		resultPos		+= ( weight * mul( inPosition,	transform ) ).xyz;\n"
				"		resultNormal	+= ( weight * mul( inNormal,	transform ) ).xyz;\n"
				"	}\n"
				"	inoutPosition	= float4( resultPos,    1.0f );\n"
				"	inoutNormal		= float4( resultNormal, 0.0f );\n"
				"}\n"
				"VS_OUT VSMain( VS_IN vin )\n"
				"{\n"
				"	vin.pos.w		= 1.0f;\n"
				"	vin.normal.w	= 0.0f;\n"
				"	ApplyBoneMatrices( vin.weights, vin.bones, vin.pos, vin.normal );\n"

				"	float4x4 W		= mul( cbAdjustMatrix, cbWorld );\n"
				"	float4x4 WVP	= mul( W, cbViewProj );\n"

				"	VS_OUT vout		= ( VS_OUT )0;\n"
				"	vout.wsPos		= mul( vin.pos, W );\n"
				"	vout.svPos		= mul( vin.pos, WVP );\n"
				"	vout.normal		= normalize( mul( vin.normal, W ) );\n"
				"	vout.texCoord	= vin.texCoord;\n"
				"	return vout;\n"
				"}\n"
				;
			}
			static constexpr const char *SkinningNameVS	= "Donya::DefaultModelSkinningVS";
			
			static constexpr const char *StaticCode()
			{
				return
				"struct VS_IN\n"
				"{\n"
				"	float4		pos			: POSITION;\n"
				"	float4		normal		: NORMAL;\n"
				"	float2		texCoord	: TEXCOORD0;\n"
				"};\n"
				"struct VS_OUT\n"
				"{\n"
				"	float4		svPos		: SV_POSITION;\n"
				"	float4		wsPos		: POSITION;\n"
				"	float4		normal		: NORMAL;\n"
				"	float2		texCoord	: TEXCOORD0;\n"
				"};\n"
				"struct DirectionalLight\n"
				"{\n"
				"	float4		color;\n"
				"	float4		direction;\n"
				"};\n"
				"cbuffer CBPerScene : register( b0 )\n"
				"{\n"
				"	DirectionalLight cbDirLight;\n"
				"	float4		cbEyePosition;\n"
				"	row_major\n"
				"	float4x4	cbViewProj;\n"
				"};\n"
				"cbuffer CBPerModel : register( b1 )\n"
				"{\n"
				"	float4		cbDrawColor;\n"
				"	row_major\n"
				"	float4x4	cbWorld;\n"
				"};\n"
				"cbuffer CBPerMesh : register( b2 )\n"
				"{\n"
				"	row_major\n"
				"	float4x4	cbAdjustMatrix;\n"
				"};\n"
				"VS_OUT VSMain( VS_IN vin )\n"
				"{\n"
				"	vin.pos.w		= 1.0f;\n"
				"	vin.normal.w	= 0.0f;\n"

				"	float4x4 W		= mul( cbAdjustMatrix, cbWorld );\n"
				"	float4x4 WVP	= mul( W, cbViewProj );\n"

				"	VS_OUT vout		= ( VS_OUT )( 0 );\n"
				"	vout.wsPos		= mul( vin.pos, W );\n"
				"	vout.svPos		= mul( vin.pos, WVP );\n"
				"	vout.normal		= normalize( mul( vin.normal, W ) );\n"
				"	vout.texCoord	= vin.texCoord;\n"
				"	return vout;\n"
				"}\n"
				;
			}
			static constexpr const char *StaticNameVS	= "Donya::DefaultModelStaticVS";
		}

		// TODO : To specifiable these configuration by arguments of render method.

		static constexpr D3D11_DEPTH_STENCIL_DESC	DefaultDepthStencilDesc()
		{
			D3D11_DEPTH_STENCIL_DESC standard{};
			standard.DepthEnable		= TRUE;
			standard.DepthWriteMask		= D3D11_DEPTH_WRITE_MASK_ALL;
			standard.DepthFunc			= D3D11_COMPARISON_LESS;
			standard.StencilEnable		= FALSE;
			return standard;
		}
		static constexpr D3D11_RASTERIZER_DESC		DefaultRasterizerDesc()
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
		static constexpr D3D11_SAMPLER_DESC			DefaultSamplerDesc()
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

		/*
		Build input-element-descs are:
		POSITION	:	DXGI_FORMAT_R32G32B32_FLOAT,
		NORMAL		:	DXGI_FORMAT_R32G32B32_FLOAT,
		TEXCOORD	:	DXGI_FORMAT_R32G32_FLOAT,
		*/
		static std::vector<D3D11_INPUT_ELEMENT_DESC> MakeInputElementsStatic()
		{
			const auto IEDescsPos = Vertex::Pos::GenerateInputElements( 0 );
			const auto IEDescsTex = Vertex::Tex::GenerateInputElements( 1 );

			std::vector<D3D11_INPUT_ELEMENT_DESC> wholeIEDescs{};
			wholeIEDescs.insert( wholeIEDescs.end(), IEDescsPos.begin(), IEDescsPos.end() );
			wholeIEDescs.insert( wholeIEDescs.end(), IEDescsTex.begin(), IEDescsTex.end() );
			return wholeIEDescs;
		}
		/*
		Build input-element-descs are:
		POSITION	:	DXGI_FORMAT_R32G32B32_FLOAT,
		NORMAL		:	DXGI_FORMAT_R32G32B32_FLOAT,
		TEXCOORD	:	DXGI_FORMAT_R32G32_FLOAT,
		WEIGHTS		:	DXGI_FORMAT_R32G32B32A32_FLOAT,
		BONES		:	DXGI_FORMAT_R32G32B32A32_UINT,
		*/
		static std::vector<D3D11_INPUT_ELEMENT_DESC> MakeInputElementsSkinned()
		{
			const auto IEDescsBone = Vertex::Bone::GenerateInputElements( 2 );

			std::vector<D3D11_INPUT_ELEMENT_DESC> wholeIEDescs = MakeInputElementsStatic();
			wholeIEDescs.insert( wholeIEDescs.end(), IEDescsBone.begin(), IEDescsBone.end() );
			return wholeIEDescs;
		}


		std::unique_ptr<Renderer::Default::Member> Renderer::Default::pMember = nullptr;
		bool Renderer::Default::Initialize( ID3D11Device *pDevice )
		{
			// Already initialized.
			if ( pMember ) { return true; }
			// else

			bool result		= true;
			bool succeeded	= true;
			pMember = std::make_unique<Renderer::Default::Member>();

			result = AssignStatusIdentifiers( pDevice );
			if ( !result ) { succeeded = false; }
			result = CreateRenderingStates  ( pDevice );
			if ( !result ) { succeeded = false; }
			result = CreateCBuffers( pDevice );
			if ( !result ) { succeeded = false; }
			result = CreateDefaultShaders   ( pDevice );
			if ( !result ) { succeeded = false; }

			// Represent "the members were not initialized".
			if ( !succeeded )
			{
				pMember.reset();
			}

			return succeeded;
		}

		bool Renderer::Default::AssignStatusIdentifiers( ID3D11Device *pDevice )
		{
			using FindFunction = std::function<bool( int )>;

			auto  AssertNotFound		= []( const std::wstring &stateName )
			{
				const std::wstring expression = L"Unexpected Error : We can't found a space of a ModelRenderer's default state of ";
				_ASSERT_EXPR( 0, ( expression + stateName + L"." ).c_str() );
			};
			auto  AssignStateIdentifier	= [&AssertNotFound]( int *pIdentifier, const std::wstring &stateName, const FindFunction &IsAlreadyExists )
			{
				// Already assigned.
				if ( *pIdentifier != Member::DEFAULT_ID ) { return true; }
				// else

				*pIdentifier = Member::DEFAULT_ID;

				// The internal object use minus value to identifier.
				for ( int i = -1; -INT_MAX < i; --i )
				{
					if ( IsAlreadyExists( i ) ) { continue; }
					// else

					*pIdentifier = i;
					break;
				}

				// If usable identifier was not found.
				if ( *pIdentifier == Member::DEFAULT_ID )
				{
					AssertNotFound( stateName );
					return false;
				}
				// else
				return true;
			};

			using Bundle = std::tuple<int *, std::wstring, FindFunction>;
			constexpr  size_t  STATE_COUNT = 3;
			std::array<Bundle, STATE_COUNT> bundles
			{
				std::make_tuple( &pMember->idDSState,	L"DepthStencil",	Donya::DepthStencil::IsAlreadyExists	),
				std::make_tuple( &pMember->idRSState,	L"Rasterizer",		Donya::Rasterizer::IsAlreadyExists		),
				std::make_tuple( &pMember->idPSSampler,	L"Sampler",			Donya::Sampler::IsAlreadyExists			),
			};

			bool succeeded = true;
			for ( size_t i = 0; i < STATE_COUNT; ++i )
			{
				// C++17
				// std::apply( AssignStateIdentifier, bundles[i] );

				bool result = AssignStateIdentifier
				(
					std::get<int *>( bundles[i] ),
					std::get<std::wstring>( bundles[i] ),
					std::get<FindFunction>( bundles[i] )
				);
				if ( !result )
				{
					succeeded = false;
				}
			}
			return succeeded;
		}
		bool Renderer::Default::CreateRenderingStates( ID3D11Device *pDevice )
		{
			if ( pMember->idDSState   == Member::DEFAULT_ID ||
				 pMember->idRSState   == Member::DEFAULT_ID ||
				 pMember->idPSSampler == Member::DEFAULT_ID )
			{
				_ASSERT_EXPR( 0, L"Error : Some status identifier of ModelRenderer is invalid!" );
				return false;
			}
			// else

			auto AssertFailedCreation	= []( const std::wstring &stateName )
			{
				const std::wstring expression = L"Failed : Create a ModelRenderer's default state of ";
				_ASSERT_EXPR( 0, ( expression + stateName + L"." ).c_str() );
			};

			constexpr auto descDS = DefaultDepthStencilDesc();
			constexpr auto descRS = DefaultRasterizerDesc();
			constexpr auto descPS = DefaultSamplerDesc();

			bool result		= true;
			bool succeeded	= true;

			result = Donya::DepthStencil::CreateState( pMember->idDSState, descDS );
			if ( !result )
			{
				AssertFailedCreation( L"DepthStencil" );
				succeeded = false;
			}

			result = Donya::Rasterizer::CreateState( pMember->idRSState, descRS );
			if ( !result )
			{
				AssertFailedCreation( L"Rasterizer" );
				succeeded = false;
			}

			result = Donya::Sampler::CreateState( pMember->idPSSampler, descPS );
			if ( !result )
			{
				AssertFailedCreation( L"Sampler" );
				succeeded = false;
			}

			return succeeded;
		}
		bool Renderer::Default::CreateCBuffers( ID3D11Device *pDevice )
		{
			bool result		= true;
			bool succeeded	= true;

			result = pMember->CBPerScene.Create( pDevice );
			if ( !result ) { succeeded = false; }
			result = pMember->CBPerModel.Create( pDevice );
			if ( !result ) { succeeded = false; }

			return succeeded;
		}
		bool Renderer::Default::CreateDefaultShaders( ID3D11Device *pDevice )
		{
			auto AssertFailedCreation = []( const std::wstring &shaderName )
			{
				const std::wstring expression = L"Failed : Create a ModelRenderer's default-shader of ";
				_ASSERT_EXPR( 0, ( expression + shaderName + L"." ).c_str() );
			};

			namespace Source = EmbeddedSourceCode;

			bool result		= true;
			bool succeeded	= true;

			// For Skinning.
			{
				/*
				result = pMember->shaderSkinning.VS.CreateByCSO
				(
					"./Data/Shader/SourceSkinnedMeshVS.cso",
					MakeInputElementsSkinned(),
					pDevice
				);
				*/
				result = pMember->shaderSkinning.VS.CreateByEmbededSourceCode
				(
					Source::SkinningNameVS, Source::SkinningCode(), Source::EntryPointVS,
					MakeInputElementsSkinned(),
					pDevice
				);
				if ( !result )
				{
					AssertFailedCreation( L"Skinned_VS" );
					succeeded = false;
				}

				/*
				result = pMember->shaderSkinning.PS.CreateByCSO
				(
					"./Data/Shader/SourceSkinnedMeshPS.cso", pDevice
				);
				*/
				result = pMember->shaderSkinning.PS.CreateByEmbededSourceCode
				(
					Source::PixelShaderName, Source::PixelShaderCode(), Source::EntryPointPS,
					pDevice
				);
				if ( !result )
				{
					AssertFailedCreation( L"Skinned_PS" );
					succeeded = false;
				}
			}
			// For Static.
			{
				result = pMember->shaderStatic.VS.CreateByEmbededSourceCode
				(
					Source::StaticNameVS, Source::StaticCode(), Source::EntryPointVS,
					MakeInputElementsStatic(),
					pDevice
				);
				if ( !result )
				{
					AssertFailedCreation( L"Static_VS" );
					succeeded = false;
				}

				succeeded = pMember->shaderStatic.PS.CreateByEmbededSourceCode
				(
					Source::PixelShaderName, Source::PixelShaderCode(), Source::EntryPointPS,
					pDevice
				);
				if ( !result )
				{
					AssertFailedCreation( L"Static_PS" );
					succeeded = false;
				}
			}

			return succeeded;
		}

		bool Renderer::Default::ActivateDepthStencil( ID3D11DeviceContext *pImmediateContext )
		{
			return Donya::DepthStencil::Activate( pMember->idDSState, pImmediateContext );
		}
		bool Renderer::Default::ActivateRasterizer( ID3D11DeviceContext *pImmediateContext )
		{
			return Donya::Rasterizer::Activate( pMember->idRSState, pImmediateContext );
		}
		bool Renderer::Default::ActivateSampler( ID3D11DeviceContext *pImmediateContext )
		{
			const RegisterDesc desc = DescSampler();
			return Donya::Sampler::Activate( pMember->idPSSampler, desc.setSlot, desc.setVS, desc.setPS, pImmediateContext );
		}
		void Renderer::Default::DeactivateDepthStencil( ID3D11DeviceContext *pImmediateContext )
		{
			Donya::DepthStencil::Deactivate( pImmediateContext );
		}
		void Renderer::Default::DeactivateRasterizer( ID3D11DeviceContext *pImmediateContext )
		{
			Donya::Rasterizer::Deactivate( pImmediateContext );
		}
		void Renderer::Default::DeactivateSampler( ID3D11DeviceContext *pImmediateContext )
		{
			Donya::Sampler::Deactivate( pImmediateContext );
		}

		void Renderer::Default::ActivateVertexShaderSkinning( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderSkinning.VS.Activate( pImmediateContext );
		}
		void Renderer::Default::ActivatePixelShaderSkinning( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderSkinning.PS.Activate( pImmediateContext );
		}
		void Renderer::Default::ActivateVertexShaderStatic( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderStatic.VS.Activate( pImmediateContext );
		}
		void Renderer::Default::ActivatePixelShaderStatic( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderStatic.PS.Activate( pImmediateContext );
		}
		void Renderer::Default::DeactivateVertexShaderSkinning( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderSkinning.VS.Deactivate( pImmediateContext );
		}
		void Renderer::Default::DeactivatePixelShaderSkinning( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderSkinning.PS.Deactivate( pImmediateContext );
		}
		void Renderer::Default::DeactivateVertexShaderStatic( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderStatic.VS.Deactivate( pImmediateContext );
		}
		void Renderer::Default::DeactivatePixelShaderStatic( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderStatic.PS.Deactivate( pImmediateContext );
		}

		void Renderer::Default::UpdateCBufferScene( const Constants::PerScene::Common &param )
		{
			pMember->CBPerScene.data = param;
		}
		void Renderer::Default::UpdateCBufferModel( const Constants::PerModel::Common &param )
		{
			pMember->CBPerModel.data = param;
		}
		void Renderer::Default::ActivateCBufferScene( ID3D11DeviceContext *pImmediateContext )
		{
			const RegisterDesc desc = DescCBufferPerScene();
			pMember->CBPerScene.Activate( desc.setSlot, desc.setVS, desc.setPS, pImmediateContext );
		}
		void Renderer::Default::ActivateCBufferModel( ID3D11DeviceContext *pImmediateContext )
		{
			const RegisterDesc desc = DescCBufferPerModel();
			pMember->CBPerModel.Activate( desc.setSlot, desc.setVS, desc.setPS, pImmediateContext );
		}
		void Renderer::Default::DeactivateCBufferScene( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->CBPerScene.Deactivate( pImmediateContext );
		}
		void Renderer::Default::DeactivateCBufferModel( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->CBPerModel.Deactivate( pImmediateContext );
		}

		RegisterDesc Renderer::Default::DescCBufferPerScene()
		{
			return RegisterDesc::Make( 0, true, true );
		}
		RegisterDesc Renderer::Default::DescCBufferPerModel()
		{
			return RegisterDesc::Make( 1, true, true );
		}
		RegisterDesc Renderer::Default::DescCBufferPerMesh()
		{
			return RegisterDesc::Make( 2, true, false );
		}
		RegisterDesc Renderer::Default::DescCBufferPerSubset()
		{
			return RegisterDesc::Make( 3, false, true );
		}
		RegisterDesc Renderer::Default::DescSampler()
		{
			return RegisterDesc::Make( 0, false, true );
		}
		RegisterDesc Renderer::Default::DescDiffuseMap()
		{
			return RegisterDesc::Make( 0, false, true );
		}

		namespace
		{
			void SetDefaultIfNullptr( ID3D11Device **ppDevice )
		{
			if ( !ppDevice || *ppDevice ) { return; }
			// else
			*ppDevice = Donya::GetDevice();
		}
		}

		Renderer::Renderer( ID3D11Device *pDevice ) :
			CBPerSubset()
		{
			SetDefaultIfNullptr( &pDevice );

			bool  result = CBPerSubset.Create( pDevice );
			if ( !result )
			{
				const std::string errMsg =
					"Exception : Creation of a constant-buffer per subset is failed.";
				throw std::runtime_error{ errMsg };
			}
		}

		void Renderer::SetVertexBuffers( const Model &model, size_t meshIndex, ID3D11DeviceContext *pImmediateContext )
		{
			const auto &meshes	= model.GetMeshes();
			const auto &mesh	= meshes[meshIndex];
			mesh.pVertex->SetVertexBuffers( pImmediateContext );
		}
		void Renderer::SetIndexBuffer( const Model &model, size_t meshIndex, ID3D11DeviceContext *pImmediateContext )
		{
			const auto &meshes	= model.GetMeshes();
			const auto &mesh	= meshes[meshIndex];
			pImmediateContext->IASetIndexBuffer( mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );
		}

		void Renderer::DrawEachSubsets( const Model &model, size_t meshIndex, const RegisterDesc &descSubset, const RegisterDesc &descDiffuseMap, ID3D11DeviceContext *pImmediateContext )
		{
			const auto &meshes	= model.GetMeshes();
			const auto &mesh	= meshes[meshIndex];

			const size_t subsetCount = mesh.subsets.size();
			for ( size_t j = 0; j < subsetCount; ++j )
			{
				const auto &subset = mesh.subsets[j];
				UpdateCBPerSubset( model, meshIndex, j, descSubset, pImmediateContext );

				ActivateCBPerSubset( descSubset, pImmediateContext );
				SetTexture( descDiffuseMap, subset.diffuse.pSRV.GetAddressOf(), pImmediateContext );

				DrawIndexed( model, meshIndex, j, pImmediateContext );

				UnsetTexture( descDiffuseMap, pImmediateContext );
				DeactivateCBPerSubset( pImmediateContext );
			}
		}

		void Renderer::UpdateCBPerSubset( const Donya::Model::Model &model, size_t meshIndex, size_t subsetIndex, const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			const auto &meshes = model.GetMeshes();
			const auto &mesh   = meshes[meshIndex];
			const auto &subset = mesh.subsets[subsetIndex];

			Constants::PerSubset::Common constants{};
			constants.ambient  = subset.ambient.color;
			constants.diffuse  = subset.diffuse.color;
			constants.specular = subset.specular.color;
			
			CBPerSubset.data   = constants;
		}
		void Renderer::ActivateCBPerSubset( const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			CBPerSubset.Activate( desc.setSlot, desc.setVS, desc.setPS, pImmediateContext );
		}
		void Renderer::DeactivateCBPerSubset( ID3D11DeviceContext *pImmediateContext )
		{
			CBPerSubset.Deactivate( pImmediateContext );
		}

		void Renderer::SetTexture( const RegisterDesc &desc, SRVType SRV, ID3D11DeviceContext *pImmediateContext ) const
		{
			if ( desc.setVS )
			{
				pImmediateContext->VSSetShaderResources( desc.setSlot, 1U, SRV );
			}
			if ( desc.setPS )
			{
				pImmediateContext->PSSetShaderResources( desc.setSlot, 1U, SRV );
			}
		}
		void Renderer::UnsetTexture( const RegisterDesc &descDiffuse, ID3D11DeviceContext *pImmediateContext ) const
		{
			ID3D11ShaderResourceView *pNullSRV = nullptr;
			if ( descDiffuse.setVS )
			{
				pImmediateContext->VSSetShaderResources( descDiffuse.setSlot, 1U, &pNullSRV );
			}
			if ( descDiffuse.setPS )
			{
				pImmediateContext->PSSetShaderResources( descDiffuse.setSlot, 1U, &pNullSRV );
			}
		}

		void Renderer::DrawIndexed( const Model &model, size_t meshIndex, size_t subsetIndex, ID3D11DeviceContext *pImmediateContext ) const
		{
			const auto &meshes	= model.GetMeshes();
			const auto &mesh	= meshes[meshIndex];
			const auto &subset	= mesh.subsets[subsetIndex];

			pImmediateContext->DrawIndexed( subset.indexCount, subset.indexStart, 0 );
		}


		namespace
		{
			void SetDefaultIfNullptr( ID3D11DeviceContext **ppImmediateContext )
			{
				if ( !ppImmediateContext || *ppImmediateContext ) { return; }
				// else
				*ppImmediateContext = Donya::GetImmediateContext();
			}

			Donya::Vector4x4 ExtractInitialPose( const Model &model, size_t meshIndex, const Pose &pose )
			{
				const auto &meshes	= model.GetMeshes();
				const auto &mesh	= meshes[meshIndex];
				return pose.GetCurrentPose()[mesh.boneIndex].global;
			}
		}


		StaticRenderer::StaticRenderer( ID3D11Device *pDevice ) : Renderer( pDevice ),
			CBPerMesh()
		{
			SetDefaultIfNullptr( &pDevice );

			bool  result = CBPerMesh.CreateBuffer( pDevice );
			if ( !result )
			{
				const std::string errMsg =
					"Exception : Creation of a constant-buffer per mesh is failed.";
				throw std::runtime_error{ errMsg };
			}
		}
		void StaticRenderer::Render( const StaticModel &model, const Pose &pose, const RegisterDesc &descMesh, const RegisterDesc &descSubset, const RegisterDesc &descDiffuseMap, ID3D11DeviceContext *pImmediateContext )
		{
			SetDefaultIfNullptr( &pImmediateContext );

			pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

			const size_t meshCount = model.GetMeshes().size();
			for ( size_t i = 0; i < meshCount; ++i )
			{
				UpdateCBPerMesh( model, i, pose, descMesh, pImmediateContext );
				ActivateCBPerMesh( descMesh, pImmediateContext );

				SetVertexBuffers( model, i, pImmediateContext );
				SetIndexBuffer( model, i, pImmediateContext );

				DrawEachSubsets( model, i, descSubset, descDiffuseMap, pImmediateContext );

				DeactivateCBPerMesh( pImmediateContext );
			}
		}
		Constants::PerMesh::Common StaticRenderer::MakeCommonConstantsPerMesh( const Model &model, size_t meshIndex, const Pose &pose ) const
		{
			Constants::PerMesh::Common constants;
			constants.adjustMatrix =
				ExtractInitialPose( model, meshIndex, pose ) *
				model.GetCoordinateConversion();
			return constants;
		}
		void StaticRenderer::UpdateCBPerMesh( const Model &model, size_t meshIndex, const Pose &pose, const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			CBPerMesh.Update( MakeCommonConstantsPerMesh( model, meshIndex, pose ) );
		}
		void StaticRenderer::ActivateCBPerMesh( const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			CBPerMesh.Activate( desc, pImmediateContext );
		}
		void StaticRenderer::DeactivateCBPerMesh( ID3D11DeviceContext *pImmediateContext )
		{
			CBPerMesh.Deactivate( pImmediateContext );
		}


		SkinningRenderer::SkinningRenderer( ID3D11Device *pDevice ) : Renderer( pDevice ),
			CBPerMesh()
		{
			SetDefaultIfNullptr( &pDevice );

			bool  result = CBPerMesh.CreateBuffer( pDevice );
			if ( !result )
			{
				const std::string errMsg =
					"Exception : Creation of a constant-buffer per mesh is failed.";
				throw std::runtime_error{ errMsg };
			}
		}
		void SkinningRenderer::Render( const SkinningModel &model, const Pose &pose, const RegisterDesc &descMesh, const RegisterDesc &descSubset, const RegisterDesc &descDiffuseMap, ID3D11DeviceContext *pImmediateContext )
		{
			SetDefaultIfNullptr( &pImmediateContext );

			pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

			const size_t meshCount = model.GetMeshes().size();
			for ( size_t i = 0; i < meshCount; ++i )
			{
				UpdateCBPerMesh( model, i, pose, descMesh, pImmediateContext );
				ActivateCBPerMesh( descMesh, pImmediateContext );

				SetVertexBuffers( model, i, pImmediateContext );
				SetIndexBuffer( model, i, pImmediateContext );

				DrawEachSubsets( model, i, descSubset, descDiffuseMap, pImmediateContext );

				DeactivateCBPerMesh( pImmediateContext );
			}
		}
		Constants::PerMesh::Common SkinningRenderer::MakeCommonConstantsPerMesh( const Model &model, size_t meshIndex, const Pose &pose ) const
		{
			Constants::PerMesh::Common constants;
			constants.adjustMatrix =
				// This matrix is contained into a bone-transform matrix.
				// ExtractInitialPose( model, meshIndex ) *
				model.GetCoordinateConversion();
			return constants;
		}
		Constants::PerMesh::Bone   SkinningRenderer::MakeBoneConstants( const Model &model, size_t meshIndex, const Pose &pose ) const
		{
			const auto &meshes		= model.GetMeshes();
			const auto &mesh		= meshes[meshIndex];
			const auto &currentPose	= pose.GetCurrentPose();
			Constants::PerMesh::Bone constants{};

			if ( mesh.boneIndices.empty() )
			{
				constants.boneTransforms[0] = currentPose[mesh.boneIndex].global;
				return constants;
			}
			// else

			Donya::Vector4x4 meshToBone{}; // So-called "Bone offset matrix".
			Donya::Vector4x4 boneToMesh{}; // Transform to mesh space of current pose.
			const size_t boneCount = std::min( mesh.boneIndices.size(), scast<size_t>( Constants::PerMesh::Bone::MAX_BONE_COUNT ) );
			for ( size_t i = 0; i < boneCount; ++i )
			{
				const size_t poseIndex = mesh.boneIndices[i]; // This index was fetched with boneOffset's name.
				meshToBone = mesh.boneOffsets[i].global;
				boneToMesh = currentPose[poseIndex].global;
				
				constants.boneTransforms[i] = meshToBone * boneToMesh;
			}

			return constants;
		}
		void SkinningRenderer::UpdateCBPerMesh( const Model &model, size_t meshIndex, const Pose &pose, const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			Constants::PerMesh::Common	constantsCommon	= MakeCommonConstantsPerMesh( model, meshIndex, pose );
			Constants::PerMesh::Bone	constantsBone	= MakeBoneConstants( model, meshIndex, pose );
			
			CBPerMesh.Update( constantsCommon, constantsBone );
		}
		void SkinningRenderer::ActivateCBPerMesh( const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			CBPerMesh.Activate( desc, pImmediateContext );
		}
		void SkinningRenderer::DeactivateCBPerMesh( ID3D11DeviceContext *pImmediateContext )
		{
			CBPerMesh.Deactivate( pImmediateContext );
		}
	}
}
