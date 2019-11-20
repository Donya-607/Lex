#include "Blend.h"

#include <assert.h>
#include <D3D11.h>
#include <unordered_map>
#include <wrl.h>

#include "Constant.h"
#include "Donya.h"
#include "Sprite.h"

constexpr D3D11_RENDER_TARGET_BLEND_DESC GetRTBlendDesc( Donya::Blend::Mode mode )
{
	/*
	The D3D11_BLEND constant is multiply to a member.
	ex)
	SrcBlend  = D3D11_BLEND_ONE;			-> src.rgb  * 1.0
	DestBlend = D3D11_BLEND_INV_SRC_COLOR;	-> dest.rgb * ( 1.0f - src.rgb )
	*/

	D3D11_RENDER_TARGET_BLEND_DESC desc{};
	switch ( mode )
	{
	case Donya::Blend::Mode::NO_BLEND:
		desc.BlendEnable			= FALSE;
		desc.SrcBlend				= D3D11_BLEND_ONE;
		desc.DestBlend				= D3D11_BLEND_ZERO;
		desc.BlendOp				= D3D11_BLEND_OP_ADD;
		desc.SrcBlendAlpha			= D3D11_BLEND_ONE;
		desc.DestBlendAlpha			= D3D11_BLEND_ZERO;
		desc.BlendOpAlpha			= D3D11_BLEND_OP_ADD;
		desc.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
		return desc; // break;
	case Donya::Blend::Mode::ALPHA: // [[fallthrough]];
	case Donya::Blend::Mode::ALPHA_NO_ATC:
		desc.BlendEnable			= TRUE;
		desc.SrcBlend				= D3D11_BLEND_SRC_ALPHA;
		desc.DestBlend				= D3D11_BLEND_INV_SRC_ALPHA;
		desc.BlendOp				= D3D11_BLEND_OP_ADD;
		desc.SrcBlendAlpha			= D3D11_BLEND_ONE;
		desc.DestBlendAlpha			= D3D11_BLEND_INV_SRC_ALPHA;
		desc.BlendOpAlpha			= D3D11_BLEND_OP_ADD;
		desc.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
		return desc; // break;
	case Donya::Blend::Mode::ADD: // [[fallthrough]];
	case Donya::Blend::Mode::ADD_NO_ATC:
		desc.BlendEnable			= TRUE;
		desc.SrcBlend				= D3D11_BLEND_SRC_ALPHA;
		desc.DestBlend				= D3D11_BLEND_ONE;
		desc.BlendOp				= D3D11_BLEND_OP_ADD;
		desc.SrcBlendAlpha			= D3D11_BLEND_ZERO;
		desc.DestBlendAlpha			= D3D11_BLEND_ONE;
		desc.BlendOpAlpha			= D3D11_BLEND_OP_ADD;
		desc.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
		return desc; // break;
	case Donya::Blend::Mode::SUB: // [[fallthrough]];
	case Donya::Blend::Mode::SUB_NO_ATC:
		desc.BlendEnable			= TRUE;
		desc.SrcBlend				= D3D11_BLEND_ZERO;
		desc.DestBlend				= D3D11_BLEND_INV_SRC_COLOR;
		desc.BlendOp				= D3D11_BLEND_OP_ADD;
		desc.SrcBlendAlpha			= D3D11_BLEND_ZERO;
		desc.DestBlendAlpha			= D3D11_BLEND_ONE;
		desc.BlendOpAlpha			= D3D11_BLEND_OP_ADD;
		desc.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
		return desc; // break;
	case Donya::Blend::Mode::MUL: // [[fallthrough]];
	case Donya::Blend::Mode::MUL_NO_ATC:
		desc.BlendEnable			= TRUE;
		desc.SrcBlend				= D3D11_BLEND_ZERO;
		desc.DestBlend				= D3D11_BLEND_SRC_COLOR;
		desc.BlendOp				= D3D11_BLEND_OP_ADD;
		desc.SrcBlendAlpha			= D3D11_BLEND_DEST_ALPHA;
		desc.DestBlendAlpha			= D3D11_BLEND_ZERO;
		desc.BlendOpAlpha			= D3D11_BLEND_OP_ADD;
		desc.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
		return desc; // break;
	case Donya::Blend::Mode::SCREEN: // [[fallthrough]];
	case Donya::Blend::Mode::SCREEN_NO_ATC:
		desc.BlendEnable			= TRUE;
		desc.SrcBlend				= D3D11_BLEND_SRC_ALPHA;
		desc.DestBlend				= D3D11_BLEND_INV_SRC_COLOR;
		desc.BlendOp				= D3D11_BLEND_OP_ADD;
		desc.SrcBlendAlpha			= D3D11_BLEND_ONE;
		desc.DestBlendAlpha			= D3D11_BLEND_INV_SRC_ALPHA;
		desc.BlendOpAlpha			= D3D11_BLEND_OP_ADD;
		desc.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
		return desc; // break;
	case Donya::Blend::Mode::LIGHTEN: // [[fallthrough]];
	case Donya::Blend::Mode::LIGHTEN_NO_ATC:
		desc.BlendEnable			= TRUE;
		desc.SrcBlend				= D3D11_BLEND_ONE;
		desc.DestBlend				= D3D11_BLEND_ONE;
		desc.BlendOp				= D3D11_BLEND_OP_MAX;
		desc.SrcBlendAlpha			= D3D11_BLEND_ONE;
		desc.DestBlendAlpha			= D3D11_BLEND_ONE;
		desc.BlendOpAlpha			= D3D11_BLEND_OP_MAX;
		desc.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
		return desc; // break;
	case Donya::Blend::Mode::DARKEN: // [[fallthrough]];
	case Donya::Blend::Mode::DARKEN_NO_ATC:
		desc.BlendEnable			= TRUE;
		desc.SrcBlend				= D3D11_BLEND_ONE;
		desc.DestBlend				= D3D11_BLEND_ONE;
		desc.BlendOp				= D3D11_BLEND_OP_MIN;
		desc.SrcBlendAlpha			= D3D11_BLEND_ONE;
		desc.DestBlendAlpha			= D3D11_BLEND_ONE;
		desc.BlendOpAlpha			= D3D11_BLEND_OP_MIN;
		desc.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
		return desc; // break;
	default: _ASSERT_EXPR( 0, L"Error : Unexpected blend mode detected in internal implementation !" ); break;
	}

	return desc;
}

