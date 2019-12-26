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

	bool StaticMesh::Create( const Loader &loader, StaticMesh &outputInstance )
	{
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

		const std::vector<Loader::Face> *pLoadedFaces = loader.GetCollisionFaces();
		const size_t faceCount = pLoadedFaces->size();

		std::vector<StaticMesh::Face> faces{ faceCount };
		for ( size_t i = 0; i < faceCount; ++i )
		{
			faces[i].materialIndex	= ( *pLoadedFaces )[i].materialIndex;
			faces[i].points			= ( *pLoadedFaces )[i].points;
		}

		return outputInstance.Init( verticesPerMesh, indicesPerMesh, meshes, faces );
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
		meshes(), collisionFaces(),
		wasLoaded( false )
	{}
	StaticMesh::~StaticMesh()
	{
		meshes.clear();
		meshes.shrink_to_fit();
		collisionFaces.clear();
		collisionFaces.shrink_to_fit();
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

	bool StaticMesh::Init( const std::vector<std::vector<Vertex>> &verticesPerMesh, const std::vector<std::vector<size_t>> &indicesPerMesh, const std::vector<Mesh> &loadedMeshes, const std::vector<Face> &loadedFaces )
	{
		if ( wasLoaded ) { return false; }
		// else

		HRESULT hr = S_OK;
		ID3D11Device *pDevice = Donya::GetDevice();

		// Copy variables, but ComPtr is not valid yet.
		meshes = loadedMeshes;
		const size_t MESH_COUNT = loadedMeshes.size();

		collisionFaces = loadedFaces;

		// Create VertexBuffer
		for ( size_t i = 0; i < MESH_COUNT; ++i )
		{
			hr = CreateVertexBuffer<Vertex>
			(
				pDevice, verticesPerMesh[i],
				D3D11_USAGE_IMMUTABLE, 0,
				meshes[i].iVertexBuffer.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create Vertex-Buffer." );
				return false;
			}
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
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create Index-Buffer." );
				return false;
			}
		}

		// TODO : Check the error of these method.

		CreateDefaultSettings( pDevice );

		CreateRasterizerState( pDevice );

		CreateDepthStencilState( pDevice );

		LoadTextures( pDevice );

		wasLoaded = true;
		return true;
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
				pDevice, vertices,
				D3D11_USAGE_IMMUTABLE, 0,
				mesh.iVertexBuffer.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create Vertex-Buffer." );
				return false;
			}
		}
		// Create IndexBuffer
		{
			hr = CreateIndexBuffer
			(
				pDevice,
				indices,
				mesh.iIndexBuffer.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create Index-Buffer." );
				return false;
			}
		}

		CreateDefaultSettings( pDevice );

		CreateRasterizerState( pDevice );

		CreateDepthStencilState( pDevice );

		LoadTextures( pDevice );

		wasLoaded = true;
		return true;
	}

	void StaticMesh::Render( ID3D11DeviceContext *pImmediateContext, bool useDefaultShading, bool isEnableFill, const Donya::Vector4x4 &defMatWVP, const Donya::Vector4x4 &defMatW, const Donya::Vector4 &defLightDir, const Donya::Vector4 &defMtlColor ) const
	{
		if ( !wasLoaded ) { return; }
		// else

		HRESULT hr = S_OK;

		// Use default context.
		if ( !pImmediateContext )
		{
			pImmediateContext = Donya::GetImmediateContext();
		}

		// For PostProcessing.
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>	prevRasterizerState;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		prevVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		prevPS;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>		prevSamplerState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	prevDepthStencilState;
		{
			pImmediateContext->RSGetState( prevRasterizerState.ReleaseAndGetAddressOf() );
			pImmediateContext->VSGetShader( prevVS.GetAddressOf(), 0, 0 );
			pImmediateContext->PSGetShader( prevPS.GetAddressOf(), 0, 0 );
			pImmediateContext->PSGetSamplers( 0, 1, prevSamplerState.ReleaseAndGetAddressOf() );
			pImmediateContext->OMGetDepthStencilState( prevDepthStencilState.ReleaseAndGetAddressOf(), 0 );
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
				const Donya::Vector4x4 globalAdjusted = mesh.globalTransform * mesh.coordinateConversion;

				ConstantBuffer cb{};
				cb.worldViewProjection	= ( globalAdjusted * defMatWVP ).XMFloat();
				cb.world				= ( globalAdjusted * defMatW   ).XMFloat();
				cb.lightDirection		= defLightDir.XMFloat();
				cb.lightColor			= XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
				cb.materialColor		= defMtlColor.XMFloat();
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

				// TODO : To be changeable the slot of sampler, SRV. Support multi texture.

				// Note:Currently support only diffuse, and only one texture.
				if ( it.diffuse.textures.empty() ) { continue; }
				// else

				pImmediateContext->PSSetSamplers( 0, 1, it.diffuse.iSampler.GetAddressOf() );
				pImmediateContext->PSSetShaderResources( 0, 1, it.diffuse.textures[0].iSRV.GetAddressOf() );

				pImmediateContext->DrawIndexed( it.indexCount, it.indexStart, 0 );
			}
		}

		// PostProcessing
		{
			ID3D11ShaderResourceView *pNullSRV = nullptr;

			pImmediateContext->RSSetState( prevRasterizerState.Get() );

			if ( useDefaultShading )
			{
				pImmediateContext->IASetInputLayout( 0 );
				pImmediateContext->VSSetShader( prevVS.Get(), nullptr, 0 );
				pImmediateContext->PSSetShader( prevPS.Get(), nullptr, 0 );

				ID3D11Buffer *nullBuffer{};
				pImmediateContext->VSSetConstantBuffers( 0, 1, &nullBuffer );
				pImmediateContext->PSSetConstantBuffers( 0, 1, &nullBuffer );
				pImmediateContext->VSSetConstantBuffers( 1, 1, &nullBuffer );
				pImmediateContext->PSSetConstantBuffers( 1, 1, &nullBuffer );
			}

			pImmediateContext->PSSetShaderResources( 0, 1, &pNullSRV );
			pImmediateContext->PSSetSamplers( 0, 1, prevSamplerState.GetAddressOf() );

			pImmediateContext->OMSetDepthStencilState( prevDepthStencilState.Get(), 1 );
		}
	}

	StaticMesh::RayPickResult StaticMesh::RayPick( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, bool enoughOnlyPickFirst )
	{
		RayPickResult rpResult{};

		const Donya::Vector3 rayVec  = rayEnd - rayStart;
		const Donya::Vector3 nRayVec = rayVec.Normalized();

		float nearestDistance = rayVec.Length();
		Donya::Vector3 faceNormal{};			// Does not normalized.
		std::array<Donya::Vector3, 3> faces{};	// [0:A][1:B][2:C]. CW.
		std::array<Donya::Vector3, 3> edges{};	// [0:AB][1:BC][2:CA]. Edges of face.
		for ( const auto &it : collisionFaces )
		{
			// Store triangle data.
			faces		= it.points;
			edges[0]	= faces[1] - faces[0];
			edges[1]	= faces[2] - faces[1];
			edges[2]	= faces[0] - faces[2];
			faceNormal	= Donya::Vector3::Cross( edges[0], edges[1] );

			// Verify the ray vector has possibility of intersection.
			if ( 0.0f  <= Donya::Vector3::Dot( rayVec, faceNormal ) ) { continue; }
			// else

			// Distance to intersection-point from rayStart.
			float currentDistance{};
			{
				Donya::Vector3 vPV = faces[0] - rayStart;

				float dotPN = Donya::Vector3::Dot( vPV,		faceNormal );
				float dotRN = Donya::Vector3::Dot( nRayVec,	faceNormal );

				currentDistance = dotPN / ( dotRN + EPSILON /* Prevent zero-divide */ );
			}
			if ( currentDistance < 0.0f ) { continue; }
			// else

			Donya::Vector3 intersection = rayStart + ( nRayVec * currentDistance );

			// Judge the intersection-point is there inside of triangle.
			bool onOutside = false;
			for ( int i = 0; i < 3/*Vertex of triangle count*/; ++i )
			{
				Donya::Vector3 vIV		= faces[i] - intersection; // Vector of intersection-point to vertex of face.
				Donya::Vector3 crossIE	= Donya::Vector3::Cross( vIV, edges[i] );

				float dotCN = Donya::Vector3::Dot( crossIE, faceNormal );
				if (  dotCN < 0.0f )
				{
					onOutside = true;
					break;
				}
			}
			if ( onOutside ) { continue; }
			// else

			if ( nearestDistance <= currentDistance ) { continue; }
			// else
			nearestDistance = currentDistance;

			rpResult.materialIndex		= it.materialIndex;
			rpResult.distanceToIP		= currentDistance;
			rpResult.intersectionPoint	= intersection;
			rpResult.normal				= faceNormal.Normalized();
			rpResult.wasHit				= true;

			if ( enoughOnlyPickFirst ) { return rpResult; }
			// else
		}

		return rpResult;
	}

}
