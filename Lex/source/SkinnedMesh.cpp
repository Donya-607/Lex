#include "SkinnedMesh.h"

#include "Common.h"
#include "Donya.h"
#include "Loader.h"
#include "Resource.h"
#include "Useful.h"

using namespace DirectX;

namespace Donya
{
	bool SkinnedMesh::Create( const Loader *loader, std::unique_ptr<SkinnedMesh> *ppOutput )
	{
		if ( !loader || !ppOutput ) { return false; }
		// else

		const std::vector<size_t> *pIndices = loader->GetIndices();
		std::vector<Vertex> vertices;
		{
			const std::vector<Donya::Vector3> *normals   = loader->GetNormals();
			const std::vector<Donya::Vector3> *positions = loader->GetPositions();
			const std::vector<Donya::Vector2> *texCoords = loader->GetTexCoords();

			vertices.resize( max( normals->size(), positions->size() ) );
			size_t end = vertices.size();
			for ( size_t i = 0; i < end; ++i )
			{
				vertices[i].normal		= scast<DirectX::XMFLOAT3>( ( *normals   )[i] );
				vertices[i].pos			= scast<DirectX::XMFLOAT3>( ( *positions )[i] );

				if ( i < texCoords->size() )
				{
					vertices[i].texCoord = scast<DirectX::XMFLOAT2>( ( *texCoords )[i] );
				}
				else
				{
					vertices[i].texCoord = { 0, 0 };
				}
			}
		}

		std::vector<SkinnedMesh::Material> materials{};

		auto *loadedMtls = loader->GetMaterials();
		size_t mtlCount  = loadedMtls->size();

		materials.resize( mtlCount );
		for ( size_t i = 0; i < mtlCount; ++i )
		{
			materials[i].ambient		= ( *loadedMtls )[i].ambient;
			materials[i].bump			= ( *loadedMtls )[i].bump;
			materials[i].diffuse		= ( *loadedMtls )[i].diffuse;
			materials[i].emissive		= ( *loadedMtls )[i].emissive;
			materials[i].transparency	= ( *loadedMtls )[i].transparency;
			if ( ( *loadedMtls )[i].pPhong )
			{
				materials[i].shininess	= ( *loadedMtls )[i].pPhong->shininess;
				materials[i].specular	= ( *loadedMtls )[i].pPhong->specular;
			}
			else
			{
				materials[i].shininess	= 1.0f;
				materials[i].specular	= { 1.0f, 1.0f, 1.0f };
			}

			auto &texNames = ( *loadedMtls )[i].textureNames;
			auto &texContainer = materials[i].textures;

			size_t texCount = texNames.size();
			texContainer.resize( texCount );
			for ( size_t j = 0; j < texCount; ++j )
			{
				texContainer[j].fileName = ( ( *loadedMtls )[i].textureNames[j] );
			}
		}

		*ppOutput =
		std::make_unique<SkinnedMesh>
		(
			*pIndices,
			vertices,
			materials
		);

		return true;
	}

