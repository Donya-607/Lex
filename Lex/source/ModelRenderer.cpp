#include "ModelRenderer.h"

#include <exception>
#include <tuple>

#include "Donya/Donya.h"			// GetDevice().
#include "Donya/RenderingStates.h"	// For default shading.

namespace Donya
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
			"	ApplyBoneMatrices( vin.weights, vin.bones, vin.pos, vin.normal );\n"

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
		standard.CullMode				= D3D11_CULL_BACK;
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
		const auto IEDescsPos = Donya::Vertex::Pos::InputElements();
		const auto IEDescsTex = Donya::Vertex::Tex::InputElements();

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
		const auto IEDescsBone = Donya::Vertex::Bone::InputElements();

		std::vector<D3D11_INPUT_ELEMENT_DESC> wholeIEDescs = MakeInputElementsStatic();
		wholeIEDescs.insert( wholeIEDescs.end(), IEDescsBone.begin(), IEDescsBone.end() );
		return wholeIEDescs;
	}

	std::unique_ptr<ModelRenderer::DefaultStatus> ModelRenderer::pDefaultStatus = nullptr;

	std::vector<D3D11_INPUT_ELEMENT_DESC> ModelRenderer::GetInputElementDescs( Donya::ModelUsage usage )
	{
		switch ( usage )
		{
		case Donya::ModelUsage::Static:
			return MakeInputElementsStatic();
		case Donya::ModelUsage::Skinned:
			return MakeInputElementsSkinned();
		default:
			_ASSERT_EXPR( 0, L"Error : That model-usage is not supported!" );
			break;
		}
		return std::vector<D3D11_INPUT_ELEMENT_DESC>{};
	}

	bool ModelRenderer::InitDefaultStatus( ID3D11Device *pDevice )
	{
		// Already initialized.
		if ( pDefaultStatus ) { return true; }
		// else

		bool result		= true;
		bool succeeded	= true;
		DefaultStatus status{};

		result = AssignStatusIdentifiers( &status );
		if ( !result ) { succeeded = false; }
		result = CreateRenderingStates  ( &status );
		if ( !result ) { succeeded = false; }

		result = status.CBPerScene.Create();
		if ( !result ) { succeeded = false; }

		result = CreateDefaultShaders   ( &status );
		if ( !result ) { succeeded = false; }

		if ( succeeded )
		{
			pDefaultStatus = std::make_unique<DefaultStatus>( std::move( status ) );
			return true;
		}
		// else

		pDefaultStatus = nullptr;
		return false;
	}
	bool ModelRenderer::AssignStatusIdentifiers( DefaultStatus *pStatus )
	{
		auto  AssertNotFound		= []( const std::wstring &stateName )
		{
			const std::wstring expression = L"Unexpected Error : We can't found a space of a sprite's state of ";
			_ASSERT_EXPR( 0, ( expression + stateName + L"." ).c_str() );
		};

		using FindFunction = std::function<bool( int )>;

		auto  AssignStateIdentifier	= [&AssertNotFound]( int *pIdentifier, const std::wstring &stateName, const FindFunction &IsAlreadyExists )
		{
			// Already assigned.
			if ( *pIdentifier != DefaultStatus::DEFAULT_ID ) { return true; }
			// else

			*pIdentifier = DefaultStatus::DEFAULT_ID;

			// The internal object use minus value to identifier.
			for ( int i = -1; -INT_MAX < i; --i )
			{
				if ( IsAlreadyExists( i ) ) { continue; }
				// else

				*pIdentifier = i;
				break;
			}

			// If usable identifier was not found.
			if ( *pIdentifier == DefaultStatus::DEFAULT_ID )
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
			std::make_tuple( &pStatus->idDSState,	L"DepthStencil",	Donya::DepthStencil::IsAlreadyExists	),
			std::make_tuple( &pStatus->idRSState,	L"Rasterizer",		Donya::Rasterizer::IsAlreadyExists		),
			std::make_tuple( &pStatus->idPSSampler,	L"Sampler",			Donya::Sampler::IsAlreadyExists			),
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
	bool ModelRenderer::CreateRenderingStates  ( DefaultStatus *pStatus )
	{
		auto AssertFailedCreation	= []( const std::wstring &stateName )
		{
			const std::wstring expression = L"Failed : Create a sprite's state of ";
			_ASSERT_EXPR( 0, ( expression + stateName + L"." ).c_str() );
		};

		auto descDS = DefaultDepthStencilDesc();
		auto descRS = DefaultRasterizerDesc();
		auto descPS = DefaultSamplerDesc();

		bool result		= true;
		bool succeeded	= true;

		if ( pStatus->idDSState != DefaultStatus::DEFAULT_ID )
		{
			result  = Donya::DepthStencil::CreateState( pStatus->idDSState, descDS );
			if ( !result )
			{
				AssertFailedCreation( L"DepthStencil" );
				succeeded = false;
			}
		}

		if ( pStatus->idRSState != DefaultStatus::DEFAULT_ID )
		{
			result = Donya::Rasterizer::CreateState( pStatus->idRSState, descRS );
			if ( !result )
			{
				AssertFailedCreation( L"Rasterizer" );
				succeeded = false;
			}
		}

		if ( pStatus->idPSSampler != DefaultStatus::DEFAULT_ID )
		{
			result = Donya::Sampler::CreateState( pStatus->idPSSampler, descPS );
			if ( !result )
			{
				AssertFailedCreation( L"Sampler" );
				succeeded = false;
			}
		}

		return succeeded;
	}
	bool ModelRenderer::CreateDefaultShaders   ( DefaultStatus *pStatus )
	{
		return false;
	}

	ModelRenderer::ModelRenderer( Donya::ModelUsage usage, ID3D11Device *pDevice ) :
		pCBPerMesh( nullptr ), CBPerModel(), CBPerSubset()
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

			bool  succeeded = AssignSpecifiedCBuffer( usage, pDevice );
			if ( !succeeded )
			{
				ThrowRuntimeError( "mesh" );
			}

			succeeded = CBPerModel.Create();
			if ( !succeeded )
			{
				ThrowRuntimeError( "model" );
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

	bool ModelRenderer::AssignSpecifiedCBuffer( Donya::ModelUsage usage, ID3D11Device *pDevice )
	{
		using namespace Strategy;

		switch ( usage )
		{
		case Donya::ModelUsage::Static:
			pCBPerMesh = std::make_shared<StaticConstantsPerMesh>();
			return pCBPerMesh->CreateBuffer( pDevice );
		case Donya::ModelUsage::Skinned:
			pCBPerMesh = std::make_shared<SkinnedConstantsPerMesh>();
			return pCBPerMesh->CreateBuffer( pDevice );
		default:
			_ASSERT_EXPR( 0, L"Error : That model-usage is not supported!" );
			return false;
		}
		return false;
	}
}
