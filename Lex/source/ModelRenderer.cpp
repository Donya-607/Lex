#include "ModelRenderer.h"

#include <exception>
#include <tuple>

#include "Donya/Donya.h"			// GetDevice().
#include "Donya/RenderingStates.h"	// For default shading.

#include "Model.h"

namespace Donya
{
	namespace Model
	{
		namespace Strategy
		{
			template<typename  SomeConstants>
			void AssignCommon( SomeConstants *pDest, const CBStructPerMesh::Common &source )
			{
				pDest->common.adjustMatrix = source.adjustMatrix;
			}
			template<typename  SomeConstants>
			void AssignBone(   SomeConstants *pDest, const CBStructPerMesh::Bone &source )
			{
				pDest->bone.boneTransforms = source.boneTransforms;
			}

			bool SkinnedConstantsPerMesh::CreateBuffer( ID3D11Device *pDevice )
			{
				return cbuffer.Create( pDevice );
			}
			void SkinnedConstantsPerMesh::Update( const CBStructPerMesh::Common &source )
			{
				AssignCommon( &cbuffer.data, source );
			}
			void SkinnedConstantsPerMesh::Update( const CBStructPerMesh::Common &srcCommon, const CBStructPerMesh::Bone &srcBone )
			{
				Update( srcCommon );
				AssignBone( &cbuffer.data, srcBone );
			}
			void SkinnedConstantsPerMesh::Activate( unsigned int setSlot, bool setVS, bool setPS, ID3D11DeviceContext *pImmediateContext ) const
			{
				cbuffer.Activate( setSlot, setVS, setPS, pImmediateContext );
			}
			void SkinnedConstantsPerMesh::Deactivate( ID3D11DeviceContext *pImmediateContext ) const
			{
				cbuffer.Deactivate( pImmediateContext );
			}
		
			bool StaticConstantsPerMesh::CreateBuffer( ID3D11Device *pDevice )
			{
				return cbuffer.Create( pDevice );
			}
			void StaticConstantsPerMesh::Update( const CBStructPerMesh::Common &source )
			{
				AssignCommon( &cbuffer.data, source );
			}
			void StaticConstantsPerMesh::Activate( unsigned int setSlot, bool setVS, bool setPS, ID3D11DeviceContext *pImmediateContext ) const
			{
				cbuffer.Activate( setSlot, setVS, setPS, pImmediateContext );
			}
			void StaticConstantsPerMesh::Deactivate( ID3D11DeviceContext *pImmediateContext ) const
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

			static constexpr const char *SkinnedCode()
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
				"	float4		pos			: SV_POSITION;\n"
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
				"cbuffer CBPerSubset : register( b3 )\n"
				"{\n"
				"	float4		cbAmbient;\n"
				"	float4		cbDiffuse;\n"
				"	float4		cbSpecular;\n"
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
				"	vout.pos		= mul( vin.pos, WVP );\n"
				"	vout.normal		= normalize( mul( vin.normal, W ) );\n"
				"	vout.texCoord	= vin.texCoord;\n"
				"	return vout;\n"
				"}\n"
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
				"	return 1;\n"
				"	if( isnan( pin.normal.x ) ) { return float4(0.0f, 1.0f, 0.0f, 1.0f); }\n"
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
			static constexpr const char *SkinnedNameVS	= "Donya::DefaultModelSkinnedVS";
			static constexpr const char *SkinnedNamePS	= "Donya::DefaultModelSkinnedPS";

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
				"	float4		pos			: SV_POSITION;\n"
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
				"cbuffer CBPerSubset : register( b3 )\n"
				"{\n"
				"	float4		cbAmbient;\n"
				"	float4		cbDiffuse;\n"
				"	float4		cbSpecular;\n"
				"};\n"
				"VS_OUT VSMain( VS_IN vin )\n"
				"{\n"
				"	vin.pos.w		= 1.0f;\n"
				"	vin.normal.w	= 0.0f;\n"

				"	row_major float4x4 W = mul( cbAdjustMatrix, cbWorld );\n"

