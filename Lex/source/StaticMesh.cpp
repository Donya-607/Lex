#include "StaticMesh.h"

#include <algorithm>	// Use std::min().
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>

#include "Common.h"
#include "Direct3DUtil.h"
#include "Donya.h"
#include "Loader.h"
#include "Resource.h"
#include "Vector.h"
#include "Useful.h"

#undef max
#undef min

using namespace DirectX;

namespace Donya
{
	std::shared_ptr<StaticMesh> StaticMesh::Create( const Loader &loader, const std::string *pVSCsoPath, const std::string *pPSCsoPath )
	{
		std::shared_ptr<StaticMesh> pInstance{};

		const std::vector<Loader::Mesh> *pLoadedMeshes = loader.GetMeshes();
		size_t loadedMeshCount = pLoadedMeshes->size();

		std::vector<std::vector<Vertex>> verticesPerMesh{};
		std::vector<std::vector<size_t>> indicesPerMesh{};

		std::vector<StaticMesh::Subset> subsets{};
		subsets.resize( loadedMeshCount );
		for ( size_t i = 0; i < loadedMeshCount; ++i )
		{
			auto &loadedMesh = ( *pLoadedMeshes )[i];

			std::vector<Vertex> vertices{};
			{
				const std::vector<Donya::Vector3> &positions	= loadedMesh.positions;
				const std::vector<Donya::Vector3> &normals		= loadedMesh.normals;
				const std::vector<Donya::Vector2> &texCoords	= loadedMesh.texCoords;

				vertices.resize( std::min( positions.size(), normals.size() ) );
				size_t end = vertices.size();
				for ( size_t j = 0; j < end; ++j )
				{
					vertices[j].pos			= positions[j];
					vertices[j].normal		= normals[j];
					vertices[j].texCoord	= ( j < texCoords.size() )
											? texCoords[j]
											: Donya::Vector2{};
				}
			}
			verticesPerMesh.emplace_back( vertices );
			indicesPerMesh.emplace_back( loadedMesh.indices );

			size_t subsetCount = loadedMesh.subsets.size();
			for ( size_t j = 0; j < subsetCount; ++j )
			{
				auto &loadedSubset	= loadedMesh.subsets[j];
				auto &mySubset		= subsets[i];

				mySubset.indexStart		= loadedSubset.indexStart;
				mySubset.indexCount		= loadedSubset.indexCount;
				mySubset.transparency	= loadedSubset.transparency;

				auto FetchMaterialContain =
				[]( StaticMesh::Material *meshMtl, const Loader::Material &loadedMtl )
				{
					meshMtl->color.x = loadedMtl.color.x;
					meshMtl->color.y = loadedMtl.color.y;
					meshMtl->color.z = loadedMtl.color.z;
					meshMtl->color.w = 1.0f;

					size_t texCount = loadedMtl.textureNames.size();
					meshMtl->textures.resize( texCount );
					for ( size_t i = 0; i < texCount; ++i )
					{
						meshMtl->textures[i].fileName = loadedMtl.textureNames[i];
					}
				};

				FetchMaterialContain( &mySubset.ambient,	loadedSubset.ambient	);
				FetchMaterialContain( &mySubset.bump,		loadedSubset.bump		);
				FetchMaterialContain( &mySubset.diffuse,	loadedSubset.diffuse	);
				FetchMaterialContain( &mySubset.emissive,	loadedSubset.emissive	);
				FetchMaterialContain( &mySubset.specular,	loadedSubset.specular	);
				mySubset.specular.color.w = loadedSubset.specular.color.w;
			}
		} // meshs loop

		// Connect vertices and indices by vertices-per-mesh, indices-per-mesh.
		std::vector<Vertex> connectedVertices{};
		std::vector<size_t> connectedIndices{};
		{
			for ( const auto &itr : verticesPerMesh )
			{
				for ( const auto &it : itr )
				{
					connectedVertices.emplace_back( it );
				}
			}

			size_t currentIndicesCount = 0;
			size_t indicesSize = indicesPerMesh.size();
			for ( size_t i = 0; i < indicesSize; ++i )
			{
				auto &indices = indicesPerMesh[i];
				for ( const auto &it : indices )
				{
					connectedIndices.emplace_back( currentIndicesCount + it );
				}

				currentIndicesCount += verticesPerMesh[i].size();
			}
		}

		std::string csoPathVS = ( !pVSCsoPath ) ? "" : *pVSCsoPath;
		std::string csoPathPS = ( !pPSCsoPath ) ? "" : *pPSCsoPath;
		pInstance = std::make_shared<StaticMesh>( L"DOES_NOT_USE_OBJ_FILE", csoPathVS, csoPathPS );
		pInstance->Init( connectedVertices, connectedIndices, subsets );

		return pInstance;
	}

