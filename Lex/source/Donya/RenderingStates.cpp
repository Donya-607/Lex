#include "RenderingStates.h"

#include <d3d11.h>
#include <unordered_map>
#include <wrl.h>
#include <vector>

#include "Donya.h"			// For fetch the library's device and immediate-context.

using namespace Microsoft::WRL;

void SetDefaultDeviceIfNull( ID3D11Device **ppDevice )
{
	if ( *ppDevice != nullptr ) { return; }
	// else

	*ppDevice = Donya::GetDevice();
}
void SetDefaultImmediateContextIfNull( ID3D11DeviceContext **ppImmediateContext )
{
	if ( *ppImmediateContext != nullptr ) { return; }
	// else

	*ppImmediateContext = Donya::GetImmediateContext();
}

namespace Donya
{
	// The Blend-State is there at "Blend.h".

	namespace DepthStencil
	{
		static std::unordered_map<int, ComPtr<ID3D11DepthStencilState>> mapDepthStencil{};
		static ComPtr<ID3D11DepthStencilState> oldState{}; // Use for Deactivate().
		bool CreateState( int id, const D3D11_DEPTH_STENCIL_DESC &desc, ID3D11Device *pDevice )
		{
			if ( IsAlreadyExists( id ) ) { return true; }
			// else

			SetDefaultDeviceIfNull( &pDevice );

			HRESULT hr = S_OK;
			ComPtr<ID3D11DepthStencilState> tmpStateObject{};

			hr = pDevice->CreateDepthStencilState( &desc, tmpStateObject.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create internal depth stencil state." );
				return false;
			}
			// else

			mapDepthStencil.insert
			(
				std::make_pair
				(
					id,
					tmpStateObject
				)
			);

			return true;
		}

		bool IsAlreadyExists( int id )
		{
			const auto found =  mapDepthStencil.find( id );
			return   ( found != mapDepthStencil.end() );
		}

		constexpr unsigned int STENCIL_REF = 0xFFFFFFFF;
		bool Activate( int id, ID3D11DeviceContext *pImmediateContext )
		{
			auto found =  mapDepthStencil.find( id );
			if ( found == mapDepthStencil.end() ) { return false; }
			// else

			SetDefaultImmediateContextIfNull( &pImmediateContext );

			pImmediateContext->OMGetDepthStencilState( oldState.ReleaseAndGetAddressOf(), NULL );

			pImmediateContext->OMSetDepthStencilState( found->second.Get(), STENCIL_REF );

			return true;
		}
		void Deactivate( ID3D11DeviceContext *pImmediateContext )
		{
			SetDefaultImmediateContextIfNull( &pImmediateContext );
			pImmediateContext->OMSetDepthStencilState( oldState.Get(), STENCIL_REF );
		}

		void ReleaseAllCachedStates()
		{
			mapDepthStencil.clear();
		}
	}

	namespace Rasterizer
	{
		static std::unordered_map<int, ComPtr<ID3D11RasterizerState>> mapRasterizer{};
		static ComPtr<ID3D11RasterizerState> oldState{};

		bool CreateState( int id, const D3D11_RASTERIZER_DESC &desc, ID3D11Device *pDevice )
		{
			if ( IsAlreadyExists( id ) ) { return true; }
			// else

			SetDefaultDeviceIfNull( &pDevice );

			HRESULT hr = S_OK;
			ComPtr<ID3D11RasterizerState> tmpStateObject{};

			hr = pDevice->CreateRasterizerState( &desc, tmpStateObject.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create internal rasterizer state." );
				return false;
			}
			// else

			mapRasterizer.insert
			(
				std::make_pair
				(
					id,
					tmpStateObject
				)
			);

			return true;
		}

		bool IsAlreadyExists( int id )
		{
			const auto found =  mapRasterizer.find( id );
			return   ( found != mapRasterizer.end() );
		}

