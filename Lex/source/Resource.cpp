#include "Resource.h"

#include <D3D11.h>
#include <DirectXMath.h>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <tchar.h>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

#include "Common.h"

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
	#pragma region VerteShaderCache

		struct VertexShaderCacheContents
		{
			Microsoft::WRL::ComPtr<ID3D11VertexShader> d3dVertexShader;
			Microsoft::WRL::ComPtr<ID3D11InputLayout>  d3dInputLayout;
		public:
			VertexShaderCacheContents
			(
				ID3D11VertexShader *pVertexShader,
				ID3D11InputLayout *pInputLayout
			) :
				d3dVertexShader( pVertexShader ),
				d3dInputLayout( pInputLayout )
			{
			}
		};

		static std::map<const char *, VertexShaderCacheContents> vertexShaderCache{};
		static std::string vertexShaderDirectory{};

		void RegisterDirectoryOfVertexShader( const char *fileDirectory )
		{
			vertexShaderDirectory = fileDirectory;
		}

		void CreateVertexShaderFromCso( ID3D11Device *d3dDevice, const char *csoname, const char *openMode, ID3D11VertexShader **d3dVertexShader, ID3D11InputLayout **d3dInputLayout, D3D11_INPUT_ELEMENT_DESC *d3dInputElementsDesc, size_t inputElementDescSize, bool enableCache )
		{
			HRESULT hr = S_OK;

			std::string combinedCsoname = vertexShaderDirectory + csoname;

			std::map<const char *, VertexShaderCacheContents>::iterator it = vertexShaderCache.find( combinedCsoname.c_str() );
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

				return;
			}
			// else

			FILE *fp = nullptr;
			fopen_s( &fp, combinedCsoname.c_str(), openMode );
			if ( !fp ) { _ASSERT_EXPR( 0, L"vs cso file not found" ); }

			fseek( fp, 0, SEEK_END );
			long csoSize = ftell( fp );
			fseek( fp, 0, SEEK_SET );

			std::unique_ptr<unsigned char[]> csoData = std::make_unique<unsigned char[]>( csoSize );
			fread( csoData.get(), csoSize, 1, fp );
			fclose( fp );

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
					d3dInputElementsDesc,
					inputElementDescSize,
					csoData.get(),
					csoSize,
					d3dInputLayout
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateInputLayout()" ) );
			}

			if ( !enableCache ) { return; }
			// else

			vertexShaderCache.insert
			(
				std::make_pair
				(
					combinedCsoname.c_str(),
					VertexShaderCacheContents
					{
						*d3dVertexShader,
						( ( d3dInputLayout != nullptr ) ? *d3dInputLayout : nullptr )
					}
				)
			);
		}

		void ReleaseAllVertexShaderCaches()
		{
			vertexShaderCache.clear();
		}

	#pragma endregion

	#pragma region PixelShaderCache

		static std::map<const char *, Microsoft::WRL::ComPtr<ID3D11PixelShader>> pixelShaderCache{};
		static std::string pixelShaderDirectory{};

		void RegisterDirectoryOfPixelShader( const char *fileDirectory )
		{
			pixelShaderDirectory = fileDirectory;
		}

		void CreatePixelShaderFromCso( ID3D11Device *d3dDevice, const char *csoname, const char *openMode, ID3D11PixelShader **d3dPixelShader, bool enableCache )
		{
			HRESULT hr = S_OK;

			std::string combinedCsoname = pixelShaderDirectory + csoname;

			std::map<const char *, Microsoft::WRL::ComPtr<ID3D11PixelShader>>::iterator it = pixelShaderCache.find( combinedCsoname.c_str() );
			if ( it != pixelShaderCache.end() )
			{
				*d3dPixelShader = it->second.Get();
				( *d3dPixelShader )->AddRef();

				return;
			}
			// else

			FILE *fp = nullptr;
			fopen_s( &fp, combinedCsoname.c_str(), openMode );
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

			if ( !enableCache ) { return; }
			// else

			pixelShaderCache.insert
			(
				std::make_pair
				(
					combinedCsoname.c_str(),
					*d3dPixelShader
				)
			);
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
			Microsoft::WRL::ComPtr<ID3D11SamplerState>			d3dSamplerState;
			D3D11_TEXTURE2D_DESC								d3dTexture2DDesc;
		public:
			SpriteCacheContents
			(
				ID3D11ShaderResourceView *pShaderResourceView,
				ID3D11SamplerState *pSamplerState,
				D3D11_TEXTURE2D_DESC *pTexture2DDesc
			) :
				d3dShaderResourceView( pShaderResourceView ),
				d3dSamplerState( pSamplerState ),
				d3dTexture2DDesc( *pTexture2DDesc )
			{
			}
		};

		static std::map<std::wstring, SpriteCacheContents> spriteCache{};
		static std::wstring spriteDirectory{};

		void RegisterDirectoryOfTexture( const wchar_t *fileDirectory )
		{
			spriteDirectory = fileDirectory;
		}

		void CreateTexture2DFromFile( ID3D11Device *d3dDevice, const std::wstring &filename, ID3D11ShaderResourceView **d3dShaderResourceView, ID3D11SamplerState **d3dSamplerState, D3D11_TEXTURE2D_DESC *d3dTexture2DDesc, const D3D11_SAMPLER_DESC *d3dSamplerDesc, bool isEnableCache )
		{
			HRESULT hr = S_OK;

			std::wstring combinedFilename = spriteDirectory + filename;

			std::map<std::wstring, SpriteCacheContents>::iterator it = spriteCache.find( combinedFilename );
			if ( it != spriteCache.end() )
			{
				*d3dShaderResourceView = it->second.d3dShaderResourceView.Get();
				( *d3dShaderResourceView )->AddRef();

				*d3dSamplerState = it->second.d3dSamplerState.Get();
				( *d3dSamplerState )->AddRef();

				*d3dTexture2DDesc = it->second.d3dTexture2DDesc;

				return;
			}
			// else

			Microsoft::WRL::ComPtr<ID3D11Resource> d3dResource;
			if ( combinedFilename.find( L".dds" ) != std::wstring::npos )
			{
				hr = CreateDDSTextureFromFile
				(
					d3dDevice, combinedFilename.c_str(),
					d3dResource.GetAddressOf(),
					d3dShaderResourceView
				);
			}
			else
			{
				hr = CreateWICTextureFromFile	// ID3D11Resource と ID3D11ShaderResourceView の２つが作成される
				(
					d3dDevice, combinedFilename.c_str(),
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

			hr = d3dDevice->CreateSamplerState( d3dSamplerDesc, d3dSamplerState );
			_ASSERT_EXPR( SUCCEEDED( hr ), _TEXT( "Failed : CreateSamplerState()" ) );


			if ( !isEnableCache ) { return; }
			// else

			spriteCache.insert
			(
				std::make_pair
				(
					combinedFilename,
					SpriteCacheContents
					{
						*d3dShaderResourceView,
						*d3dSamplerState,
						d3dTexture2DDesc
					}
				)
			);
		}

		void ReleaseAllTexture2DCaches()
		{
			spriteCache.clear();
		}

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
				std::vector<DirectX::XMFLOAT3> *pVertices,
				std::vector<DirectX::XMFLOAT3> *pNormals,
				std::vector<DirectX::XMFLOAT2> *pTexCoords,
				std::vector<size_t> *pIndices,
				std::vector<Material> *pMaterials
			) :
				vertices( *pVertices ),
				normals( *pNormals ),
				texCoords( *pTexCoords ),
				indices( *pIndices ),
				materials( *pMaterials )
			{
			}
			~ObjFileCacheContents()
			{
				std::vector<DirectX::XMFLOAT3>().swap( vertices );
				std::vector<DirectX::XMFLOAT3>().swap( normals );
				std::vector<DirectX::XMFLOAT2>().swap( texCoords );
				std::vector<size_t>().swap( indices );
				std::vector<Material>().swap( materials );
			}
		};

		static std::map<std::wstring, ObjFileCacheContents> objFileCache;

		/// <summary>
		/// It is storage of materials by mtl-file.
		/// </summary>
		class MtlFile
		{
		private:
			std::wstring mtllibName;
			std::map<std::wstring, Material> newMtls;
		public:
			MtlFile( ID3D11Device *pDevice, const std::wstring &mtlFileName ) : mtllibName(), newMtls()
			{
				Load( pDevice, mtlFileName );
			}
			~MtlFile() {}
			MtlFile( const MtlFile & ) = delete;
			MtlFile &operator = ( const MtlFile & ) = delete;
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
								std::wstring mapPath = mtlFileName;
								{
									size_t lastTreePos = mapPath.find_last_of( L"/" );
									mapPath = mapPath.substr( 0, lastTreePos );
									mapPath += L"/";
								}
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
								samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
								samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
								samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
								samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
								samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
								samplerDesc.MinLOD = 0;
								samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

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
		void LoadObjFile( ID3D11Device *pDevice, const std::wstring &objFileName, std::vector<DirectX::XMFLOAT3> *pVertices, std::vector<DirectX::XMFLOAT3> *pNormals, std::vector<XMFLOAT2> *pTexCoords, std::vector<size_t> *pIndices, std::vector<Material> *pMaterials, bool *hasLoadedMtl, bool isEnableCache )
		{
			// check objFileName already has cached ?
			{
				decltype( objFileCache )::iterator it = objFileCache.find( objFileName );
				if ( it != objFileCache.end() )
				{
					if ( pVertices ) { *pVertices = it->second.vertices; }
					if ( pNormals ) { *pNormals = it->second.normals; }
					if ( pTexCoords ) { *pNormals = it->second.normals; }
					if ( pIndices ) { *pIndices = it->second.indices; }
					if ( pMaterials ) { *pMaterials = it->second.materials; }

					return;
				}
			}
			// else

			if ( pVertices == nullptr ) { return; }
			// else

			std::wifstream ifs( objFileName.c_str() );
			if ( !ifs )
			{
				_ASSERT_EXPR( 0, L"Failed : load obj flie." );
				return;
			}
			// else

			size_t lastVertexIndex = 0;	// zero-based number.
			size_t lastNormalIndex = 0;	// zero-based number.
			size_t lastTexCoordIndex = 0;	// zero-based number.
			std::wstring command{};
			std::wstringstream ss{};
			std::vector<DirectX::XMFLOAT3> tmpPositions{};
			std::vector<DirectX::XMFLOAT3> tmpNormals{};
			std::vector<DirectX::XMFLOAT2> tmpTexCoords{};

			size_t		materialCount = 0;	// zero-based number.
			Material *usemtlTarget = nullptr;
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

					std::wstring mtlPath = objFileName;
					{
						size_t lastTreePos = mtlPath.find_last_of( L"/" );
						mtlPath = mtlPath.substr( 0, lastTreePos );
						mtlPath += L"/";
					}

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

						usemtlTarget = nullptr;
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

						if ( ss.peek() != L'/' ) { ss.ignore( 1024, L' ' ); continue; }
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
								return;
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
								return;
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

				usemtlTarget = nullptr;
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
		}

		void ReleaseAllObjFileCaches()
		{
			objFileCache.clear();
		}

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