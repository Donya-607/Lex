#include "Resource.h"

#include <D3D11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <tchar.h>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

#include "Constant.h"
#include "Donya.h"		// Use for GetDevice().
#include "Useful.h"

// This resolve un external symbol.
#pragma comment( lib, "d3dcompiler.lib" )

using namespace DirectX;
using namespace Microsoft::WRL;

bool IsContainStr( const std::wstring &str, const wchar_t *searchStr )
{
	return ( str.find( searchStr ) != std::wstring::npos ) ? true : false;
}

// Don't skip delim character.
void SkipUntilNextDelim( std::wstring *pStr, std::wstringstream *pSS, const wchar_t *delim = L" " )
{
	int disposal = pStr->find( delim );
	*pStr = pStr->substr( disposal, pStr->length() );

	pSS->clear();
	pSS->str( *pStr );
}

namespace Donya
{
	namespace Resource
	{
		/// <summary>
		/// Returns 0 if read the file failed.
		/// </summary>
		long ReadByteCode( std::unique_ptr<unsigned char[]> *pByteCode, std::string filePath, std::string openMode )
		{
			if ( !pByteCode ) { return NULL; }
			if ( !Donya::IsExistFile( filePath ) ) { return NULL; }
			// else

			FILE *fp = nullptr;

			fopen_s( &fp, filePath.c_str(), openMode.c_str() );
			if ( !fp ) { return NULL; }
			// else

			fseek( fp, 0, SEEK_END );
			long codeLength = ftell( fp );
			fseek( fp, 0, SEEK_SET );

			*pByteCode = std::make_unique<unsigned char[]>( codeLength );
			fread( pByteCode->get(), codeLength, 1, fp );

			fclose( fp );

			return codeLength;
		}

		#pragma region VerteShaderCache

		struct VertexShaderCacheContents
		{
			Microsoft::WRL::ComPtr<ID3D11VertexShader> d3dVertexShader;
			Microsoft::WRL::ComPtr<ID3D11InputLayout>  d3dInputLayout;
		public:
			VertexShaderCacheContents
			(
				ID3D11VertexShader	*pVertexShader,
				ID3D11InputLayout	*pInputLayout
			) :
				d3dVertexShader( pVertexShader ),
				d3dInputLayout( pInputLayout )
			{}
		};

		static std::unordered_map<std::string, VertexShaderCacheContents> vertexShaderCache{};
	
