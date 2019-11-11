#ifndef INCLUDED_DONYA_C_BUFFER_H_
#define INCLUDED_DONYA_C_BUFFER_H_

#include <d3d11.h>
#include <wrl.h>

#include "Donya.h"
#include "Direct3DUtil.h"	// Use CreateConstantBuffer().

namespace Donya
{
	/// <summary>
	/// Constant Buffer.<para></para>
	/// Use UpdateSubresource() with the public-member of "data" at Activate(). so you must updates of members before call Activate().
	/// </summary>
	template<class ContainData>
	class CBuffer
	{
	public:
		ContainData data; // Data of template-types.
	private:
		bool			wasCreated; // If create failed, this will false.
		unsigned int	usingSlot;
		Microsoft::WRL::ComPtr<ID3D11Buffer> iBuffer;
	public:
		CBuffer() : data(), wasCreated( false ), usingSlot(), iBuffer() {}
		virtual ~CBuffer() = default;

		CBuffer( const CBuffer & ) = delete;
		const CBuffer &operator = ( const CBuffer & ) = delete;
	public:
		/// <summary>
		/// If the "pDevice" is null, use default(library's) device.<para></para>
		/// Returns true if succeeded to create or already created.
		/// </summary>
		bool Create( ID3D11Device *pDevice = nullptr )
		{
			if ( wasCreated ) { return true; }
			// else

			constexpr size_t TEMPLATE_TYPE_SIZE = sizeof( ContainData );
			_ASSERT_EXPR( ( TEMPLATE_TYPE_SIZE % 16 ) == 0, L"Warning : Constant-buffer's size should be 16-byte aligned !" );

			// Use default device.
			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			HRESULT hr = S_OK;
			hr = Donya::CreateConstantBuffer
			(
				pDevice,
				TEMPLATE_TYPE_SIZE,
				iBuffer.GetAddressOf()
			);

			if ( FAILED( hr ) )
			{
				wasCreated = false;
				_ASSERT_EXPR( 0, L"Failed : Create Constant Buffer !" );
				return false;
			}
			// else

			wasCreated = true;
			return true;
		}
		/// <summary>
		/// Use UpdateSubresource(). so you must updates of members before call this.<para></para>
		/// If the "pImmediateContext" is null, use default(library's) device.<para></para>
		/// If setXX is false, setting null-object.
		/// </summary>
		void Activate( unsigned int setSlot, bool setVS, bool setPS, ID3D11DeviceContext *pImmediateContext = nullptr )
		{
			// Use default context.
			if ( !pImmediateContext )
			{
				pImmediateContext = Donya::GetImmediateContext();
			}

			pImmediateContext->UpdateSubresource( iBuffer.Get(), 0, nullptr, &data, 0, 0 );
			
			usingSlot = setSlot;

			ID3D11Buffer *nullBuffer{};
			pImmediateContext->VSSetConstantBuffers
			(
				usingSlot,
				1,
				( setVS )
				? iBuffer.GetAddressOf()
				: &nullBuffer
			);
			pImmediateContext->PSSetConstantBuffers
			(
				usingSlot,
				1,
				( setPS )
				? iBuffer.GetAddressOf()
				: &nullBuffer
			);
		}
		/// <summary>
		/// If the "pImmediateContext" is null, use default(library's) device.<para></para>
		/// </summary>
		void Deactivate( ID3D11DeviceContext *pImmediateContext = nullptr )
		{
			// Use default context.
			if ( !pImmediateContext )
			{
				pImmediateContext = Donya::GetImmediateContext();
			}

			ID3D11Buffer *nullBuffer{};
			pImmediateContext->VSSetConstantBuffers( usingSlot, 1, &nullBuffer );
			pImmediateContext->PSSetConstantBuffers( usingSlot, 1, &nullBuffer );
		}
	};
}

#endif // !INCLUDED_DONYA_C_BUFFER_H_
