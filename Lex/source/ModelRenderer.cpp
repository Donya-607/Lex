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

		result = status.CBPerModel.Create();
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

	ModelRenderer::ModelRenderer( Donya::ModelUsage usage, ID3D11Device *pDevice ) :
		pCBPerMesh( nullptr ), CBPerSubset()
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