		bool CreateVertexShaderFromCso( ID3D11Device *d3dDevice, const char *csoname, const char *openMode, ID3D11VertexShader **d3dVertexShader, ID3D11InputLayout **d3dInputLayout, const D3D11_INPUT_ELEMENT_DESC *d3dInputElementsDescs, size_t inputElementDescSize, bool enableCache )
		{
			HRESULT hr = S_OK;

			std::string strCsoName = csoname;

			if ( !Donya::IsExistFile( strCsoName ) ) { return false; }
			// else

			auto it = vertexShaderCache.find( strCsoName );
			if ( it != vertexShaderCache.end() )
			{
				*d3dVertexShader = it->second.d3dVertexShader.Get();
				( *d3dVertexShader )->AddRef();

				if ( d3dInputLayout != nullptr )
				{
					*d3dInputLayout = it->second.d3dInputLayout.Get();
					_ASSERT_EXPR( *d3dInputLayout, L"cached InputLayout must be not Null." );
					( *d3dInputLayout )->AddRef();
				}

				return true;
			}
			// else

			std::unique_ptr<unsigned char[]> csoData{ nullptr };
			long csoSize = ReadByteCode( &csoData, strCsoName, openMode );
			_ASSERT_EXPR( 0 < csoSize, L"vs cso file not found" );

			hr = d3dDevice->CreateVertexShader
			(
				csoData.get(),
				csoSize,
				NULL,
				d3dVertexShader
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateVertexShader()" );
			
			if ( d3dInputLayout != nullptr )
			{
				hr = d3dDevice->CreateInputLayout
				(
					d3dInputElementsDescs,
					inputElementDescSize,
					csoData.get(),
					csoSize,
					d3dInputLayout
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateInputLayout()" ) );
			}

			if ( enableCache )
			{
				vertexShaderCache.insert
				(
					std::make_pair
					(
						strCsoName,
						VertexShaderCacheContents
						{
							*d3dVertexShader,
							( ( d3dInputLayout != nullptr ) ? *d3dInputLayout : nullptr )
						}
					)
				);
			}

			return true;
		}

		bool CreateVertexShaderFromSource( ID3D11Device *pDevice, const std::string &shaderId, const std::string &shaderCode, const std::string &shaderEntryPoint, ID3D11VertexShader **pOutVertexShader, ID3D11InputLayout **pOutInputLayout, const D3D11_INPUT_ELEMENT_DESC *pInputElementsDesc, size_t inputElementsCount, bool isEnableCache )
		{
			HRESULT hr = S_OK;

			auto it = vertexShaderCache.find( shaderId );
			if ( it != vertexShaderCache.end() )
			{
				*pOutVertexShader = it->second.d3dVertexShader.Get();
				( *pOutVertexShader )->AddRef();

				if ( pOutInputLayout != nullptr )
				{
					*pOutInputLayout = it->second.d3dInputLayout.Get();
					_ASSERT_EXPR( *pOutInputLayout, L"Cached InputLayout must be not Null." );
					( *pOutInputLayout )->AddRef();
				}

				return true;
			}
			// else

			DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;
		#if DEBUG_MODE
			flags |= D3DCOMPILE_DEBUG;
			flags |= D3D10_SHADER_SKIP_OPTIMIZATION;
		#endif

			Microsoft::WRL::ComPtr<ID3DBlob> compiledShaderBlob;
			Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

			// D3DCompile() : https://docs.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompile
			hr = D3DCompile
			(
				shaderCode.c_str(),
				shaderCode.length(),
				NULL,
				NULL,
				NULL,
				shaderEntryPoint.c_str(),
				"vs_5_0",
				flags,
				NULL,
				compiledShaderBlob.GetAddressOf(),
				errorBlob.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				auto errorStr = errorBlob->GetBufferPointer();
				OutputDebugStringA( ( char * )( errorStr ) );
				_ASSERT_EXPR( 0, ( wchar_t * )( errorStr ) );
				return false;
			}
			// else

			hr = pDevice->CreateVertexShader
			(
				compiledShaderBlob->GetBufferPointer(),
				compiledShaderBlob->GetBufferSize(),
				0,
				pOutVertexShader
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, reinterpret_cast<LPWSTR>( errorBlob->GetBufferPointer() ) );
				return false;
			}
			// else

			if ( pOutInputLayout != nullptr )
			{
				hr = pDevice->CreateInputLayout
				(
					pInputElementsDesc,
					inputElementsCount,
					compiledShaderBlob->GetBufferPointer(),
					compiledShaderBlob->GetBufferSize(),
					pOutInputLayout
				);
				if ( FAILED( hr ) )
				{
					_ASSERT_EXPR( SUCCEEDED( hr ), reinterpret_cast<LPCSTR>( errorBlob->GetBufferPointer() ) );
					return false;
				}
				// else
			}

			if ( isEnableCache )
			{
				vertexShaderCache.insert
				(
					std::make_pair
					(
						shaderId,
						VertexShaderCacheContents
						{
							*pOutVertexShader,
							( ( pOutInputLayout != nullptr ) ? *pOutInputLayout : nullptr )
						}
					)
				);
			}

			return true;
		}

		void ReleaseAllVertexShaderCaches()
		{
			vertexShaderCache.clear();
		}

		#pragma endregion

		#pragma region PixelShaderCache

		static std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11PixelShader>> pixelShaderCache{};
		
		bool CreatePixelShaderFromCso( ID3D11Device  *d3dDevice, const char *csoname, const char *openMode, ID3D11PixelShader **d3dPixelShader, bool enableCache )
		{
			HRESULT hr = S_OK;

			std::string strCsoName = csoname;

			if ( !Donya::IsExistFile( strCsoName ) ) { return false; }
			// else

			auto it = pixelShaderCache.find( strCsoName );
			if ( it != pixelShaderCache.end() )
			{
				*d3dPixelShader = it->second.Get();
				( *d3dPixelShader )->AddRef();

				return true;
			}
			// else

			FILE* fp = nullptr;
			fopen_s( &fp, strCsoName.c_str(), openMode );
			if ( !fp ) { _ASSERT_EXPR( 0, L"ps cso file not found" ); }

			fseek( fp, 0, SEEK_END );
			long csoSize = ftell( fp );
			fseek( fp, 0, SEEK_SET );

			std::unique_ptr<unsigned char[]> csoData = std::make_unique<unsigned char[]>( csoSize );
			fread( csoData.get(), csoSize, 1, fp );
			fclose( fp );

			hr = d3dDevice->CreatePixelShader
			(
				csoData.get(),
				csoSize,
				NULL,
				d3dPixelShader
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreatePixelShader()" );

			if ( enableCache )
			{
				pixelShaderCache.insert
				(
					std::make_pair
					(
						strCsoName,
						*d3dPixelShader
					)
				);
			}

			return true;
		}

		bool CreatePixelShaderFromSource( ID3D11Device *pDevice, const std::string &shaderId, const std::string &shaderCode, const std::string &shaderEntryPoint, ID3D11PixelShader **pOutPixelShader, bool isEnableCache )
		{
			HRESULT hr = S_OK;

			auto it = pixelShaderCache.find( shaderId );
			if ( it != pixelShaderCache.end() )
			{
				*pOutPixelShader = it->second.Get();
				( *pOutPixelShader )->AddRef();

				return true;
			}
			// else

			DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;
		#if DEBUG_MODE
			flags |= D3DCOMPILE_DEBUG;
			flags |= D3D10_SHADER_SKIP_OPTIMIZATION;
		#endif

			Microsoft::WRL::ComPtr<ID3DBlob> compiledShaderBlob;
			Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

			// D3DCompile() : https://docs.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompile
			hr = D3DCompile
			(
				shaderCode.c_str(),
				shaderCode.length(),
				NULL,
				NULL,
				NULL,
				shaderEntryPoint.c_str(),
				"ps_5_0",
				flags,
				NULL,
				compiledShaderBlob.GetAddressOf(),
				errorBlob.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, reinterpret_cast<LPCSTR>( errorBlob->GetBufferPointer() ) );
				return false;
			}
			// else

			hr = pDevice->CreatePixelShader
			(
				compiledShaderBlob->GetBufferPointer(),
				compiledShaderBlob->GetBufferSize(),
				0,
				pOutPixelShader
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, reinterpret_cast<LPCSTR>( errorBlob->GetBufferPointer() ) );
				return false;
			}
			// else

			if ( isEnableCache )
			{
				pixelShaderCache.insert
				(
					std::make_pair
					(
						shaderId,
						*pOutPixelShader
					)
				);
			}

			return true;
		}