				"	VS_OUT vout		= ( VS_OUT )0;\n"
				"	vout.pos		= mul( vin.pos, mul( cbWorld, cbViewProj ) );\n"
				"	vout.wsPos		= mul( vin.pos, cbWorld );\n"
				"	vout.normal		= normalize( mul( vin.normal, cbWorld ) );\n"
				"	vout.texCoord	= vin.texCoord;\n"
				"	return vout;\n"
				"}\n"
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
			static constexpr const char *StaticNameVS	= "Donya::DefaultModelStaticVS";
			static constexpr const char *StaticNamePS	= "Donya::DefaultModelStaticPS";
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
			standard.CullMode				= D3D11_CULL_FRONT;
			standard.FrontCounterClockwise	= FALSE;
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

		std::unique_ptr<ModelRenderer::Default::Member> ModelRenderer::Default::pMember = nullptr;
		bool ModelRenderer::Default::Initialize( ID3D11Device *pDevice )
		{
			// Already initialized.
			if ( pMember ) { return true; }
			// else

			bool result		= true;
			bool succeeded	= true;
			pMember = std::make_unique<ModelRenderer::Default::Member>();

			result = AssignStatusIdentifiers( pDevice );
			if ( !result ) { succeeded = false; }
			result = CreateRenderingStates  ( pDevice );
			if ( !result ) { succeeded = false; }

			result = pMember->CBPerScene.Create();
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

		bool ModelRenderer::Default::AssignStatusIdentifiers( ID3D11Device *pDevice )
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
		bool ModelRenderer::Default::CreateRenderingStates  ( ID3D11Device *pDevice )
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
		bool ModelRenderer::Default::CreateDefaultShaders   ( ID3D11Device *pDevice )
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
				result = pMember->shaderSkinning.VS.CreateByCSO
				(
					"./Data/Shader/SourceSkinnedMeshVS.cso",
					MakeInputElementsSkinned(),
					pDevice
				);
				/*
				result = pMember->shaderSkinned.VS.CreateByEmbededSourceCode
				(
					Source::SkinnedNameVS, Source::SkinnedCode(), Source::EntryPointVS,
					MakeInputElementsSkinned(),
					pDevice
				);
				*/
				if ( !result )
				{
					AssertFailedCreation( L"Skinned_VS" );
					succeeded = false;
				}

				result = pMember->shaderSkinning.PS.CreateByCSO
				(
					"./Data/Shader/SourceSkinnedMeshPS.cso", pDevice
				);
				/*
				result = pMember->shaderSkinned.PS.CreateByEmbededSourceCode
				(
					Source::SkinnedNamePS, Source::SkinnedCode(), Source::EntryPointPS,
					pDevice
				);
				*/
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
					Source::StaticNamePS, Source::StaticCode(), Source::EntryPointPS,
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

		bool ModelRenderer::Default::ActivateDepthStencil( ID3D11DeviceContext *pImmediateContext )
		{
			return Donya::DepthStencil::Activate( pMember->idDSState, pImmediateContext );
		}
		bool ModelRenderer::Default::ActivateRasterizer( ID3D11DeviceContext *pImmediateContext )
		{
			return Donya::Rasterizer::Activate( pMember->idRSState, pImmediateContext );
		}
		bool ModelRenderer::Default::ActivateSampler( const RegisterDesc &setting, ID3D11DeviceContext *pImmediateContext )
		{
			return Donya::Sampler::Activate( pMember->idPSSampler, setting.setSlot, setting.setVS, setting.setPS, pImmediateContext );
		}
		void ModelRenderer::Default::DeactivateDepthStencil( ID3D11DeviceContext *pImmediateContext )
		{
			Donya::DepthStencil::Deactivate( pImmediateContext );
		}
		void ModelRenderer::Default::DeactivateRasterizer( ID3D11DeviceContext *pImmediateContext )
		{
			Donya::Rasterizer::Deactivate( pImmediateContext );
		}
		void ModelRenderer::Default::DeactivateSampler( ID3D11DeviceContext *pImmediateContext )
		{
			Donya::Sampler::Deactivate( pImmediateContext );
		}

		void ModelRenderer::Default::ActivateVertexShaderSkinning( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderSkinning.VS.Activate( pImmediateContext );
		}
		void ModelRenderer::Default::ActivatePixelShaderSkinning( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderSkinning.PS.Activate( pImmediateContext );
		}
		void ModelRenderer::Default::ActivateVertexShaderStatic( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderStatic.VS.Activate( pImmediateContext );
		}
		void ModelRenderer::Default::ActivatePixelShaderStatic( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderStatic.PS.Activate( pImmediateContext );
		}
		void ModelRenderer::Default::DeactivateVertexShaderSkinning( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderSkinning.VS.Deactivate( pImmediateContext );
		}
		void ModelRenderer::Default::DeactivatePixelShaderSkinning( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderSkinning.PS.Deactivate( pImmediateContext );
		}
		void ModelRenderer::Default::DeactivateVertexShaderStatic( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderStatic.VS.Deactivate( pImmediateContext );
		}
		void ModelRenderer::Default::DeactivatePixelShaderStatic( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->shaderStatic.PS.Deactivate( pImmediateContext );
		}

		void ModelRenderer::Default::UpdateSceneConstants( const Constants::PerScene::Common &param )
		{
			pMember->CBPerScene.data = param;
		}
		void ModelRenderer::Default::ActivateSceneConstants( const RegisterDesc &setting, ID3D11DeviceContext *pImmediateContext )
		{
			pMember->CBPerScene.Activate( setting.setSlot, setting.setVS, setting.setPS, pImmediateContext );
		}
		void ModelRenderer::Default::DeactivateSceneConstants( ID3D11DeviceContext *pImmediateContext )
		{
			pMember->CBPerScene.Deactivate( pImmediateContext );
		}

		std::vector<D3D11_INPUT_ELEMENT_DESC> ModelRenderer::GetInputElementDescs( ModelUsage usage )
		{
			switch ( usage )
			{
			case ModelUsage::Static:
				return MakeInputElementsStatic();
			case ModelUsage::Skinned:
				return MakeInputElementsSkinned();
			default:
				_ASSERT_EXPR( 0, L"Error : That model-usage is not supported!" );
				break;
			}
			return std::vector<D3D11_INPUT_ELEMENT_DESC>{};
		}

		ModelRenderer::ModelRenderer( ModelUsage usage, ID3D11Device *pDevice ) :
			inputUsage( usage ), CBPerModel(), pCBPerMesh( nullptr ), CBPerSubset()
		{
			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			try
			{
				auto ThrowRuntimeError = []( const std::string &constantsName )
				{
					const std::string errMsg =
						"Exception : Creation of a constant-buffer per " + constantsName + " is failed.";
					throw std::runtime_error{ errMsg };
				};

				bool succeeded = true;

				succeeded = CBPerModel.Create();
				if ( !succeeded )
				{
					ThrowRuntimeError( "model" );
				}
				succeeded = AssignSpecifiedCBuffer( usage, pDevice );
				if ( !succeeded )
				{
					ThrowRuntimeError( "mesh" );
				}
				succeeded = CBPerSubset.Create();
				if ( !succeeded )
				{
					ThrowRuntimeError( "subset" );
				}
			}
			catch ( ... )
			{
				exit( EXIT_FAILURE );
			}
		}

		bool ModelRenderer::AssignSpecifiedCBuffer( ModelUsage usage, ID3D11Device *pDevice )
		{
			using namespace Strategy;

			switch ( usage )
			{
			case ModelUsage::Static:
				pCBPerMesh = std::make_shared<StaticConstantsPerMesh>();
				return pCBPerMesh->CreateBuffer( pDevice );
			case ModelUsage::Skinned:
				pCBPerMesh = std::make_shared<SkinnedConstantsPerMesh>();
				return pCBPerMesh->CreateBuffer( pDevice );
			default:
				_ASSERT_EXPR( 0, L"Error : That model-usage is not supported!" );
				return false;
			}
			return false;
		}

		void ModelRenderer::ActivateModelConstants( const Constants::PerModel::Common &input, const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			CBPerModel.data = input;
			CBPerModel.Activate( desc.setSlot, desc.setVS, desc.setPS, pImmediateContext );
		}
		void ModelRenderer::DeactivateModelConstants( ID3D11DeviceContext *pImmediateContext ) const
		{
			CBPerModel.Deactivate( pImmediateContext );
		}

		// HACK : These render method can optimize. There is many copy-paste :(

		bool ModelRenderer::RenderSkinned( const Model &model, const FocusMotion &activeMotion, const Animator &animator, const RegisterDesc &descMesh, const RegisterDesc &descSubset, const RegisterDesc &descDiffuse, ID3D11DeviceContext *pImmediateContext )
		{
			if ( !EnableSkinned() ) { return false; }
			// else

			if ( !pImmediateContext )
			{
				pImmediateContext = Donya::GetImmediateContext();
			}

			pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

			const size_t meshCount = model.meshes.size();
			for ( size_t i = 0; i < meshCount; ++i )
			{
				const auto &mesh = model.meshes[i];
				
				UpdateConstantsPerMeshSkinned( model, i, activeMotion, descMesh, pImmediateContext );
				ActivateCBPerMesh( descMesh, pImmediateContext );

				mesh.pVertex->SetVertexBuffers( pImmediateContext );
				pImmediateContext->IASetIndexBuffer( mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );

				const size_t subsetCount = mesh.subsets.size();
				for ( size_t j = 0; j < subsetCount; ++j )
				{
					const auto &subset = mesh.subsets[j];

					UpdateConstantsPerSubset( model, i, j, descSubset, pImmediateContext );
					ActivateCBPerSubset( descSubset, pImmediateContext );

					SetTexture( descDiffuse, subset.diffuse.pSRV.GetAddressOf(), pImmediateContext );

					pImmediateContext->DrawIndexed( subset.indexCount, subset.indexStart, 0 );

					ResetTexture( descDiffuse, pImmediateContext );
					DeactivateCBPerSubset( pImmediateContext );
				}

				DeactivateCBPerMesh( pImmediateContext );
			}

			return true;
		}
		bool ModelRenderer::RenderStatic( const Model &model, const RegisterDesc &descMesh, const RegisterDesc &descSubset, const RegisterDesc &descDiffuse, ID3D11DeviceContext *pImmediateContext )
		{
			if ( EnableSkinned() ) { return false; }
			// else

			if ( !pImmediateContext )
			{
				pImmediateContext = Donya::GetImmediateContext();
			}

			pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

			const size_t meshCount = model.meshes.size();
			for ( size_t i = 0; i < meshCount; ++i )
			{
				const auto &mesh = model.meshes[i];
				UpdateConstantsPerMeshStatic( model, i, descMesh, pImmediateContext );
				ActivateCBPerMesh( descMesh, pImmediateContext );

				mesh.pVertex->SetVertexBuffers( pImmediateContext );
				pImmediateContext->IASetIndexBuffer( mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );

				const size_t subsetCount = mesh.subsets.size();
				for ( size_t j = 0; j < subsetCount; ++j )
				{
					const auto &subset = mesh.subsets[j];
					UpdateConstantsPerSubset( model, i, j, descSubset, pImmediateContext );

					SetTexture( descDiffuse, subset.diffuse.pSRV.GetAddressOf(), pImmediateContext );

					pImmediateContext->DrawIndexed( subset.indexCount, subset.indexStart, 0 );

					ResetTexture( descDiffuse, pImmediateContext );
				}
			}

			return true;
		}

		bool ModelRenderer::EnableSkinned()
		{
			switch ( inputUsage )
			{
			case ModelUsage::Static:  return false;
			case ModelUsage::Skinned: return true;
			default:
				_ASSERT_EXPR( 0, L"Error : That model-usage is not supported!" );
				break;
			}
			return false;
		}

		Constants::PerMesh::Common ModelRenderer::MakeConstantsCommon( const Model &model, size_t meshIndex ) const
		{
			const auto &mesh = model.meshes[meshIndex];
			Constants::PerMesh::Common constants{};

			constants.adjustMatrix = mesh.globalTransform * mesh.coordinateConversion;

			return constants;
		}
		Constants::PerMesh::Bone   ModelRenderer::MakeConstantsBone  ( const Model &model, size_t meshIndex, const FocusMotion &activeMotion ) const
		{
			const auto &mesh		= model.meshes[meshIndex];
			const auto &currentPose	= activeMotion.GetCurrentSkeletal();
			Constants::PerMesh::Bone constants{};

			Donya::Vector4x4 meshToBone{}; // So-called "Bone offset matrix".
			Donya::Vector4x4 boneToMesh{}; // Transform to mesh space of current pose.
			const size_t boneCount = std::min( mesh.boneIndices.size(), scast<size_t>( Constants::PerMesh::Bone::MAX_BONE_COUNT ) );
			for ( size_t i = 0; i < boneCount; ++i )
			{
				size_t poseIndex = mesh.boneIndices[i]; // This index was fetched with boneOffset's name.
				meshToBone = mesh.boneOffsets[i].transform.ToWorldMatrix();
				boneToMesh = currentPose[poseIndex].global;
				
				constants.boneTransforms[i] = meshToBone * boneToMesh;
			}

			return constants;
		}
		void ModelRenderer::UpdateConstantsPerMeshSkinned( const Model &model, size_t meshIndex, const FocusMotion &activeMotion, const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			Constants::PerMesh::Common constantsCommon = MakeConstantsCommon( model, meshIndex );
			Constants::PerMesh::Bone   constantsBone   = MakeConstantsBone  ( model, meshIndex, activeMotion );

			pCBPerMesh->Update( constantsCommon, constantsBone );
		}
		void ModelRenderer::UpdateConstantsPerMeshStatic ( const Model &model, size_t meshIndex, const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			Constants::PerMesh::Common constantsCommon = MakeConstantsCommon( model, meshIndex );
		
			pCBPerMesh->Update( constantsCommon );
		}
		void ModelRenderer::ActivateCBPerMesh( const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			pCBPerMesh->Activate( desc.setSlot, desc.setVS, desc.setPS, pImmediateContext );
		}
		void ModelRenderer::DeactivateCBPerMesh( ID3D11DeviceContext *pImmediateContext )
		{
			pCBPerMesh->Deactivate( pImmediateContext );
		}

		void ModelRenderer::UpdateConstantsPerSubset( const Donya::Model::Model &model, size_t meshIndex, size_t subsetIndex, const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			const auto &mesh   = model.meshes[meshIndex];
			const auto &subset = mesh.subsets[subsetIndex];

			Constants::PerSubset::Common constants{};
			constants.ambient  = subset.ambient.color;
			constants.diffuse  = subset.diffuse.color;
			constants.specular = subset.specular.color;
			
			CBPerSubset.data   = constants;
		}
		void ModelRenderer::ActivateCBPerSubset( const RegisterDesc &desc, ID3D11DeviceContext *pImmediateContext )
		{
			CBPerSubset.Activate( desc.setSlot, desc.setVS, desc.setPS, pImmediateContext );
		}
		void ModelRenderer::DeactivateCBPerSubset( ID3D11DeviceContext *pImmediateContext )
		{
			CBPerSubset.Deactivate( pImmediateContext );
		}

		void ModelRenderer::SetTexture( const RegisterDesc &descDiffuse, SRVType diffuseSRV, ID3D11DeviceContext *pImmediateContext ) const
		{
			if ( descDiffuse.setVS )
			{
				pImmediateContext->VSSetShaderResources( descDiffuse.setSlot, 1U, diffuseSRV );
			}
			if ( descDiffuse.setPS )
			{
				pImmediateContext->PSSetShaderResources( descDiffuse.setSlot, 1U, diffuseSRV );
			}
		}
		void ModelRenderer::ResetTexture( const RegisterDesc &descDiffuse, ID3D11DeviceContext *pImmediateContext ) const
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
	}
}
