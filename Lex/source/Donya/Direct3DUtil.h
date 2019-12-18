#pragma once

#include <d3d11.h>
#include <vector>

namespace Donya
{
	/// <summary>
	/// Settings detail:<para></para>
	/// BUFFER_DESC::ByteWidth = sizeof( Vertex ) * vertices.size()<para></para>
	/// BUFFER_DESC::Usage = usage<para></para>
	/// BUFFER_DESC::BindFlags = D3D11_BIND_VERTEX_BUFFER<para></para>
	/// BUFFER_DESC::CPUAccessFlags = CPUAccessFlags<para></para>
	/// BUFFER_DESC::MiscFlags = 0;<para></para>
	/// BUFFER_DESC::StructureByteStride = 0;<para></para>
	/// </summary>
	template<typename Vertex>
	static HRESULT CreateVertexBuffer( ID3D11Device *pDevice, const std::vector<Vertex> &vertices, D3D11_USAGE usage, UINT CPUAccessFlags, ID3D11Buffer **bufferAddress )
	{
		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.ByteWidth			= sizeof( Vertex ) * vertices.size();
		bufferDesc.Usage				= usage;
		bufferDesc.BindFlags			= D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags		= CPUAccessFlags;
		bufferDesc.MiscFlags			= 0;
		bufferDesc.StructureByteStride	= 0;

		D3D11_SUBRESOURCE_DATA subResource{};
		subResource.pSysMem				= vertices.data();
		subResource.SysMemPitch			= 0; // Not use for vertex buffers.
		subResource.SysMemSlicePitch	= 0; // Not use for vertex buffers.

		return pDevice->CreateBuffer
		(
			&bufferDesc,
			&subResource,
			bufferAddress
		);
	}
	/// <summary>
	/// Settings detail:<para></para>
	/// BUFFER_DESC::ByteWidth = sizeof( Vertex ) * vertices.size()<para></para>
	/// BUFFER_DESC::Usage = usage<para></para>
	/// BUFFER_DESC::BindFlags = D3D11_BIND_VERTEX_BUFFER<para></para>
	/// BUFFER_DESC::CPUAccessFlags = CPUAccessFlags<para></para>
	/// BUFFER_DESC::MiscFlags = 0;<para></para>
	/// BUFFER_DESC::StructureByteStride = 0;<para></para>
	/// </summary>
	template<typename Vertex, size_t ArraySize>
	static HRESULT CreateVertexBuffer( ID3D11Device *pDevice, const std::array<Vertex, ArraySize> &vertices, D3D11_USAGE usage, UINT CPUAccessFlags, ID3D11Buffer **bufferAddress )
	{
		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.ByteWidth			= sizeof( Vertex ) * vertices.size();
		bufferDesc.Usage				= usage;
		bufferDesc.BindFlags			= D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags		= CPUAccessFlags;
		bufferDesc.MiscFlags			= 0;
		bufferDesc.StructureByteStride	= 0;

		D3D11_SUBRESOURCE_DATA subResource{};
		subResource.pSysMem				= vertices.data();
		subResource.SysMemPitch			= 0; // Not use for vertex buffers.
		subResource.SysMemSlicePitch	= 0; // Not use for vertex buffers.

		return pDevice->CreateBuffer
		(
			&bufferDesc,
			&subResource,
			bufferAddress
		);
	}

	/// <summary>
	/// Settings detail:<para></para>
	/// BUFFER_DESC::ByteWidth = sizeof( unsigned int ) * indices.size()<para></para>
	/// BUFFER_DESC::Usage = D3D11_USAGE_IMMUTABLE<para></para>
	/// BUFFER_DESC::BindFlags = D3D11_BIND_INDEX_BUFFER<para></para>
	/// BUFFER_DESC::CPUAccessFlags = 0<para></para>
	/// BUFFER_DESC::MiscFlags = 0;<para></para>
	/// BUFFER_DESC::StructureByteStride = 0;<para></para>
	/// </summary>
	static HRESULT CreateIndexBuffer( ID3D11Device *pDevice, const std::vector<unsigned int> &indices, ID3D11Buffer **bufferAddress )
	{
		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.ByteWidth			= sizeof( unsigned int ) * indices.size();
		bufferDesc.Usage				= D3D11_USAGE_IMMUTABLE;
		bufferDesc.BindFlags			= D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags		= 0;
		bufferDesc.MiscFlags			= 0;
		bufferDesc.StructureByteStride	= 0;

		D3D11_SUBRESOURCE_DATA subResource{};
		subResource.pSysMem				= indices.data();
		subResource.SysMemPitch			= 0;
		subResource.SysMemSlicePitch	= 0;

		return pDevice->CreateBuffer
		(
			&bufferDesc,
			&subResource,
			bufferAddress
		);
	}

	/// <summary>
	/// Settings detail:<para></para>
	/// BUFFER_DESC::ByteWidth = sizeOfConstantBuffer<para></para>
	/// BUFFER_DESC::Usage = D3D11_USAGE_DEFAULT<para></para>
	/// BUFFER_DESC::BindFlags = D3D11_BIND_CONSTANT_BUFFER<para></para>
	/// BUFFER_DESC::CPUAccessFlags = 0<para></para>
	/// BUFFER_DESC::MiscFlags = 0;<para></para>
	/// BUFFER_DESC::StructureByteStride = 0;<para></para>
	/// </summary>
	static HRESULT CreateConstantBuffer( ID3D11Device *pDevice, size_t sizeOfConstantBuffer, ID3D11Buffer **bufferAddress )
	{
		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.ByteWidth			= sizeOfConstantBuffer;
		bufferDesc.Usage				= D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags			= D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags		= 0;
		bufferDesc.MiscFlags			= 0;
		bufferDesc.StructureByteStride	= 0;

		return pDevice->CreateBuffer
		(
			&bufferDesc,
			nullptr,
			bufferAddress
		);
	}

}