		void ReleaseAllPixelShaderCaches()
		{
			pixelShaderCache.clear();
		}

		#pragma endregion

		#pragma region SpriteCache

		struct SpriteCacheContents
		{
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	d3dShaderResourceView;
			D3D11_TEXTURE2D_DESC								d3dTexture2DDesc;
		public:
			SpriteCacheContents
			(
				ID3D11ShaderResourceView	*pShaderResourceView,
				D3D11_TEXTURE2D_DESC		*pTexture2DDesc
			) :
				d3dShaderResourceView( pShaderResourceView ),
				d3dTexture2DDesc( *pTexture2DDesc )
			{}
		};

		static std::unordered_map<std::wstring, SpriteCacheContents> spriteCache{};

		bool CreateTexture2DFromFile( ID3D11Device *d3dDevice, const std::wstring &filename, ID3D11ShaderResourceView **d3dShaderResourceView, D3D11_TEXTURE2D_DESC *d3dTexture2DDesc, bool isEnableCache )
		{
			HRESULT hr = S_OK;

			if ( !Donya::IsExistFile( filename ) ) { return false; }
			// else

			auto it = spriteCache.find( filename );
			if ( it != spriteCache.end() )
			{
				*d3dShaderResourceView = it->second.d3dShaderResourceView.Get();
				( *d3dShaderResourceView )->AddRef();

				*d3dTexture2DDesc = it->second.d3dTexture2DDesc;

				return true;
			}
			// else

			Microsoft::WRL::ComPtr<ID3D11Resource> d3dResource;
			if ( filename.find( L".dds" ) != std::wstring::npos )
			{
				hr = CreateDDSTextureFromFile
				(
					d3dDevice, filename.c_str(),
					d3dResource.GetAddressOf(),
					d3dShaderResourceView
				);
			}
			else
			{
				hr = CreateWICTextureFromFile	// ID3D11Resource と ID3D11ShaderResourceView の２つが作成される
				(
					d3dDevice, filename.c_str(),
					d3dResource.GetAddressOf(),
					d3dShaderResourceView
				);
			}
			_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateWICTextureFromFile()" ) );

			Microsoft::WRL::ComPtr<ID3D11Texture2D> d3dTexture2D;
			hr = d3dResource.Get()->QueryInterface<ID3D11Texture2D>( d3dTexture2D.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : QueryInterface()" ) );

			d3dTexture2D->GetDesc( d3dTexture2DDesc );	// テクスチャ情報の取得

			/*
			if ( d3dTexture2DDesc->BindFlags & D3D11_BIND_SHADER_RESOURCE )
			{
				// D3D11 WARNING: Process is terminating. Using simple reporting. Please call ReportLiveObjects() at runtime for standard reporting.
				// D3D11 WARNING: Live Producer at 0x0023E8CC, Refcount: 2. [ STATE_CREATION WARNING #0: UNKNOWN]
				D3D11_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc{};
				d3dShaderResourceViewDesc.Format = d3dTexture2DDesc->Format;
				d3dShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				d3dShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
				d3dShaderResourceViewDesc.Texture2D.MipLevels = d3dTexture2DDesc->MipLevels;

				hr = d3dDevice->CreateShaderResourceView
				(
					d3dResource.Get(),
					&d3dShaderResourceViewDesc,
					d3dShaderResourceView
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateShaderResourceView()" ) );
			}
			*/

			if ( isEnableCache )
			{
				spriteCache.insert
				(
					std::make_pair
					(
						filename,
						SpriteCacheContents
						{
							*d3dShaderResourceView,
							d3dTexture2DDesc
						}
					)
				);
			}

			return true;
		}

		void CreateUnicolorTexture( ID3D11Device *pDevice, ID3D11ShaderResourceView **pOutSRV, D3D11_TEXTURE2D_DESC *pOutTexDesc, unsigned int dimensions, float R, float G, float B, float A, bool isEnableCache )
		{
			unsigned int RGBA{};
			{
				auto Clamp = []( float &color )
				{
					if ( color < 0.0f ) { color = 0.0f; }
					if ( 1.0f < color ) { color = 1.0f; }
				};
				Clamp( R );
				Clamp( B );
				Clamp( G );
				Clamp( A );

				int r = scast<unsigned int>( R * 255.0f );
				int g = scast<unsigned int>( G * 255.0f );
				int b = scast<unsigned int>( B * 255.0f );
				int a = scast<unsigned int>( A * 255.0f );

				RGBA = ( r << 24 ) | ( g << 16 ) | ( b << 8 ) | ( a << 0 );
			}

			std::wstring dummyFileName = L"UnicolorTexture:[RGBA:" + std::to_wstring( RGBA ) + L"]";
			auto &it = spriteCache.find( dummyFileName );
			if ( it != spriteCache.end() )
			{
				*pOutSRV = it->second.d3dShaderResourceView.Get();
				( *pOutSRV )->AddRef();

				return;
			}
			// else

			HRESULT hr = S_OK;

			*pOutTexDesc = {};
			pOutTexDesc->Width				= dimensions;
			pOutTexDesc->Height				= dimensions;
			pOutTexDesc->MipLevels			= 1;
			pOutTexDesc->ArraySize			= 1;
			pOutTexDesc->Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
			pOutTexDesc->SampleDesc.Count	= 1;
			pOutTexDesc->SampleDesc.Quality	= 0;
			pOutTexDesc->Usage				= D3D11_USAGE_DEFAULT;
			pOutTexDesc->BindFlags			= D3D11_BIND_SHADER_RESOURCE;
			pOutTexDesc->CPUAccessFlags		= 0;
			pOutTexDesc->MiscFlags			= 0;

			std::unique_ptr<unsigned int[]> pSysMem = std::make_unique<unsigned int[]>( dimensions * dimensions );
			for ( unsigned int i = 0; i < dimensions * dimensions; i++ )
			{
				pSysMem[i] = RGBA;
			}
			D3D11_SUBRESOURCE_DATA subresource{};
			subresource.pSysMem				= pSysMem.get();
			subresource.SysMemPitch			= sizeof( unsigned int ) * dimensions;
			subresource.SysMemSlicePitch	= 0;

			Microsoft::WRL::ComPtr<ID3D11Texture2D> iTexture2D{};
			hr = pDevice->CreateTexture2D( pOutTexDesc, &subresource, iTexture2D.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateUnocolorTexture" ) );


			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
			SRVDesc.Format					= pOutTexDesc->Format;
			SRVDesc.ViewDimension			= D3D11_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels		= 1;

			hr = pDevice->CreateShaderResourceView( iTexture2D.Get(), &SRVDesc, pOutSRV );
			_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateUnocolorTexture" ) );

			if ( isEnableCache )
			{
				spriteCache.insert
				(
					std::make_pair
					(
						dummyFileName,
						SpriteCacheContents
						{
							*pOutSRV,
							pOutTexDesc
						}
					)
				);
			}
		}

		void ReleaseAllTexture2DCaches()
		{
			spriteCache.clear();
		}

	#pragma region Sampler

		static std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplerCache{};

		size_t RequireSamplerDescHash( const D3D11_SAMPLER_DESC &key )
		{
			std::string bytes( reinterpret_cast<const char *>( &key ) );
			return std::hash<std::string>()( bytes );
		}

		void CreateSamplerState( ID3D11Device *pDevice, Microsoft::WRL::ComPtr<ID3D11SamplerState> *pOutSampler, const D3D11_SAMPLER_DESC &samplerDesc, bool isEnableCache )
		{
			size_t hash = RequireSamplerDescHash( samplerDesc );

			auto it = samplerCache.find( hash );
			if ( it != samplerCache.end() )
			{
				*pOutSampler = it->second;
				return;
			}
			// else

			HRESULT hr = S_OK;

			hr = pDevice->CreateSamplerState( &samplerDesc, pOutSampler->ReleaseAndGetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateSamplerState()" ) );

			if ( isEnableCache )
			{
				samplerCache.insert( std::make_pair( hash, *pOutSampler ) );
			}
		}

		Microsoft::WRL::ComPtr<ID3D11SamplerState> &RequireInvalidSamplerStateComPtr()
		{
			static Microsoft::WRL::ComPtr<ID3D11SamplerState> pInvalidSampler;
			if ( !pInvalidSampler )
			{
				D3D11_SAMPLER_DESC null{};
				null.AddressU = null.AddressV = null.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

				HRESULT hr = Donya::GetDevice()->CreateSamplerState( &null, pInvalidSampler.GetAddressOf() );
				_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateSamplerState()" ) );
			}

			return pInvalidSampler;
		}

	#pragma endregion

		#pragma endregion

		#pragma region OBJ

		struct ObjFileCacheContents
		{
			std::vector<DirectX::XMFLOAT3>	vertices;
			std::vector<DirectX::XMFLOAT3>	normals;
			std::vector<DirectX::XMFLOAT2>	texCoords;
			std::vector<size_t>				indices;
			std::vector<Material>			materials;
		public:
			ObjFileCacheContents
			(
				std::vector<DirectX::XMFLOAT3>	*pVertices,
				std::vector<DirectX::XMFLOAT3>	*pNormals,
				std::vector<DirectX::XMFLOAT2>	*pTexCoords,
				std::vector<size_t>				*pIndices,
				std::vector<Material>			*pMaterials
			) :
				vertices( *pVertices ),
				normals( *pNormals ),
				texCoords( *pTexCoords ),
				indices( *pIndices ),
				materials( *pMaterials )
			{}
			~ObjFileCacheContents()
			{
				std::vector<DirectX::XMFLOAT3>().swap( vertices );
				std::vector<DirectX::XMFLOAT3>().swap( normals );
				std::vector<DirectX::XMFLOAT2>().swap( texCoords );
				std::vector<size_t>().swap( indices );
				std::vector<Material>().swap( materials );
			}
		};

		static std::unordered_map<std::wstring, ObjFileCacheContents> objFileCache;

		/// <summary>
		/// It is storage of materials by mtl-file.
		/// </summary>
		class MtlFile
		{
		private:
			std::wstring mtllibName;
			std::unordered_map<std::wstring, Material> newMtls;
		public:
			MtlFile( ID3D11Device *pDevice, const std::wstring &mtlFileName ) : mtllibName(), newMtls()
			{
				Load( pDevice, mtlFileName );
			}
			~MtlFile() {}
			MtlFile( const MtlFile & ) = delete;
			MtlFile & operator = ( const MtlFile & ) = delete;
		private:
			void Load( ID3D11Device *pDevice, const std::wstring &mtlFileName )
			{
				std::wifstream ifs( mtlFileName.c_str() );
				if ( !ifs )
				{
					_ASSERT_EXPR( 0, L"Failed : load mtl flie." );
					return;
				}
				// else
				mtllibName = mtlFileName;

				std::wstring mtlName{};	// This name is immutable until update.
				std::wstring lineBuf{};
				std::wstringstream ss{};
				for ( size_t i = 0; ifs; ++i )
				{
					std::getline( ifs, lineBuf );

					if ( IsContainStr( lineBuf, L"#" ) )
					{
						continue;
					}
					// else
					if ( IsContainStr( lineBuf, L"\n" ) )
					{
						continue;
					}
					// else
					if ( IsContainStr( lineBuf, L"newmtl" ) )
					{
						#pragma region newmtl

						SkipUntilNextDelim( &lineBuf, &ss );
						lineBuf.erase( lineBuf.begin() );	// erase space.

						mtlName = lineBuf;
						newMtls.insert( std::make_pair( mtlName, Material{} ) );

						#pragma endregion
						continue;
					}
					// else
					if ( IsContainStr( lineBuf, L"map" ) )
					{
						#pragma region map_

						if ( IsContainStr( lineBuf, L"map_Kd" ) )
						{
							#pragma region diffuseMap

							SkipUntilNextDelim( &lineBuf, &ss );
							lineBuf.erase( lineBuf.begin() );	// erase space.
							ss.ignore();

							decltype( newMtls )::iterator it = newMtls.find( mtlName );
							if ( it != newMtls.end() )
							{
								std::wstring mapPath = Donya::ExtractFileDirectoryFromFullPath( mtlFileName );
								std::wstring mapName; ss >> mapName;
								{
									if ( IsContainStr( mapName, L"-" ) )
									{
										#pragma region options

										// TODO:Apply a options.

										ss.ignore();
										ss >> mapName;

										#pragma endregion
									}
								}

								it->second.diffuseMap.mapName = mapPath + mapName;

								D3D11_SAMPLER_DESC samplerDesc{};
								samplerDesc.Filter			= D3D11_FILTER_MIN_MAG_MIP_LINEAR;
								samplerDesc.AddressU		= D3D11_TEXTURE_ADDRESS_WRAP;
								samplerDesc.AddressV		= D3D11_TEXTURE_ADDRESS_WRAP;
								samplerDesc.AddressW		= D3D11_TEXTURE_ADDRESS_WRAP;
								samplerDesc.ComparisonFunc	= D3D11_COMPARISON_NEVER;
								samplerDesc.MinLOD			= 0;
								samplerDesc.MaxLOD			= D3D11_FLOAT32_MAX;

								it->second.CreateDiffuseMap( pDevice, samplerDesc );
							}

							#pragma endregion
							continue;
						}
						// else

						#pragma endregion
						continue;
					}
					// else
					if ( IsContainStr( lineBuf, L"illum" ) )
					{
						#pragma region illum

						SkipUntilNextDelim( &lineBuf, &ss );

						decltype( newMtls )::iterator it = newMtls.find( mtlName );
						if ( it != newMtls.end() )
						{
							ss >> it->second.illuminate;
						}

						#pragma endregion
						continue;
					}
					// else
					if ( IsContainStr( lineBuf, L"Ns" ) )
					{
						#pragma region Ns

						SkipUntilNextDelim( &lineBuf, &ss );

						decltype( newMtls )::iterator it = newMtls.find( mtlName );
						if ( it != newMtls.end() )
						{
							ss >> it->second.shininess;
						}

						#pragma endregion
						continue;
					}
					// else
					if ( IsContainStr( lineBuf, L"Ka" ) )
					{
						#pragma region Ka

						SkipUntilNextDelim( &lineBuf, &ss );

						decltype( newMtls )::iterator it = newMtls.find( mtlName );
						if ( it != newMtls.end() )
						{
							ss >> it->second.ambient[0]; ss.ignore();
							ss >> it->second.ambient[1]; ss.ignore();
							ss >> it->second.ambient[2]; ss.ignore();
						}

						#pragma endregion
						continue;
					}
					// else
					if ( IsContainStr( lineBuf, L"Kd" ) )
					{
						#pragma region Kd

						SkipUntilNextDelim( &lineBuf, &ss );

						decltype( newMtls )::iterator it = newMtls.find( mtlName );
						if ( it != newMtls.end() )
						{
							ss >> it->second.diffuse[0]; ss.ignore();
							ss >> it->second.diffuse[1]; ss.ignore();
							ss >> it->second.diffuse[2]; ss.ignore();
						}

						#pragma endregion
						continue;
					}
					// else
					if ( IsContainStr( lineBuf, L"Ks" ) )
					{
						#pragma region Ks

						SkipUntilNextDelim( &lineBuf, &ss );

						decltype( newMtls )::iterator it = newMtls.find( mtlName );
						if ( it != newMtls.end() )
						{
							ss >> it->second.specular[0]; ss.ignore();
							ss >> it->second.specular[1]; ss.ignore();
							ss >> it->second.specular[2]; ss.ignore();
						}

						#pragma endregion
						continue;
					}
					// else
				}
			}
		public:
			// returns false when if not found, set nullptr to output.
			bool Extract( const std::wstring &useMtlName, Material **materialPointer )
			{
				decltype( newMtls )::iterator it = newMtls.find( useMtlName );
				if ( it == newMtls.end() )
				{
					materialPointer = nullptr;
					return false;
				}
				// else
				*materialPointer = &it->second;
				return true;
			}

			void CopyAllMaterialsToVector( std::vector<Material> *pMaterials ) const
			{
				for ( const auto &it : newMtls )
				{
					pMaterials->push_back( it.second );
				}
			}
		};

		// TODO:There are many unsupported extensions yet.
		bool LoadObjFile( ID3D11Device *pDevice, const std::wstring &objFileName, std::vector<DirectX::XMFLOAT3> *pVertices, std::vector<DirectX::XMFLOAT3> *pNormals, std::vector<XMFLOAT2> *pTexCoords, std::vector<size_t> *pIndices, std::vector<Material> *pMaterials, bool *hasLoadedMtl, bool isEnableCache )
		{
			if ( !Donya::IsExistFile( objFileName ) ) { return false; }
			// else

			// check objFileName already has cached ?
			{
				decltype( objFileCache )::iterator it = objFileCache.find( objFileName );
				if ( it != objFileCache.end() )
				{
					if ( pVertices  ) { *pVertices  = it->second.vertices;  }
					if ( pNormals   ) { *pNormals   = it->second.normals;   }
					if ( pTexCoords ) { *pNormals   = it->second.normals;   }
					if ( pIndices   ) { *pIndices   = it->second.indices;   }
					if ( pMaterials ) { *pMaterials = it->second.materials; }

					return true;
				}
			}
			// else

			if ( pVertices == nullptr ) { return false; }
			// else

			std::wifstream ifs( objFileName.c_str() );
			if ( !ifs )
			{
				_ASSERT_EXPR( 0, L"Failed : load obj flie." );
				return false;
			}
			// else

			size_t lastVertexIndex   = 0;	// zero-based number.
			size_t lastNormalIndex   = 0;	// zero-based number.
			size_t lastTexCoordIndex = 0;	// zero-based number.
			std::wstring command{};
			std::wstringstream ss{};
			std::vector<DirectX::XMFLOAT3> tmpPositions{};
			std::vector<DirectX::XMFLOAT3> tmpNormals{};
			std::vector<DirectX::XMFLOAT2> tmpTexCoords{};

			size_t		materialCount = 0;	// zero-based number.
			Material	*usemtlTarget = nullptr;
			std::unique_ptr<MtlFile> pMtllib{};

			for ( size_t i = 0; ifs; ++i )	// i is column num.
			{
				std::getline( ifs, command );

				if ( IsContainStr( command, L"#" ) )
				{
					// comment.
					continue;
				}
				// else
				if ( IsContainStr( command, L"\n" ) )
				{
					// linefeed.
					continue;
				}
				// else
				if ( IsContainStr( command, L"mtllib" ) )
				{
					#pragma region mtllib

					SkipUntilNextDelim( &command, &ss );

					std::wstring mtlPath = Donya::ExtractFileDirectoryFromFullPath( objFileName );

					std::wstring mtlName; ss >> mtlName;

					pMtllib = std::make_unique<MtlFile>( pDevice, mtlPath + mtlName );

					#pragma endregion
					continue;
				}
				// else
				if ( IsContainStr( command, L"g " ) )
				{
					#pragma region Group
					#pragma endregion
					continue;
				}
				// else
				if ( IsContainStr( command, L"s " ) )
				{
					#pragma region Smooth
					#pragma endregion
					continue;
				}
				// else
				if ( IsContainStr( command, L"v " ) )
				{
					#pragma region Vertex

					SkipUntilNextDelim( &command, &ss );

					float x, y, z;
					ss >> x;
					ss >> y;
					ss >> z;

					tmpPositions.push_back( { x, y, z } );

					lastVertexIndex++;

					#pragma endregion
					continue;
				}
				// else
				if ( IsContainStr( command, L"vt " ) )
				{
					#pragma region TexCoord
					if ( pTexCoords == nullptr ) { continue; }
					// else

					SkipUntilNextDelim( &command, &ss );

					float u, v;
					ss >> u;
					ss >> v;
					
					// tmpTexCoords.push_back( { u, v } );	// If obj-file is LH
					tmpTexCoords.push_back( { u, -v } );	// If obj-file is RH

					lastTexCoordIndex++;

					#pragma endregion
					continue;
				}
				// else
				if ( IsContainStr( command, L"vn " ) )
				{
					#pragma region Normal
					if ( pNormals == nullptr ) { continue; }
					// else

					SkipUntilNextDelim( &command, &ss );

					float x, y, z;
					ss >> x;
					ss >> y;
					ss >> z;

					tmpNormals.push_back( { x, y, z } );

					lastNormalIndex++;

					#pragma endregion
					continue;
				}
				// else
				if ( IsContainStr( command, L"usemtl " ) )
				{
					#pragma region usemtl

					SkipUntilNextDelim( &command, &ss );
					command.erase( command.begin() );	// erase space.
					ss.ignore();

					if ( usemtlTarget != nullptr )
					{
						usemtlTarget->indexCount = materialCount;

						usemtlTarget  = nullptr;
						materialCount = 0;
					}

					std::wstring materialName; ss >> materialName;
					pMtllib->Extract( materialName, &usemtlTarget );

					if ( usemtlTarget != nullptr )
					{
						usemtlTarget->indexStart = pIndices->size();
					}

					#pragma endregion
					continue;
				}
				// else
				if ( IsContainStr( command, L"f " ) )
				{
					#pragma region Face Indices

					SkipUntilNextDelim( &command, &ss );

					for ( ; !ss.eof(); )
					{
						int index = NULL;

						// Vertex
						{
							ss >> index;
							if ( index < 0 )
							{
								index++; // convert to minus-one-based.
								index = lastVertexIndex + index;
							}

							if ( pIndices->empty() )
							{
								pIndices->push_back( 0 );
							}
							else
							{
								pIndices->push_back( pIndices->back() + 1 );
							}
							materialCount++;

							pVertices->push_back( tmpPositions[index - 1] ); // convert one-based -> zero-based.
						}

						if ( ss.peek() != L'/' ) { ss.ignore( 1024, L' '  ); continue; }
						// else
						ss.ignore();

						// TexCoord
						{
							ss >> index;
							if ( index < 0 )
							{
								index++; // convert to minus-one-based.
								index = lastTexCoordIndex + index;
							}

							if ( tmpTexCoords.size() < scast<size_t>( index ) )
							{
								_ASSERT_EXPR( 0, L"obj file error! : not found specified normal-index until specify position." );
								return false;
							}

							pTexCoords->push_back( tmpTexCoords[index - 1] ); // convert one-based -> zero-based.
						}

						if ( ss.peek() != L'/' ) { ss.ignore( 1024, L' ' ); continue; }
						// else
						ss.ignore();

						// Normal
						{
							ss >> index;
							if ( index < 0 )
							{
								index++; // convert to minus-one-based.
								index = lastNormalIndex + index;
							}

							if ( tmpNormals.size() < scast<size_t>( index ) )
							{
								_ASSERT_EXPR( 0, L"obj file error! : not found specified normal-index until specify position." );
								return false;
							}

							pNormals->push_back( tmpNormals[index - 1] ); // convert one-based -> zero-based.
						}

						ss.ignore();
					}

					#pragma endregion
					continue;
				}
				// else
			}

			if ( usemtlTarget != nullptr )
			{
				usemtlTarget->indexCount = materialCount;

				usemtlTarget  = nullptr;
				materialCount = 0;
			}

			if ( pMaterials != nullptr && pMtllib != nullptr )
			{
				pMtllib->CopyAllMaterialsToVector( pMaterials );
			}

			ifs.close();

			if ( isEnableCache )
			{
				objFileCache.insert
				(
					std::make_pair
					(
						objFileName,
						ObjFileCacheContents{ pVertices, pNormals, pTexCoords, pIndices, pMaterials }
					)
				);
			}

			return true;
		}

		void ReleaseAllObjFileCaches()
		{
			objFileCache.clear();
		}

		#pragma endregion
	
		#pragma region ID3DObject

		#pragma endregion

		void ReleaseAllCachedResources()
		{
			ReleaseAllVertexShaderCaches();
			ReleaseAllPixelShaderCaches();
			ReleaseAllTexture2DCaches();
			ReleaseAllObjFileCaches();
		}
	}

}