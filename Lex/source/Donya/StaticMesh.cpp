#include "StaticMesh.h"

#include <algorithm>	// Use std::min().
#include <array>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>

#include "Color.h"
#include "Constant.h"
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
	// TODO : User can be specify the ID3D11Device when create mesh.

	std::shared_ptr<StaticMesh> StaticMesh::Create( const Loader &loader )
	{
		std::shared_ptr<StaticMesh> pInstance{};

		const std::vector<Loader::Mesh> *pLoadedMeshes = loader.GetMeshes();
		size_t loadedMeshCount = pLoadedMeshes->size();

		// These will be pass to Init().
		std::vector<std::vector<Vertex>> verticesPerMesh{};
		std::vector<std::vector<size_t>> indicesPerMesh{};
		std::vector<Mesh> meshes{};	// Store data of coordinateConversion, globalTransform, subsets.
		meshes.resize( loadedMeshCount );
		for ( size_t i = 0; i < loadedMeshCount; ++i )
		{
			auto &loadedMesh	= ( *pLoadedMeshes )[i];
			auto &currentMesh	= meshes[i];

			currentMesh.coordinateConversion	= loadedMesh.coordinateConversion;
			currentMesh.globalTransform		= loadedMesh.globalTransform;

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

			auto &currentSubsets = currentMesh.subsets;

			size_t subsetCount = loadedMesh.subsets.size();
			currentSubsets.resize( subsetCount );
			for ( size_t j = 0; j < subsetCount; ++j )
			{
				auto &loadedSubset		= loadedMesh.subsets[j];
				auto &mySubset			= currentSubsets[j];

				mySubset.indexStart		= loadedSubset.indexStart;
				mySubset.indexCount		= loadedSubset.indexCount;
				mySubset.transparency	= loadedSubset.transparency;

				const std::string fileDirectory = loader.GetFileDirectory();

				auto FetchMaterialContain =
				[&fileDirectory]( StaticMesh::Material *meshMtl, const Loader::Material &loadedMtl )
				{
					meshMtl->color.x = loadedMtl.color.x;
					meshMtl->color.y = loadedMtl.color.y;
					meshMtl->color.z = loadedMtl.color.z;
					meshMtl->color.w = 1.0f;

					size_t texCount = loadedMtl.relativeTexturePaths.size();
					meshMtl->textures.resize( texCount );
					for ( size_t i = 0; i < texCount; ++i )
					{
						meshMtl->textures[i].fileName = fileDirectory + loadedMtl.relativeTexturePaths[i];
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

		pInstance = std::make_shared<StaticMesh>();
		pInstance->Init( verticesPerMesh, indicesPerMesh, meshes );

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
		"	float4 normal		: NORMAL;\n"
		"	float2 texCoord		: TEXCOORD0;\n"
		// "	float4 color		: COLOR;\n"
		// "	float4 eyeVector	: TEXCOORD0;\n"
		"};\n"
		"cbuffer CONSTANT_BUFFER : register( b0 )\n"
		"{\n"
		"	row_major\n"
		"	float4x4	worldViewProjection;\n"
		"	row_major\n"
		"	float4x4	world;\n"
		"	float4		lightDirection;\n"
		"	float4		lightColor;\n"
		"	float4		materialColor;\n"
		// "	float4		cameraPosition;\n"
		"};\n"
		"cbuffer MATERIAL_BUFFER : register( b1 )\n"
		"{\n"
		"	float4 ambient;\n"
		"	float4 diffuse;\n"
		"	float4 specular;\n"
		"};\n"
		"VS_OUT VSMain( VS_IN vin )\n"
		"{\n"
		"	vin.normal.w	= 0;\n"
		"	float4 nNorm	= normalize( mul( vin.normal, world ) );\n"
		"	float4 nLight	= normalize( -lightDirection );\n"
		"\n"
		"	VS_OUT vout		= (VS_OUT)( 0 );\n"
		"	vout.pos		= mul( vin.pos, worldViewProjection );\n"
		"\n"
		// "	vout.eyeVector	= cameraPosition - normalize( vout.pos );\n"
		"\n"
		"	vout.normal		= nNorm;\n"
		"	vout.texCoord	= vin.texCoord;\n"
		"\n"
		"	return vout;\n"
		"}\n"
		"\n"
		"Texture2D		diffuseMap			: register( t0 );\n"
		"SamplerState	diffuseMapSampler	: register( s0 );\n"
		"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
		"{\n"
		"	float3	nLightDir	= normalize( -lightDirection.rgb );\n"
		"	float	NL			= saturate( dot( pin.normal.rgb, nLightDir ) );\n"
		"			NL			= NL * 0.5f + 0.5f;\n"
		"\n"
		"	float4	diffuseColor= ( diffuse * materialColor ) * NL;\n"
		"	float4	sampleColor	= diffuseMap.Sample( diffuseMapSampler, pin.texCoord );\n"
		"\n"
		"	float3	outputColor	= sampleColor.rgb * diffuseColor.rgb;\n"
		"	float3	light		= lightColor.rgb * lightColor.w;\n"
		"	return	float4( saturate( outputColor + ambient ) * light, sampleColor.a );\n"
		"\n"
		/*
		"	float4	nNormal		= normalize( pin.normal );\n"
		"	float4	nLightDir	= normalize( lightDirection );\n"
		"	float4	nViewDir	= normalize( pin.eyeVector );\n"
		"	float4	NL			= saturate( dot( nNormal, nLightDir ) );\n"
		"\n"
		"	float4	nReflect	= normalize( 2 * NL * nNormal - nLightDir );\n"
		"	float4	varSpecular	= pow( saturate( dot( nReflect, nViewDir ) ), 2 ) * 2;\n"
		"\n"
		"	float4	phongShade	= diffuse * NL + varSpecular * specular;\n"
		"\n"
		"	return	diffuseMap.Sample( diffuseMapSampler, pin.texCoord )\n"
		// "			* phongShade\n"
		"			* materialColor\n"
		"			;\n"
		*/
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

	StaticMesh::StaticMesh() :
		iDefaultCBuffer(), iDefaultMaterialCBuffer(),
		iDefaultInputLayout(), iDefaultVS(), iDefaultPS(),
		iRasterizerStateSurface(), iRasterizerStateWire(), iDepthStencilState(),
		meshes(),
		wasLoaded( false )
	{}
	StaticMesh::~StaticMesh()
	{
		meshes.clear();
		meshes.shrink_to_fit();
	}

	void StaticMesh::CreateDefaultSettings( ID3D11Device *pDevice )
	{
		_ASSERT_EXPR( pDevice, L"Error : Passed pointer is null !" );

		HRESULT hr = S_OK;

		// Create ConstantBuffers
		{
			hr = CreateConstantBuffer
			(
				pDevice,
				sizeof( ConstantBuffer ),
				iDefaultCBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Constant-Buffer." );

			hr = CreateConstantBuffer
			(
				pDevice,
				sizeof( MaterialConstBuffer ),
				iDefaultMaterialCBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Material-Constant-Buffer." );
		}

	#if DEBUG_MODE
		constexpr bool ENABLE_CACHE = true;
	#else
		constexpr bool ENABLE_CACHE = false;
	#endif // DEBUG_MODE

		// Create VertexShader and InputLayout
		{
			auto inputElementDescs = InputElementDescs();

			Resource::CreateVertexShaderFromSource
			(
				pDevice,
				DefaultShaderNameVS(),
				DefaultShaderSourceCode(),
				DefaultShaderEntryPointVS(),
				iDefaultVS.GetAddressOf(),
				iDefaultInputLayout.GetAddressOf(),
				inputElementDescs.data(),
				inputElementDescs.size(),
				ENABLE_CACHE
			);
		}

		// Create PixelShader
		{
			Resource::CreatePixelShaderFromSource
			(
				pDevice,
				DefaultShaderNamePS(),
				DefaultShaderSourceCode(),
				DefaultShaderEntryPointPS(),
				iDefaultPS.GetAddressOf(),
				ENABLE_CACHE
			);
		}
	}
	void StaticMesh::CreateRasterizerState( ID3D11Device *pDevice )
	{
		_ASSERT_EXPR( pDevice, L"Error : Passed pointer is null !" );

		HRESULT hr = S_OK;

		D3D11_RASTERIZER_DESC rsDescWire	= RasterizerDesc( D3D11_FILL_WIREFRAME	);
		D3D11_RASTERIZER_DESC rsDescSurface	= RasterizerDesc( D3D11_FILL_SOLID		);

		hr = pDevice->CreateRasterizerState( &rsDescWire, iRasterizerStateWire.GetAddressOf() );
		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create RasterizerState Wireframe." );

		hr = pDevice->CreateRasterizerState( &rsDescSurface, iRasterizerStateSurface.GetAddressOf() );
		_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create RasterizerState Solid." );
	}
	void StaticMesh::CreateDepthStencilState( ID3D11Device *pDevice )
	{
		_ASSERT_EXPR( pDevice, L"Error : Passed pointer is null !" );

		HRESULT hr = S_OK;

		D3D11_DEPTH_STENCIL_DESC desc = DepthStencilDesc();

		hr = pDevice->CreateDepthStencilState
		(
			&desc,
			iDepthStencilState.GetAddressOf()
		);
		_ASSERT_EXPR( SUCCEEDED( hr ), "Failed : Create DepthStencilState." );
	}
	void StaticMesh::LoadTextures( ID3D11Device *pDevice )
	{
		constexpr D3D11_SAMPLER_DESC samplerDesc = SamplerDesc();

		auto CreateSamplerAndTextures = [&pDevice, &samplerDesc]( StaticMesh::Material *pMtl )
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

		#if DEBUG_MODE
			constexpr bool ENABLE_CACHE = true;
		#else
			constexpr bool ENABLE_CACHE = false;
		#endif // DEBUG_MODE

			Resource::CreateSamplerState
			(
				pDevice,
				&pMtl->iSampler,
				samplerDesc,
				ENABLE_CACHE
			);
			for ( size_t i = 0; i < textureCount; ++i )
			{
				auto &tex = pMtl->textures[i];
				Resource::CreateTexture2DFromFile
				(
					pDevice,
					Donya::MultiToWide( tex.fileName ),
					tex.iSRV.GetAddressOf(),
					&tex.texture2DDesc,
					ENABLE_CACHE
				);
			}
		};

		for ( auto &mesh : meshes )
		{
			for ( auto &subset : mesh.subsets )
			{
				CreateSamplerAndTextures( &subset.ambient	);
				CreateSamplerAndTextures( &subset.bump		);
				CreateSamplerAndTextures( &subset.diffuse	);
				CreateSamplerAndTextures( &subset.emissive	);
				CreateSamplerAndTextures( &subset.specular	);
			}
		}
	}

	void StaticMesh::Init( const std::vector<std::vector<Vertex>> &verticesPerMesh, const std::vector<std::vector<size_t>> &indicesPerMesh, const std::vector<Mesh> &loadedMeshes )
	{
		if ( wasLoaded ) { return; }
		// else

		HRESULT hr = S_OK;
		ID3D11Device *pDevice = Donya::GetDevice();

		// Copy variables, but ComPtr is not valid yet.
		meshes = loadedMeshes;
		const size_t MESH_COUNT = loadedMeshes.size();

		// Create VertexBuffer
		for ( size_t i = 0; i < MESH_COUNT; ++i )
		{
			hr = CreateVertexBuffer<Vertex>
			(
				pDevice,
				verticesPerMesh[i],
				meshes[i].iVertexBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Vertex-Buffer." );
		}
		// Create IndexBuffer
		for ( size_t i = 0; i < MESH_COUNT; ++i )
		{
			hr = CreateIndexBuffer
			(
				pDevice,
				indicesPerMesh[i],
				meshes[i].iIndexBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Index-Buffer." );
		}

		CreateDefaultSettings( pDevice );

		CreateRasterizerState( pDevice );

		CreateDepthStencilState( pDevice );

		LoadTextures( pDevice );

		wasLoaded = true;
	}

	bool StaticMesh::LoadObjFile( const std::wstring &objFilePath )
	{
		if ( wasLoaded ) { return false; }
		if ( !Donya::IsExistFile( objFilePath ) ) { return false; }
		// else

		HRESULT hr = S_OK;
		ID3D11Device *pDevice = Donya::GetDevice();

		// Use only one mesh in obj-file.
		meshes.clear();
		meshes.emplace_back();
		auto &mesh = meshes.back();

		std::vector<Vertex>		vertices{};
		std::vector<size_t>		indices{};

		// Load obj-file
		{
			std::vector<XMFLOAT3>			positions{};
			std::vector<XMFLOAT3>			normals{};
			std::vector<XMFLOAT2>			texCoords{};
			std::vector<Resource::Material>	materials{};

			Resource::LoadObjFile
			(
				pDevice, objFilePath,
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
				mesh.subsets.resize( mtlCount );
				for ( size_t i = 0; i < mtlCount; ++i )
				{
					auto &subset = mesh.subsets[i];
					auto &mtl    = materials[i];

					subset.indexCount		= mtl.indexCount;
					subset.indexStart		= mtl.indexStart;

					subset.ambient.color	= ConvertColor( mtl.ambient );
					subset.diffuse.color	= ConvertColor( mtl.diffuse );
					subset.specular.color	= ConvertColor( mtl.specular );
					subset.specular.color.w	= mtl.shininess;

					subset.diffuse.textures.emplace_back();
					subset.diffuse.textures.back().fileName = Donya::WideToMulti( mtl.diffuseMap.mapName );
				}
			}
		}

		// Create VertexBuffer
		{
			hr = CreateVertexBuffer<Vertex>
			(
				pDevice,
				vertices,
				mesh.iVertexBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Vertex-Buffer." );
		}
		// Create IndexBuffer
		{
			hr = CreateIndexBuffer
			(
				pDevice,
				indices,
				mesh.iIndexBuffer.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Index-Buffer." );
		}

		CreateDefaultSettings( pDevice );

		CreateRasterizerState( pDevice );

		CreateDepthStencilState( pDevice );

		LoadTextures( pDevice );

		wasLoaded = true;
		return true;
	}

	void StaticMesh::Render( ID3D11DeviceContext *pImmediateContext, bool useDefaultShading, bool isEnableFill, const XMFLOAT4X4 &defMatWVP, const XMFLOAT4X4 &defMatW, const XMFLOAT4 &defLightDir, const XMFLOAT4 &defMtlColor ) const
	{
		if ( !wasLoaded ) { return; }
		// else

		HRESULT hr = S_OK;

		// Use default context.
		if ( !pImmediateContext )
		{
			pImmediateContext = Donya::GetImmediateContext();
		}

		// Common Settings
		{
			pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

			if ( useDefaultShading )
			{
				pImmediateContext->IASetInputLayout( iDefaultInputLayout.Get() );
				pImmediateContext->VSSetShader( iDefaultVS.Get(), nullptr, 0 );
			}

			ID3D11RasterizerState	*ppRasterizerState
									= ( isEnableFill )
									? iRasterizerStateSurface.Get()
									: iRasterizerStateWire.Get();
			pImmediateContext->RSSetState( ppRasterizerState );

			if ( useDefaultShading )
			{
				pImmediateContext->PSSetShader( iDefaultPS.Get(), nullptr, 0 );
			}

			pImmediateContext->OMSetDepthStencilState( iDepthStencilState.Get(), 0xffffffff );
		}

		for ( const auto &mesh : meshes )
		{
			// Update ConstantBuffer.
			if ( useDefaultShading )
			{
				auto Mul4x4 = []( const XMFLOAT4X4 &lhs, const XMFLOAT4X4 &rhs ) ->DirectX::XMFLOAT4X4
				{
					DirectX::XMFLOAT4X4 rv{};
					DirectX::XMStoreFloat4x4
					(
						&rv,
						DirectX::XMLoadFloat4x4( &lhs ) * DirectX::XMLoadFloat4x4( &rhs )
					);
					return rv;
				};
				const XMFLOAT4X4 &coordinateConversion	= mesh.coordinateConversion;
				const XMFLOAT4X4 &globalTransform		= mesh.globalTransform;

				ConstantBuffer cb;
				cb.worldViewProjection	= Mul4x4( Mul4x4( coordinateConversion, globalTransform ), defMatWVP );
				cb.world				= Mul4x4( Mul4x4( coordinateConversion, globalTransform ), defMatW   );
				cb.lightDirection		= defLightDir;
				cb.lightColor			= XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
				cb.materialColor		= defMtlColor;
				cb.materialColor.w		= Donya::Color::FilteringAlpha( cb.materialColor.w );

				pImmediateContext->UpdateSubresource( iDefaultCBuffer.Get(), 0, nullptr, &cb, 0, 0 );
				pImmediateContext->VSSetConstantBuffers( 0, 1, iDefaultCBuffer.GetAddressOf() );
				pImmediateContext->PSSetConstantBuffers( 0, 1, iDefaultCBuffer.GetAddressOf() );
			}

			UINT stride = sizeof( Vertex );
			UINT offset = 0;
			pImmediateContext->IASetVertexBuffers( 0, 1, mesh.iVertexBuffer.GetAddressOf(), &stride, &offset );
			pImmediateContext->IASetIndexBuffer( mesh.iIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );

			for ( const auto &it : mesh.subsets )
			{
				// Update Material Constant Buffer.
				if ( useDefaultShading )
				{
					MaterialConstBuffer mtlCB;
					mtlCB.ambient  = it.ambient.color;
					mtlCB.diffuse  = it.diffuse.color;
					mtlCB.specular = it.specular.color;

					pImmediateContext->UpdateSubresource( iDefaultMaterialCBuffer.Get(), 0, nullptr, &mtlCB, 0, 0 );
					pImmediateContext->VSSetConstantBuffers( 1, 1, iDefaultMaterialCBuffer.GetAddressOf() );
					pImmediateContext->PSSetConstantBuffers( 1, 1, iDefaultMaterialCBuffer.GetAddressOf() );
				}

				// Note:Currently support only diffuse, and only one texture.
				if ( it.diffuse.textures.empty() ) { continue; }
				// else

				pImmediateContext->PSSetSamplers( 0, 1, it.diffuse.iSampler.GetAddressOf() );
				pImmediateContext->PSSetShaderResources( 0, 1, it.diffuse.textures[0].iSRV.GetAddressOf() );

				pImmediateContext->DrawIndexed( it.indexCount, it.indexStart, 0 );
			}
		}
	}
}
