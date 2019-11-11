#include "Shader.h"

#include "Donya.h"		// Use ID3D11Device getter.
#include "Resource.h"
#include "Useful.h"		// Use operate string method.

namespace Donya
{
	VertexShader::VertexShader() :
		wasCreated( false ),
		iInputLayout(), iVertexShader(),
		iDefaultInputLayout(), iDefaultVertexShader()
	{}
	VertexShader::~VertexShader() = default;

	bool VertexShader::CreateByCSO( const std::string &filePath, const std::vector<D3D11_INPUT_ELEMENT_DESC> &inputElements, bool isEnableCache, ID3D11Device *pDevice )
	{
		if ( wasCreated ) { return true; }
		// else

		if ( !Donya::IsExistFile( filePath ) ) { return false; }
		// else

		// Use default device.
		if ( !pDevice )
		{
			pDevice = Donya::GetDevice();
		}

		bool result =
		Donya::Resource::CreateVertexShaderFromCso
		(
			pDevice,
			filePath.c_str(), "rb",
			iVertexShader.GetAddressOf(),
			iInputLayout.GetAddressOf(),
			inputElements.data(),
			inputElements.size(),
			isEnableCache
		);

		if ( !result ) { return false; }
		// else

		wasCreated = true;
		return true;
	}

	void VertexShader::Activate( ID3D11DeviceContext *pContext ) const
	{
		if ( !wasCreated )
		{
			_ASSERT_EXPR( 0, L"Error : The vertex-shader was not created !" );
			return;
		}
		// else

		// Use default-context.
		if ( !pContext )
		{
			pContext = Donya::GetImmediateContext();
		}

		pContext->IAGetInputLayout( iDefaultInputLayout.GetAddressOf() );
		pContext->VSGetShader( iDefaultVertexShader.GetAddressOf(), 0, 0 );

		pContext->IASetInputLayout( iInputLayout.Get() );
		pContext->VSSetShader( iVertexShader.Get(), 0, 0 );
	}
	void VertexShader::Deactivate( ID3D11DeviceContext *pContext ) const
	{
		if ( !wasCreated )
		{
			_ASSERT_EXPR( 0, L"Error : The vertex-shader was not created !" );
			return;
		}
		// else

		// Use default-context.
		if ( !pContext )
		{
			pContext = Donya::GetImmediateContext();
		}

		pContext->IASetInputLayout( iDefaultInputLayout.Get() );
		pContext->VSSetShader( iDefaultVertexShader.Get(), 0, 0 );
	}

	PixelShader::PixelShader() :
		wasCreated( false ),
		iPixelShader(), iDefaultPixelShader()
	{}
	PixelShader::~PixelShader() = default;

	bool PixelShader::CreateByCSO( const std::string &filePath, bool isEnableCache, ID3D11Device *pDevice )
	{
		if ( wasCreated ) { return true; }
		// else

		if ( !Donya::IsExistFile( filePath ) ) { return false; }
		// else

		// Use default device.
		if ( !pDevice )
		{
			pDevice = Donya::GetDevice();
		}

		bool result =
		Donya::Resource::CreatePixelShaderFromCso
		(
			pDevice,
			filePath.c_str(), "rb",
			iPixelShader.GetAddressOf(),
			isEnableCache
		);

		if ( !result ) { return false; }
		// else

		wasCreated = true;
		return true;
	}

	void PixelShader::Activate( ID3D11DeviceContext *pContext ) const
	{
		if ( !wasCreated )
		{
			_ASSERT_EXPR( 0, L"Error : The vertex-shader was not created !" );
			return;
		}
		// else

		// Use default-context.
		if ( !pContext )
		{
			pContext = Donya::GetImmediateContext();
		}

		pContext->PSGetShader( iDefaultPixelShader.GetAddressOf(), 0, 0 );

		pContext->PSSetShader( iPixelShader.Get(), 0, 0 );
	}
	void PixelShader::Deactivate( ID3D11DeviceContext *pContext ) const
	{
		if ( !wasCreated )
		{
			_ASSERT_EXPR( 0, L"Error : The vertex-shader was not created !" );
			return;
		}
		// else

		// Use default-context.
		if ( !pContext )
		{
			pContext = Donya::GetImmediateContext();
		}

		pContext->PSSetShader( iDefaultPixelShader.Get(), 0, 0 );
	}
}