		bool Activate( int id, ID3D11DeviceContext *pImmediateContext )
		{
			auto found =  mapRasterizer.find( id );
			if ( found == mapRasterizer.end() ) { return false; }
			// else

			SetDefaultImmediateContextIfNull( &pImmediateContext );

			pImmediateContext->RSGetState( oldState.ReleaseAndGetAddressOf() );

			pImmediateContext->RSSetState( found->second.Get() );

			return true;
		}
		void Deactivate( ID3D11DeviceContext *pImmediateContext )
		{
			SetDefaultImmediateContextIfNull( &pImmediateContext );
			pImmediateContext->RSSetState( oldState.Get() );
		}

		void ReleaseAllCachedStates()
		{
			mapRasterizer.clear();
		}
	}

	namespace Sampler
	{
		struct Option
		{
			unsigned int slot{};
			bool setVS{};
			bool setPS{};
		};
		struct Configuration
		{
			ComPtr<ID3D11SamplerState> oldStateVS{};
			ComPtr<ID3D11SamplerState> oldStatePS{};
			Option op{};
		};
		static std::unordered_map<int, ComPtr<ID3D11SamplerState>> mapSampler{};
		static Configuration setConfig{};

		bool CreateState( int id, const D3D11_SAMPLER_DESC &desc, ID3D11Device *pDevice )
		{
			if ( IsAlreadyExists( id ) ) { return true; }
			// else

			SetDefaultDeviceIfNull( &pDevice );

			HRESULT hr = S_OK;
			ComPtr<ID3D11SamplerState> tmpStateObject{};

			hr = pDevice->CreateSamplerState( &desc, tmpStateObject.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create internal rasterizer state." );
				return false;
			}
			// else

			mapSampler.insert
			(
				std::make_pair
				(
					id,
					tmpStateObject
				)
			);

			return true;
		}

		bool IsAlreadyExists( int id )
		{
			const auto found =  mapSampler.find( id );
			return   ( found != mapSampler.end() );
		}

		void SetSampler( const Option &op, const ComPtr<ID3D11SamplerState> &samplerVS, const ComPtr<ID3D11SamplerState> &samplerPS, ID3D11DeviceContext *pImmediateContext )
		{
			SetDefaultImmediateContextIfNull( &pImmediateContext );

			if ( op.setVS )
			{
				pImmediateContext->VSSetSamplers( op.slot, 1, samplerVS.GetAddressOf() );
			}
			if ( op.setPS )
			{
				pImmediateContext->PSSetSamplers( op.slot, 1, samplerPS.GetAddressOf() );
			}
		}
		bool Activate( int id, unsigned int setSlot, bool setVS, bool setPS, ID3D11DeviceContext *pImmediateContext )
		{
			auto found =  mapSampler.find( id );
			if ( found == mapSampler.end() ) { return false; }
			// else

			SetDefaultImmediateContextIfNull( &pImmediateContext );

			setConfig.op.slot  = setSlot;
			setConfig.op.setVS = setVS;
			setConfig.op.setPS = setPS;

			if ( setConfig.op.setVS )
			{
				pImmediateContext->VSGetSamplers( setConfig.op.slot, 1, setConfig.oldStateVS.ReleaseAndGetAddressOf() );
			}
			if ( setConfig.op.setPS )
			{
				pImmediateContext->PSGetSamplers( setConfig.op.slot, 1, setConfig.oldStatePS.ReleaseAndGetAddressOf() );
			}
			
			SetSampler( setConfig.op, found->second, found->second, pImmediateContext );

			return true;
		}
		void Deactivate( ID3D11DeviceContext *pImmediateContext )
		{
			SetDefaultImmediateContextIfNull( &pImmediateContext );

			SetSampler( setConfig.op, setConfig.oldStateVS, setConfig.oldStatePS, pImmediateContext );
		}

		void ReleaseAllCachedStates()
		{
			mapSampler.clear();
		}
	}
}