namespace Donya
{
	namespace Blend
	{
		static std::unordered_map<Blend::Mode, Microsoft::WRL::ComPtr<ID3D11BlendState>>
			blendStateMap{};
		static std::unordered_map<int, Microsoft::WRL::ComPtr<ID3D11BlendState>>
			externalBlendStateMap{};

		/// <summary>
		/// Returns false when failed the creation.
		/// </summary>
		bool CreateBlendState( ID3D11Device *pDevice, Blend::Mode identifier, bool enableAlphaToCoverage )
		{
			auto find =  blendStateMap.find( identifier );
			if ( find != blendStateMap.end() ) { return true; }
			// else

			// Use default(library) device.
			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			D3D11_BLEND_DESC desc{};
			desc.AlphaToCoverageEnable	= ( enableAlphaToCoverage ) ? TRUE : FALSE;
			desc.IndependentBlendEnable	= FALSE;
			desc.RenderTarget[0]		= GetRTBlendDesc( identifier );
			
			Microsoft::WRL::ComPtr<ID3D11BlendState> tmpBlendObject{};

			HRESULT hr = S_OK;
			pDevice->CreateBlendState
			(
				&desc,
				tmpBlendObject.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, "Failed : Create internal blend state." );
				return false;
			}
			// else

			blendStateMap.insert
			(
				std::make_pair
				(
					identifier,
					tmpBlendObject
				)
			);

			return true;
		}