	SkinnedMesh::SkinnedMesh( const std::vector<size_t> &indices, const std::vector<Vertex> &vertices, const std::vector<Material> &loadedMaterials )
		: vertexCount( vertexCount )
	{
		HRESULT hr = S_OK;
		ID3D11Device *pDevice = Donya::GetDevice();

		// Create VertexBuffer
		{
			D3D11_BUFFER_DESC d3d11BufferDesc{};
			d3d11BufferDesc.ByteWidth				= sizeof( Vertex ) * vertices.size();
			d3d11BufferDesc.Usage					= D3D11_USAGE_IMMUTABLE;
			d3d11BufferDesc.BindFlags				= D3D11_BIND_VERTEX_BUFFER;
			d3d11BufferDesc.CPUAccessFlags			= 0;
			d3d11BufferDesc.MiscFlags				= 0;
			d3d11BufferDesc.StructureByteStride		= 0;

			D3D11_SUBRESOURCE_DATA d3d11SubResourceData{};
			d3d11SubResourceData.pSysMem			= vertices.data();
			d3d11SubResourceData.SysMemPitch		= 0;
			d3d11SubResourceData.SysMemSlicePitch	= 0;

			hr = pDevice->CreateBuffer( &d3d11BufferDesc, &d3d11SubResourceData, iVertexBuffer.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateBuffer" );
		}
		// Create IndexBuffer
		{
			D3D11_BUFFER_DESC d3dIndexBufferDesc{};
			d3dIndexBufferDesc.ByteWidth			= sizeof( size_t ) * indices.size();
			d3dIndexBufferDesc.Usage				= D3D11_USAGE_IMMUTABLE;
			d3dIndexBufferDesc.BindFlags			= D3D11_BIND_INDEX_BUFFER;
			d3dIndexBufferDesc.CPUAccessFlags		= 0;
			d3dIndexBufferDesc.MiscFlags			= 0;
			d3dIndexBufferDesc.StructureByteStride	= 0;

			D3D11_SUBRESOURCE_DATA d3dSubresourceData{};
			d3dSubresourceData.pSysMem				= indices.data();
			d3dSubresourceData.SysMemPitch			= 0;
			d3dSubresourceData.SysMemSlicePitch		= 0;

			vertexCount = indices.size();

			hr = pDevice->CreateBuffer( &d3dIndexBufferDesc, &d3dSubresourceData, iIndexBuffer.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateBuffer" );
		}
		// Create ConstantBuffers
		{
			D3D11_BUFFER_DESC d3dConstantBufferDesc{};
			d3dConstantBufferDesc.ByteWidth				= sizeof( ConstantBuffer );
			d3dConstantBufferDesc.Usage					= D3D11_USAGE_DEFAULT;
			d3dConstantBufferDesc.BindFlags				= D3D11_BIND_CONSTANT_BUFFER;
			d3dConstantBufferDesc.CPUAccessFlags		= 0;
			d3dConstantBufferDesc.MiscFlags				= 0;
			d3dConstantBufferDesc.StructureByteStride	= 0;

			hr = pDevice->CreateBuffer( &d3dConstantBufferDesc, nullptr, iConstantBuffer.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateBuffer" );

			d3dConstantBufferDesc.ByteWidth				= sizeof( MaterialConstantBuffer );

			hr = pDevice->CreateBuffer( &d3dConstantBufferDesc, nullptr, iMaterialConstantBuffer.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateBuffer" );
		}
		// Create VertexShader and InputLayout
		{
			D3D11_INPUT_ELEMENT_DESC d3d11InputElementsDesc[] =
			{
				{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			Resource::CreateVertexShaderFromCso
			(
				pDevice,
				"SkinnedMeshVS.cso", "rb",
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
				"SkinnedMeshPS.cso", "rb",
				iPixelShader.GetAddressOf(),
				/* enableCache = */ true
			);
		}
		// Create Rasterizer States
		{
			D3D11_RASTERIZER_DESC d3d11ResterizerDescBase{};
			d3d11ResterizerDescBase.CullMode					= D3D11_CULL_BACK;
			d3d11ResterizerDescBase.FrontCounterClockwise		= FALSE;
			d3d11ResterizerDescBase.DepthBias					= 0;
			d3d11ResterizerDescBase.DepthBiasClamp				= 0;
			d3d11ResterizerDescBase.SlopeScaledDepthBias		= 0;
			d3d11ResterizerDescBase.DepthClipEnable				= TRUE;
			d3d11ResterizerDescBase.ScissorEnable				= FALSE;
			d3d11ResterizerDescBase.MultisampleEnable			= FALSE;
			d3d11ResterizerDescBase.AntialiasedLineEnable		= TRUE;

			D3D11_RASTERIZER_DESC d3d11ResterizerWireDesc		= d3d11ResterizerDescBase;
			D3D11_RASTERIZER_DESC d3d11ResterizerSurfaceDesc	= d3d11ResterizerDescBase;
			d3d11ResterizerWireDesc.FillMode					= D3D11_FILL_WIREFRAME;
			d3d11ResterizerSurfaceDesc.FillMode					= D3D11_FILL_SOLID;
			d3d11ResterizerSurfaceDesc.AntialiasedLineEnable	= FALSE;

			hr = pDevice->CreateRasterizerState( &d3d11ResterizerWireDesc, iRasterizerStateWire.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateRasterizerState()" );

			hr = pDevice->CreateRasterizerState( &d3d11ResterizerSurfaceDesc, iRasterizerStateSurface.GetAddressOf() );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateRasterizerState()" );
		}
		// Create DepthsStencilState
		{
			D3D11_DEPTH_STENCIL_DESC d3dDepthStencilDesc{};
			d3dDepthStencilDesc.DepthEnable		= TRUE;
			d3dDepthStencilDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ALL;
			d3dDepthStencilDesc.DepthFunc		= D3D11_COMPARISON_LESS;
			d3dDepthStencilDesc.StencilEnable	= false;

			hr = pDevice->CreateDepthStencilState
			(
				&d3dDepthStencilDesc,
				iDepthStencilState.GetAddressOf()
			);
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateDepthStencilState" );
		}
		// Read Texture
		{
			materials = loadedMaterials;

			D3D11_SAMPLER_DESC samplerDesc{};
			samplerDesc.Filter			= D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU		= D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressV		= D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressW		= D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.ComparisonFunc	= D3D11_COMPARISON_NEVER;
			samplerDesc.MinLOD			= 0;
			samplerDesc.MaxLOD			= D3D11_FLOAT32_MAX;

			size_t mtlCount = materials.size();
			for ( size_t i = 0; i < mtlCount; ++i )
			{
				auto &mtl = materials[i];

				size_t textureCount = mtl.textures.size();
				if ( !textureCount )
				{
					SkinnedMesh::Material::Texture dummy{};
					Resource::CreateUnicolorTexture
					(
						pDevice,
						dummy.iSRV.GetAddressOf(),
						&dummy.iSampler,
						&dummy.texture2DDesc
					);

					mtl.textures.push_back( dummy );
					continue;
				}
				// else
				for ( size_t j = 0; j < textureCount; ++j )
				{
					auto &mtlTex = mtl.textures[j];

					Resource::CreateTexture2DFromFile
					(
						pDevice,
						Donya::MultiToWide( mtlTex.fileName ),
						mtlTex.iSRV.GetAddressOf(),
						mtlTex.iSampler.GetAddressOf(),
						&mtlTex.texture2DDesc,
						&samplerDesc,
						/* enableCache = */ true
					);
				}
			}
		}
	}
	SkinnedMesh::~SkinnedMesh()
	{
		materials.clear();
		materials.shrink_to_fit();
	}

	void SkinnedMesh::Render( const DirectX::XMFLOAT4X4 &worldViewProjection, const DirectX::XMFLOAT4X4 &world, const DirectX::XMFLOAT4 &eyePosition, const DirectX::XMFLOAT4 &lightColor, const DirectX::XMFLOAT4 &lightDirection, bool isEnableFill )
	{
		HRESULT hr = S_OK;
		ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();

		// Update Subresource
		{
			ConstantBuffer cb;
			cb.worldViewProjection	= worldViewProjection;
			cb.world				= world;
			cb.lightColor			= lightColor;
			cb.lightDir				= lightDirection;
			// cb.eyePosition			= eyePosition;
			pImmediateContext->UpdateSubresource( iConstantBuffer.Get(), 0, nullptr, &cb, 0, 0 );
		}

		Microsoft::WRL::ComPtr<ID3D11RasterizerState>	prevRasterizerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>		prevSamplerState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	prevDepthStencilState;
		{
			pImmediateContext->RSGetState( prevRasterizerState.ReleaseAndGetAddressOf() );
			pImmediateContext->PSGetSamplers( 0, 1, prevSamplerState.ReleaseAndGetAddressOf() );
			pImmediateContext->OMGetDepthStencilState( prevDepthStencilState.ReleaseAndGetAddressOf(), 0 );
		}

		// Settings
		{
			/*
			IA...InputAssembler
			VS...VertexShader
			HS...HullShader
			DS...DomainShader
			GS...GeometryShader
			SO...StreamOutput
			RS...RasterizerState
			PS...PixelShader
			OM...OutputMerger

			CS...ComputeShader
			*/

			UINT stride = sizeof( Vertex );
			UINT offset = 0;
			pImmediateContext->IASetVertexBuffers( 0, 1, iVertexBuffer.GetAddressOf(), &stride, &offset );
			pImmediateContext->IASetIndexBuffer( iIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );
			pImmediateContext->IASetInputLayout( iInputLayout.Get() );
			pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

			pImmediateContext->VSSetShader( iVertexShader.Get(), nullptr, 0 );
			pImmediateContext->VSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );

			ID3D11RasterizerState	*ppRasterizerState
									= ( isEnableFill )
									? iRasterizerStateSurface.Get()
									: iRasterizerStateWire.Get();
			pImmediateContext->RSSetState( ppRasterizerState );

			pImmediateContext->PSSetShader( iPixelShader.Get(), nullptr, 0 );
			pImmediateContext->PSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );

			pImmediateContext->OMSetDepthStencilState( iDepthStencilState.Get(), 0xffffffff );
		}


		size_t mtlCount = materials.size();
		for ( size_t i = 0; i < mtlCount; ++i )
		{
			auto &mtl = materials[i];

			// Update Subresource
			{
				auto ConvertFloat4 =
				[&]( const XMFLOAT3 &color )
				{
					XMFLOAT4 rv;
					rv.x = color.x;
					rv.y = color.y;
					rv.z = color.z;
					rv.w = 1.0f - mtl.transparency;

					return rv;
				};

				MaterialConstantBuffer mtlCB{};
				mtlCB.ambient	= ConvertFloat4( mtl.ambient );
				mtlCB.bump		= ConvertFloat4( mtl.bump );
				mtlCB.diffuse	= ConvertFloat4( mtl.diffuse );
				mtlCB.emissive	= ConvertFloat4( mtl.emissive );
				mtlCB.specular	= ConvertFloat4( mtl.specular );
				mtlCB.shininess	= mtl.shininess;

				pImmediateContext->UpdateSubresource( iMaterialConstantBuffer.Get(), 0, nullptr, &mtlCB, 0, 0 );
			}
			pImmediateContext->VSSetConstantBuffers( 1, 1, iMaterialConstantBuffer.GetAddressOf() );
			pImmediateContext->PSSetConstantBuffers( 1, 1, iMaterialConstantBuffer.GetAddressOf() );

			auto &mtlTex = materials[i].textures;
			size_t texCount = mtlTex.size();
			for ( size_t j = 0; j < texCount; ++j )
			{
				pImmediateContext->PSSetSamplers( 0, 1, mtlTex[j].iSampler.GetAddressOf() );
				pImmediateContext->PSSetShaderResources( 0, 1, mtlTex[j].iSRV.GetAddressOf() );

				pImmediateContext->DrawIndexed( vertexCount, 0, 0 );
			}
		}

		// PostProcessing
		{
			ID3D11ShaderResourceView *pNullSRV = nullptr;

			pImmediateContext->IASetInputLayout( 0 );

			pImmediateContext->VSSetShader( 0, 0, 0 );

			pImmediateContext->RSSetState( prevRasterizerState.Get() );

			pImmediateContext->PSSetShader( 0, 0, 0 );
			pImmediateContext->PSSetShaderResources( 0, 1, &pNullSRV );
			pImmediateContext->PSSetSamplers( 0, 1, prevSamplerState.GetAddressOf() );

			pImmediateContext->OMSetDepthStencilState( prevDepthStencilState.Get(), 1 );
		}
	}


}