#include "Donya.h"

#include <d3d11.h>
#include <memory>
#include <roapi.h>
#include <Windows.h>
#include <Windows.Foundation.h>
#include <wrl.h>

#pragma comment( lib, "runtimeobject.lib" )

namespace Donya
{
	struct SystemMemberGathering
	{
		HWND hwnd;
		Microsoft::WRL::ComPtr<ID3D11Device>			device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>		immediateContext;
		// Microsoft::WRL::ComPtr<IDXGISwapChain>			swapChain;
		// Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	renderTargetView;
		// Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	depthStencilView;
	};
	static std::unique_ptr<SystemMemberGathering> smg{};

	void InitWindow( const char *windowCaption )
	{
		// This Function has not implemented yet.
	}

	bool Init( const char *windowCaption, bool isAppendFPS )
	{
		// This Function has not completely implemented yet.

		smg = std::make_unique<SystemMemberGathering>();

		HRESULT hr = S_OK;

		#pragma region Create ID3D11Device and ID3D11DeviceContext
		{
			// see https://docs.microsoft.com/en-us/windows/win32/api/roapi/nf-roapi-initialize

			hr = Windows::Foundation::Initialize( RO_INIT_MULTITHREADED );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Windows::Foundation::Initialize()" );
				exit( -1 );
				return false;
			}

			// This flag is required in order to enable compatibility with Direct2D.
			UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
		#if defined( _DEBUG )
			// If the project is in a debug build, enable debugging via SDK Layers with this flag.
			creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
		#endif

			// This array defines the ordering of feature levels that D3D should attempt to create.
			D3D_FEATURE_LEVEL featureLevels[] =
			{
				// D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_3,
				D3D_FEATURE_LEVEL_9_1
			};

			hr = D3D11CreateDevice
			(
				NULL,
				D3D_DRIVER_TYPE_HARDWARE,
				NULL,
				creationFlags,
				featureLevels,
				ARRAYSIZE( featureLevels ),
				D3D11_SDK_VERSION,
				smg->device.GetAddressOf(),
				nullptr,
				smg->immediateContext.GetAddressOf()
			);

			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : D3D11CreateDevice()" );
				exit( -1 );
				return false;
			}
		}
		#pragma endregion

		return true;
	}

	bool MessageLoop()
	{
		// This Function has not implemented yet.

		return false;
	}

	void Uninit()
	{
		// This Function has not completely implemented yet.
		smg.reset( nullptr );

		Windows::Foundation::Uninitialize();
	}

	ID3D11Device		*GetDevice()			{ return smg->device.Get(); }
	ID3D11DeviceContext	*GetImmediateContext()	{ return smg->immediateContext.Get(); }
}