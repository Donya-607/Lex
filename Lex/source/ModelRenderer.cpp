#include "ModelRenderer.h"

#include <exception>

#include "Donya/RenderingStates.h" // For default shading.

namespace Donya
{
	namespace Strategy
	{

	}

	bool ModelRenderer::InitDefaultStatus( ID3D11Device *pDevice )
	{
		// Already initialized.
		if ( pDefaultStatus ) { return; }
		// else

		DefaultStatus status{};


	}
	void ModelRenderer::AssignStatusIdentifiers( DefaultStatus *pStatus )
	{
		auto AssertFailedCreation	= []( const std::wstring &stateName )
		{
			const std::wstring expression = L"Failed : Create a sprite's state of ";
			_ASSERT_EXPR( 0, ( expression + stateName + L"." ).c_str() );
		};
		auto AssertNotFound			= []( const std::wstring &stateName )
		{
			const std::wstring expression = L"Unexpected Error : We can't found a space of a sprite's state of ";
			_ASSERT_EXPR( 0, ( expression + stateName + L"." ).c_str() );
		};

		constexpr int DEFAULT_ID	= 0;
		using FindFunction = std::function<bool( int )>;

		auto AssignStateIdentifier	= [&AssertNotFound, &DEFAULT_ID]( int *pIdentifier, const std::wstring &stateName, const FindFunction &IsAlreadyExists )
		{
			// Already assigned.
			if ( *pIdentifier != DEFAULT_ID ) { return; }
			// else

			*pIdentifier = DEFAULT_ID;

			// The internal object use minus value to identifier.
			for ( int i = -1; -INT_MAX < i; --i )
			{
				if ( IsAlreadyExists( i ) ) { continue; }
				// else

				*pIdentifier = i;
				break;
			}

			// If usable identifier was not found.
			if ( *pIdentifier == DEFAULT_ID )
			{
				AssertNotFound( stateName );
				return;
			}
			// else

		};

		constexpr int STATE_COUNT	= 3;
		std::array<int *, STATE_COUNT> pIdentifiers
		{
			&pStatus->idDSState,
			&pStatus->idRSState,
			&pStatus->idPSSampler,
		};
		std::array<FindFunction, STATE_COUNT> pIdentifiers
		{
			&Donya::DepthStencil::IsAlreadyExists,
			&pStatus->idRSState,
			&pStatus->idPSSampler,
		};

		if ( Instance::idDepthStencil == Instance::DEFAULT_ID )
		{
			for ( int i = -1; -INT_MAX < i; --i )
			{
				if ( Donya::DepthStencil::IsAlreadyExists( i ) ) { continue; }
				// else
				Instance::idDepthStencil = i;
				break;
			}
			if ( Instance::idDepthStencil == Instance::DEFAULT_ID )
			{
				AssertNotFound( L"DepthStencil" );
				return false;
			}
			// else
		}
	}

	ModelRenderer::ModelRenderer( Donya::ModelUsage usage, ID3D11Device *pDevice ) :
		pCBPerMesh( nullptr ), CBPerSubset()
	{
		try
		{
			auto ThrowRuntimeError = []( const std::string &constantsName )
			{
				const std::string errMsg =
					"Exception : Creation of a constant-buffer per " + constantsName + " is failed.";
				throw std::runtime_error{ errMsg };
			};

			bool  succeeded = AssignSpecifiedCBuffer( usage );
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

	bool ModelRenderer::AssignSpecifiedCBuffer( Donya::ModelUsage usage )
	{
		using namespace Strategy;

		switch ( usage )
		{
		case Donya::ModelUsage::Static:
			pCBPerMesh = IConstantsPerMesh::Create<StaticConstantsPerMesh>();
			return ( !pCBPerMesh ) ? false : true;
		case Donya::ModelUsage::Skinned:
			pCBPerMesh = IConstantsPerMesh::Create<SkinnedConstantsPerMesh>();
			return ( !pCBPerMesh ) ? false : true;
		default:
			_ASSERT_EXPR( 0, L"Error : That model-usage is not supported!" );
			return false;
		}
		return false;
	}
}