		bool Init( ID3D11Device *pDevice )
		{
			blendStateMap.clear();
			externalBlendStateMap.clear();

			constexpr Mode createList[]
			{
				Mode::NO_BLEND,
				Mode::ALPHA,
				Mode::ADD,
				Mode::SUB,
				Mode::MUL,
				Mode::SCREEN,
				Mode::LIGHTEN,
				Mode::DARKEN
			};
			constexpr Mode createNoATCList[]
			{
				Mode::ALPHA_NO_ATC,
				Mode::ADD_NO_ATC,
				Mode::SUB_NO_ATC,
				Mode::MUL_NO_ATC,
				Mode::SCREEN_NO_ATC,
				Mode::LIGHTEN_NO_ATC,
				Mode::DARKEN_NO_ATC
			};

			// Use default(library) device.
			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			bool result = false, succeeded = true;
			for ( const auto &it : createList )
			{
				result = CreateBlendState( pDevice, it, /* enableAlphaToCoverage */ true );
				if ( !result ) { succeeded = false; }
			}
			for ( const auto &it : createNoATCList )
			{
				result = CreateBlendState( pDevice, it, /* enableAlphaToCoverage */ false );
				if ( !result ) { succeeded = false; }
			}

			return succeeded;
		}

		bool CreateExternalBlendState( const D3D11_BLEND_DESC &userBlendDesc, int identifier, bool enableAlphaToCoverage, ID3D11Device *pDevice )
		{
			auto find =  externalBlendStateMap.find( identifier );
			if ( find != externalBlendStateMap.end() ) { return true; }
			// else

			// Use default(library) device.
			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			Microsoft::WRL::ComPtr<ID3D11BlendState> tmpBlendObject{};

			HRESULT hr = S_OK;
			pDevice->CreateBlendState
			(
				&userBlendDesc,
				tmpBlendObject.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, "Failed : Create internal blend state." );
				return false;
			}
			// else

			externalBlendStateMap.insert
			(
				std::make_pair
				(
					identifier,
					tmpBlendObject
				)
			);

			return true;
		}

		/// <summary>
		/// Should call this before Activate().
		/// </summary>
		void PreProcess()
		{
			Donya::Sprite::Flush();
		}

		void Activate( Blend::Mode blendMode, ID3D11Device *pDevice )
		{
			if ( blendMode == Blend::Mode::BLEND_MODE_COUNT ) { return; }
			// else

			// TODO : Prevent to set to same state.

			auto blendState =  blendStateMap.find( blendMode );
			if ( blendState == blendStateMap.end() ) { return; }
			// else

			// Use default(library) device.
			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			PreProcess();

			constexpr FLOAT BLEND_FACTORS[]	= { 0.0f, 0.0f, 0.0f, 0.0f }; // RGBA
			constexpr UINT  SAMPLE_MASK		= 0xFFFFFFFF; // A:FF, R:FF, G:FF, B:FF. LDR:Low Dynamic Range
			Donya::GetImmediateContext()->OMSetBlendState
			(
				blendState->second.Get(),
				BLEND_FACTORS,
				SAMPLE_MASK
			);
		}
		
		bool ActivateExternal( int identifier, ID3D11Device *pDevice )
		{
			// TODO : Prevent to set to same state.

			auto blendState =  externalBlendStateMap.find( identifier );
			if ( blendState == externalBlendStateMap.end() ) { return false; }
			// else

			// Use default(library) device.
			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			PreProcess();

			constexpr FLOAT BLEND_FACTORS[]	= { 0.0f, 0.0f, 0.0f, 0.0f }; // RGBA
			constexpr UINT  SAMPLE_MASK		= 0xFFFFFFFF; // A:FF, R:FF, G:FF, B:FF. LDR:Low Dynamic Range
			Donya::GetImmediateContext()->OMSetBlendState
			(
				blendState->second.Get(),
				BLEND_FACTORS,
				SAMPLE_MASK
			);

			return true;
		}
	}
}