	constexpr const char *DefaultShaderSourceCode()
	{
		return
		"struct VS_IN\n"
		"{\n"
		"	float4 pos			: POSITION;\n"
		"	float4 normal		: NORMAL;\n"
		"	float2 texCoord		: TEXCOORD;\n"
		"};\n"
		"struct VS_OUT\n"
		"{\n"
		"	float4 pos			: SV_POSITION;\n"
		"	float4 color		: COLOR;\n"
		"	float4 eyeVector	: TEXCOORD0;\n"
		"	float4 normal		: TEXCOORD1;\n"
		"	float2 texCoord		: TEXCOORD2;\n"
		"};\n"
		"cbuffer CONSTANT_BUFFER : register( b0 )\n"
		"{\n"
		"	row_major\n"
		"	float4x4	worldViewProjection;\n"
		"	row_major\n"
		"	float4x4	world;\n"
		"	float4		lightDirection;\n"
		"	float4		materialColor;\n"
		"	float4		cameraPosition;\n"
		"};\n"
		"cbuffer MATERIAL_BUFFER : register( b1 )\n"
		"{\n"
		"	float4 ambient;\n"
		"	float4 diffuse;\n"
		"	float4 specular;\n"
		"};\n"
		"Texture2D		diffuseMap			: register( t0 );\n"
		"SamplerState	diffuseMapSampler	: register( s0 );\n"
		"VS_OUT VSMain( VS_IN vin )\n"
		"{\n"
		"	vin.normal.w	= 0;\n"
		"	float4 nNorm	= normalize( mul( vin.normal, world ) );\n"
		"	float4 nLight	= normalize( -lightDirection );\n"
		"\n"
		"	VS_OUT vout;\n"
		"	vout.pos		= mul( vin.pos, worldViewProjection );\n"
		"\n"
		"	vout.color		= ( diffuse * materialColor ) * max( dot( nLight, nNorm ), 0.0f );\n" // Lambert's cosine law
		"	vout.color.a	= materialColor.a;\n"
		"\n"
		"	vout.eyeVector	= cameraPosition - normalize( vout.pos );\n"
		"	vout.normal		= nNorm;\n"
		"	vout.texCoord	= vin.texCoord;\n"
		"\n"
		"	return vout;\n"
		"}\n"
		"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
		"{\n"
		"	float4	nNormal		= normalize( pin.normal );\n"
		"	float4	nLightDir	= normalize( lightDirection );\n"
		"	float4	nViewDir	= normalize( pin.eyeVector );\n"
		"	float4	NL = saturate( dot( nNormal, nLightDir ) );\n"
		"\n"
		"	float4	nReflect	= normalize( 2 * NL * nNormal - nLightDir );\n"
		"	float4	varSpecular	= pow( saturate( dot( nReflect, nViewDir ) ), 2 ) * 2;\n"
		"\n"
		"	float4	phongShade	= diffuse * NL + varSpecular * specular;\n"
		"\n"
		"	return	diffuseMap.Sample( diffuseMapSampler, pin.texCoord )\n"
		"			* phongShade\n"
		"			* materialColor\n"
		// "			* ambient\n"
		"			;\n"
		"}\n"
		;
	}
	constexpr const char *DefaultShaderNameVS()
	{
		return "DefaultStaticMeshVS";
	}
	constexpr const char *DefaultShaderNamePS()
	{
		return "DefaultStaticMeshPS";
	}
	constexpr const char *DefaultShaderEntryPointVS()
	{
		return "VSMain";
	}
	constexpr const char *DefaultShaderEntryPointPS()
	{
		return "PSMain";
	}

	constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 3>	InputElementDescs()
	{
		return std::array<D3D11_INPUT_ELEMENT_DESC, 3>
		{
			D3D11_INPUT_ELEMENT_DESC{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			D3D11_INPUT_ELEMENT_DESC{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
	}
	constexpr D3D11_RASTERIZER_DESC						RasterizerDesc( D3D11_FILL_MODE fillMode )
	{
		D3D11_RASTERIZER_DESC standard{};
		standard.FillMode				= fillMode;
		standard.CullMode				= D3D11_CULL_BACK;
		standard.FrontCounterClockwise	= FALSE;
		standard.DepthBias				= 0;
		standard.DepthBiasClamp			= 0;
		standard.SlopeScaledDepthBias	= 0;
		standard.DepthClipEnable		= TRUE;
		standard.ScissorEnable			= FALSE;
		standard.MultisampleEnable		= FALSE;
		standard.AntialiasedLineEnable	= ( fillMode == D3D11_FILL_WIREFRAME )
										? TRUE
										: FALSE;

		return standard;
	}
	constexpr D3D11_DEPTH_STENCIL_DESC					DepthStencilDesc()
	{
		D3D11_DEPTH_STENCIL_DESC standard{};
		standard.DepthEnable	= TRUE;							// default : TRUE ( Z-Test:ON )
		standard.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ALL;	// default : D3D11_DEPTH_WRITE_ALL ( Z-Write:ON, OFF is D3D11_DEPTH_WRITE_ZERO, does not means write zero! )
		standard.DepthFunc		= D3D11_COMPARISON_LESS;		// default : D3D11_COMPARISION_LESS ( ALWAYS:always pass )
		standard.StencilEnable	= FALSE;

		return standard;
	}
	constexpr D3D11_SAMPLER_DESC						SamplerDesc()
	{
		D3D11_SAMPLER_DESC standard{};
		standard.Filter			= D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		standard.AddressU		= D3D11_TEXTURE_ADDRESS_WRAP;
		standard.AddressV		= D3D11_TEXTURE_ADDRESS_WRAP;
		standard.AddressW		= D3D11_TEXTURE_ADDRESS_WRAP;
		standard.ComparisonFunc	= D3D11_COMPARISON_ALWAYS;
		standard.MinLOD			= 0;
		standard.MaxLOD			= D3D11_FLOAT32_MAX;

		/*
		standard.MipLODBias		= 0;
		standard.MaxAnisotropy	= 16;

		DirectX::XMFLOAT4 borderColor{ 0.0f, 0.0f, 0.0f, 0.0f };
		memcpy
		(
			standard.BorderColor,
			&borderColor,
			sizeof( decltype( borderColor ) )
		);
		*/

		return standard;
	}

	StaticMesh::StaticMesh( const std::wstring &objFileName, const std::string &vertexShaderCsoPath, const std::string &pixelShaderCsoPath ) :
		CSO_PATH_PS( pixelShaderCsoPath ), CSO_PATH_VS( vertexShaderCsoPath ), OBJ_FILE_PATH( objFileName ),
		iVertexBuffer(), iIndexBuffer(), iConstantBuffer(), iMaterialConstBuffer(),
		iInputLayout(), iVertexShader(), iPixelShader(),
		iRasterizerStateSurface(), iRasterizerStateWire(), iDepthStencilState(),
		subsets(),
		wasLoaded( false )
	{}
	StaticMesh::~StaticMesh()
	{
		subsets.clear();
		subsets.shrink_to_fit();
	}

	void StaticMesh::Init( const std::vector<Vertex> &connectedVertices, const std::vector<size_t> &connectedIndices, const std::vector<Subset> &argSubsets )
	{
		if ( wasLoaded ) { return; }
		// else

		HRESULT hr = S_OK;
		ID3D11Device *pDevice = Donya::GetDevice();

		// Create VertexBuffer
		{
			hr = CreateVertexBuffer<Vertex>
			(
				pDevice,
				connectedVertices,
				iVertexBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Vertex-Buffer." );
		}
		// Create IndexBuffer
		{
			hr = CreateIndexBuffer
			(
				pDevice,
				connectedIndices,
				iIndexBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Index-Buffer." );
		}
		// Create ConstantBuffers
		{
			hr = CreateConstantBuffer
			(
				pDevice,
				sizeof( ConstantBuffer ),
				iConstantBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Constant-Buffer." );

			hr = CreateConstantBuffer
			(
				pDevice,
				sizeof( MaterialConstBuffer ),
				iMaterialConstBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Material-Constant-Buffer." );
		}

		bool useDefaultShaderV = ( CSO_PATH_VS.empty() ) ? true : false;
		bool useDefaultShaderP = ( CSO_PATH_PS.empty() ) ? true : false;

		// Create VertexShader and InputLayout
		{
			auto inputElementDescs = InputElementDescs();

			if ( useDefaultShaderV )
			{
				Resource::CreateVertexShaderFromSource
				(
					pDevice,
					DefaultShaderNameVS(),
					DefaultShaderSourceCode(),
					DefaultShaderEntryPointVS(),
					iVertexShader.GetAddressOf(),
					iInputLayout.GetAddressOf(),
					inputElementDescs.data(),
					inputElementDescs.size()
				);
			}
			else
			{
				Resource::CreateVertexShaderFromCso
				(
					pDevice,
					CSO_PATH_VS.c_str(), "rb",
					iVertexShader.GetAddressOf(),
					iInputLayout.GetAddressOf(),
					inputElementDescs.data(),
					inputElementDescs.size(),
					/* enableCache = */ true
				);
			}
		}
		// Create PixelShader
		{
			if ( useDefaultShaderP )
			{
				Resource::CreatePixelShaderFromSource
				(
					pDevice,
					DefaultShaderNamePS(),
					DefaultShaderSourceCode(),
					DefaultShaderEntryPointPS(),
					iPixelShader.GetAddressOf()
				);
			}
			else
			{
				Resource::CreatePixelShaderFromCso
				(
					pDevice,
					CSO_PATH_PS.c_str(), "rb",
					iPixelShader.GetAddressOf(),
					/* enableCache = */ true
				);
			}
		}

		// Create Rasterizer States
		{
			D3D11_RASTERIZER_DESC rsDescWire	= RasterizerDesc( D3D11_FILL_WIREFRAME	);
			D3D11_RASTERIZER_DESC rsDescSurface	= RasterizerDesc( D3D11_FILL_SOLID		);

			hr = pDevice->CreateRasterizerState( &rsDescWire, iRasterizerStateWire.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create RasterizerState Wireframe." );

			hr = pDevice->CreateRasterizerState( &rsDescSurface, iRasterizerStateSurface.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create RasterizerState Solid." );
		}
		// Create DepthsStencilState
		{
			D3D11_DEPTH_STENCIL_DESC desc = DepthStencilDesc();

			hr = pDevice->CreateDepthStencilState
			(
				&desc,
				iDepthStencilState.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), "Failed : Create DepthStencilState." );
		}
		// Load Textures
		{
			constexpr D3D11_SAMPLER_DESC samplerDesc = SamplerDesc();

			auto CreateSamplerAndTextures = [&]( StaticMesh::Material *pMtl )
			{
				size_t textureCount = pMtl->textures.size();
				if ( !textureCount )
				{
					pMtl->iSampler = Resource::RequireInvalidSamplerStateComPtr();

					StaticMesh::Material::Texture dummy{};
					Resource::CreateUnicolorTexture
					(
						pDevice,
						dummy.iSRV.GetAddressOf(),
						&dummy.texture2DDesc
					);

					pMtl->textures.push_back( dummy );

					return;	// Escape from lambda-expression.
				}
				// else

				Resource::CreateSamplerState
				(
					pDevice,
					&pMtl->iSampler,
					samplerDesc
				);
				for ( size_t i = 0; i < textureCount; ++i )
				{
					auto &tex = pMtl->textures[i];
					Resource::CreateTexture2DFromFile
					(
						pDevice,
						Donya::MultiToWide( tex.fileName ),
						tex.iSRV.GetAddressOf(),
						&tex.texture2DDesc
					);
				}
			};

			subsets = argSubsets;
			for ( auto &it : subsets )
			{
				CreateSamplerAndTextures( &it.ambient	);
				CreateSamplerAndTextures( &it.bump		);
				CreateSamplerAndTextures( &it.diffuse	);
				CreateSamplerAndTextures( &it.emissive	);
				CreateSamplerAndTextures( &it.specular	);
			}
		}

		wasLoaded = true;
	}

	bool StaticMesh::LoadObjFile()
	{
		if ( wasLoaded ) { return false; }
		if ( !Donya::IsExistFile( OBJ_FILE_PATH ) ) { return false; }
		// else

		HRESULT hr = S_OK;
		ID3D11Device *pDevice = Donya::GetDevice();

		std::vector<Vertex>		vertices{};
		std::vector<size_t>		indices{};

		#pragma region Load OBJ-file

		// Load obj-file
		{
			std::vector<XMFLOAT3>			positions{};
			std::vector<XMFLOAT3>			normals{};
			std::vector<XMFLOAT2>			texCoords{};
			std::vector<Resource::Material>	materials{};

			Resource::LoadObjFile
			(
				pDevice, OBJ_FILE_PATH,
				&positions, &normals, &texCoords, &indices, &materials,
				nullptr
			);

			// Store positions and normals to vertices.
			{
				size_t end = std::min( positions.size(), std::min( normals.size(), texCoords.size() ) );
				vertices.resize( end );
				for ( size_t i = 0; i < end; ++i )
				{
					vertices[i].pos			= positions[i];
					vertices[i].normal		= normals[i];
					vertices[i].texCoord	= texCoords[i];
				}
			}

			// Store material data.
			{
				auto ConvertColor = []( const float( &color )[3] )->XMFLOAT4
				{
					return XMFLOAT4
					{
						color[0],
						color[1],
						color[2],
						1.0f
					};
				};

				size_t mtlCount = materials.size();
				subsets.resize( mtlCount );
				for ( size_t i = 0; i < mtlCount; ++i )
				{
					auto &subset = subsets[i];
					auto &mtl    = materials[i];

					subset.indexCount = mtl.indexCount;
					subset.indexStart = mtl.indexStart;

					subset.ambient.color = ConvertColor( mtl.ambient );
					subset.diffuse.color = ConvertColor( mtl.diffuse );
					subset.specular.color = ConvertColor( mtl.specular );
					subset.specular.color.w = mtl.shininess;

					subset.diffuse.textures.emplace_back();
					subset.diffuse.textures.back().fileName = Donya::WideToMulti( mtl.diffuseMap.mapName );
				}
			}
		}

		#pragma endregion

		#pragma region Create Buffers
		// Create VertexBuffer
		{
			hr = CreateVertexBuffer<Vertex>
			(
				pDevice,
				vertices,
				iVertexBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Vertex-Buffer." );
		}
		// Create IndexBuffer
		{
			hr = CreateIndexBuffer
			(
				pDevice,
				indices,
				iIndexBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Index-Buffer." );
		}
		// Create ConstantBuffers
		{
			hr = CreateConstantBuffer
			(
				pDevice,
				sizeof( ConstantBuffer ),
				iConstantBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Constant-Buffer." );

			hr = CreateConstantBuffer
			(
				pDevice,
				sizeof( MaterialConstBuffer ),
				iMaterialConstBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Material-Constant-Buffer." );
		}
		#pragma endregion

		#pragma region Create Shaders
		// Create VertexShader and InputLayout
		{
			D3D11_INPUT_ELEMENT_DESC d3d11InputElementsDesc[] =
			{
				{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			Resource::CreateVertexShaderFromCso
			(
				pDevice,
				CSO_PATH_VS.c_str(), "rb",
				iVertexShader.GetAddressOf(),
				iInputLayout.GetAddressOf(),
				const_cast<D3D11_INPUT_ELEMENT_DESC *>( d3d11InputElementsDesc ),
				_countof( d3d11InputElementsDesc ),
				/* enableCache = */ true
			);
		}
		// Create PixelShader
		{
			Resource::CreatePixelShaderFromCso
			(
				pDevice,
				CSO_PATH_PS.c_str(), "rb",
				iPixelShader.GetAddressOf(),
				/* enableCache = */ true
			);
		}
		#pragma endregion

		// Create Rasterizer States
		{
			D3D11_RASTERIZER_DESC rsDescWire	= RasterizerDesc( D3D11_FILL_WIREFRAME	);
			D3D11_RASTERIZER_DESC rsDescSurface	= RasterizerDesc( D3D11_FILL_SOLID		);

			hr = pDevice->CreateRasterizerState( &rsDescWire, iRasterizerStateWire.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create RasterizerState Wireframe." );

			hr = pDevice->CreateRasterizerState( &rsDescSurface, iRasterizerStateSurface.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create RasterizerState Solid." );
		}
		// Create DepthsStencilState
		{
			D3D11_DEPTH_STENCIL_DESC desc = DepthStencilDesc();

			hr = pDevice->CreateDepthStencilState
			(
				&desc,
				iDepthStencilState.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), "Failed : Create DepthStencilState." );
		}
		// Load Textures
		{
			constexpr D3D11_SAMPLER_DESC samplerDesc = SamplerDesc();

			auto CreateSamplerAndTextures = [&]( StaticMesh::Material *pMtl )
			{
				size_t textureCount = pMtl->textures.size();
				if ( !textureCount )
				{
					pMtl->iSampler = Resource::RequireInvalidSamplerStateComPtr();

					StaticMesh::Material::Texture dummy{};
					Resource::CreateUnicolorTexture
					(
						pDevice,
						dummy.iSRV.GetAddressOf(),
						&dummy.texture2DDesc
					);

					pMtl->textures.push_back( dummy );

					return;	// Escape from lambda-expression.
				}
				// else

				Resource::CreateSamplerState
				(
					pDevice,
					&pMtl->iSampler,
					samplerDesc
				);
				for ( size_t i = 0; i < textureCount; ++i )
				{
					auto &tex = pMtl->textures[i];
					Resource::CreateTexture2DFromFile
					(
						pDevice,
						Donya::MultiToWide( tex.fileName ),
						tex.iSRV.GetAddressOf(),
						&tex.texture2DDesc
					);
				}
			};

			for ( auto &it : subsets )
			{
				CreateSamplerAndTextures( &it.ambient	);
				CreateSamplerAndTextures( &it.bump		);
				CreateSamplerAndTextures( &it.diffuse	);
				CreateSamplerAndTextures( &it.emissive	);
				CreateSamplerAndTextures( &it.specular	);
			}
		}

		wasLoaded = true;
		return true;
	}

	void StaticMesh::Render( const DirectX::XMFLOAT4X4 &worldViewProjection, const DirectX::XMFLOAT4X4 &world, const DirectX::XMFLOAT4 &lightDirection, const DirectX::XMFLOAT4 &materialColor, const DirectX::XMFLOAT4 &cameraPos, bool isEnableFill ) const
	{
		if ( !wasLoaded ) { return; }
		// else

		HRESULT hr = S_OK;
		ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();

		// Common Settings
		{
			// Update ConstantBuffer.
			{
				ConstantBuffer cb;
				cb.worldViewProjection	= worldViewProjection;
				cb.world				= world;
				cb.lightDirection		= lightDirection;
				cb.materialColor		= materialColor;
				cb.cameraPos			= cameraPos;

				pImmediateContext->UpdateSubresource( iConstantBuffer.Get(), 0, nullptr, &cb, 0, 0 );

			}
			pImmediateContext->VSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );
			pImmediateContext->PSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );

			UINT stride = sizeof( Vertex );
			UINT offset = 0;
			pImmediateContext->IASetVertexBuffers( 0, 1, iVertexBuffer.GetAddressOf(), &stride, &offset );
			pImmediateContext->IASetIndexBuffer( iIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );
			pImmediateContext->IASetInputLayout( iInputLayout.Get() );
			pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

			pImmediateContext->VSSetShader( iVertexShader.Get(), nullptr, 0 );

			ID3D11RasterizerState	*ppRasterizerState
									= ( isEnableFill )
									? iRasterizerStateSurface.Get()
									: iRasterizerStateWire.Get();
			pImmediateContext->RSSetState( ppRasterizerState );

			pImmediateContext->PSSetShader( iPixelShader.Get(), nullptr, 0 );

			pImmediateContext->OMSetDepthStencilState( iDepthStencilState.Get(), 0xffffffff );
		}

		for ( const auto &it : subsets )
		{
			// Update Material Constant Buffer.
			{
				MaterialConstBuffer mtlCB;
				mtlCB.ambient  = it.ambient.color;
				mtlCB.diffuse  = it.diffuse.color;
				mtlCB.specular = it.specular.color;

				pImmediateContext->UpdateSubresource( iMaterialConstBuffer.Get(), 0, nullptr, &mtlCB, 0, 0 );

			}
			pImmediateContext->VSSetConstantBuffers( 1, 1, iMaterialConstBuffer.GetAddressOf() );
			pImmediateContext->PSSetConstantBuffers( 1, 1, iMaterialConstBuffer.GetAddressOf() );

			// Note:Currently support only diffuse, and only one texture.
			if ( it.diffuse.textures.empty() ) { continue; }
			// else

			pImmediateContext->PSSetSamplers( 0, 1, it.diffuse.iSampler.GetAddressOf() );
			pImmediateContext->PSSetShaderResources( 0, 1, it.diffuse.textures[0].iSRV.GetAddressOf() );

			pImmediateContext->DrawIndexed( it.indexCount, it.indexStart, 0 );
		}
	}
}