#include "StaticMesh.h"

#include <fstream>
#include <sstream>
#include <string>

#include "Common.h"
#include "Donya.h"
#include "Resource.h"
#include "Vector.h"

using namespace DirectX;

#include <windows.h>

namespace Donya
{
	bool StaticMesh::Create( const Loader &loader, StaticMesh *pOutput )
	{
		if ( !pOutput ) { return false; }
		// else


	}

	StaticMesh::StaticMesh( const std::wstring &objFileName, const std::string &vertexShaderCsoPath, const std::string &pixelShaderCsoPath )
	{
		HRESULT hr = S_OK;
		ID3D11Device *pDevice = Donya::GetDevice();

		std::vector<Vertex>		vertices{};
		std::vector<XMFLOAT3>	positions{};
		std::vector<XMFLOAT3>	normals{};
		std::vector<XMFLOAT2>	texCoords{};
		std::vector<size_t>		indices{};

		#pragma region Load OBJ-file

		// Load obj-file
		{
			Resource::LoadObjFile
			(
				pDevice, objFileName,
				&positions, &normals, &texCoords, &indices, &materials,
				&isEnableTexture
			);

			// Store positions and normals to vertices.
			{
				_ASSERT_EXPR( positions.size() == normals.size(), L"Failed : Loaded obj file data error." );
				size_t end = positions.size();
				vertices.resize( end );
				for ( size_t i = 0; i < end; ++i )
				{
					vertices[i].pos			= positions[i];
					vertices[i].normal		= normals[i];
					vertices[i].texCoord	= texCoords[i];
				}
			}
		}

		#pragma endregion

		#pragma region Create Buffers
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
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateBuffer()" );
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

			hr = pDevice->CreateBuffer( &d3dIndexBufferDesc, &d3dSubresourceData, iIndexBuffer.GetAddressOf() );
			assert( SUCCEEDED( hr ) );
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
			assert( SUCCEEDED( hr ) );

			D3D11_BUFFER_DESC materialConstBufferDesc{};
			materialConstBufferDesc.ByteWidth           = sizeof( MaterialConstBuffer );
			materialConstBufferDesc.Usage               = D3D11_USAGE_DEFAULT;
			materialConstBufferDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
			materialConstBufferDesc.CPUAccessFlags      = 0;
			materialConstBufferDesc.MiscFlags           = 0;
			materialConstBufferDesc.StructureByteStride = 0;

			hr = pDevice->CreateBuffer( &materialConstBufferDesc, nullptr, iMaterialConstBuffer.GetAddressOf() );
			assert( SUCCEEDED( hr ) );
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
				vertexShaderCsoPath.c_str(), "rb",
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
				pixelShaderCsoPath.c_str(), "rb",
				iPixelShader.GetAddressOf(),
				/* enableCache = */ true
			);
		}
		#pragma endregion

		#pragma region Create States
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
			_ASSERT_EXPR( SUCCEEDED( hr ), "Failed : CreateDepthStencilState" );
		}
		#pragma endregion

	}
	StaticMesh::~StaticMesh() = default;

	void StaticMesh::Render( const DirectX::XMFLOAT4X4 &worldViewProjection, const DirectX::XMFLOAT4X4 &world, const DirectX::XMFLOAT4 &lightDirection, const DirectX::XMFLOAT4 &materialColor, const DirectX::XMFLOAT4 &cameraPos, bool isEnableFill )
	{
		HRESULT hr = S_OK;
		ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();

		#pragma region Common Settings

		{
			ConstantBuffer cb;
			cb.worldViewProjection	= worldViewProjection;
			cb.world				= world;
			cb.lightDirection		= lightDirection;
			cb.materialColor		= materialColor;
			cb.cameraPos			= cameraPos;

			pImmediateContext->UpdateSubresource( iConstantBuffer.Get(), 0, nullptr, &cb, 0, 0 );

			pImmediateContext->VSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );
			pImmediateContext->PSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );
		}

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

		#pragma endregion

		for ( const Resource::Material &it : materials )
		{
			{
				auto ConvertFloat4 = [&]( const float color[3] )->XMFLOAT4
				{
					return XMFLOAT4{ color[0], color[1], color[2], 1.0f };
				};

				MaterialConstBuffer mtlConstBuffer;
				mtlConstBuffer.ambient  = ConvertFloat4( it.ambient  );
				mtlConstBuffer.diffuse  = ConvertFloat4( it.diffuse  );
				mtlConstBuffer.specular = ConvertFloat4( it.specular );

				pImmediateContext->UpdateSubresource( iMaterialConstBuffer.Get(), 0, nullptr, &mtlConstBuffer, 0, 0 );

				pImmediateContext->VSSetConstantBuffers( 1, 1, iMaterialConstBuffer.GetAddressOf() );
				pImmediateContext->PSSetConstantBuffers( 1, 1, iMaterialConstBuffer.GetAddressOf() );
			}

			pImmediateContext->PSSetSamplers( 0, 1, it.diffuseMap.sampler.GetAddressOf() );
			pImmediateContext->PSSetShaderResources( 0, 1, it.diffuseMap.srv.GetAddressOf() );
			/* memo
			シェーダでは，レジスタのことをスロットとも言う。
			VS, PS SetConstantBuffers()の第１引数は，対応するレジスタ番号のものをセットする，というもの。
			0 なら b0, 1 なら b1, といった具合。
			*/

			pImmediateContext->DrawIndexed( it.indexCount, it.indexStart, 0 );
		}
	}
}