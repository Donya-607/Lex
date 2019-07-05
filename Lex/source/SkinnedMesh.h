#ifndef _INCLUDED_SKINNED_MESH_H_
#define _INCLUDED_SKINNED_MESH_H_

#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>
#include <wrl.h>

namespace Donya
{
	class Loader;

	class SkinnedMesh
	{
	public:
		/// <summary>
		/// Create from Loader object.<para></para>
		/// if create successed, return true.
		/// </summary>
		static bool Create( const Loader *loader, std::unique_ptr<SkinnedMesh> *ppOutput );
	public:
		struct Vertex
		{
			DirectX::XMFLOAT3	pos;
			DirectX::XMFLOAT3	normal;
			DirectX::XMFLOAT2	texCoord;
		};
		struct ConstantBuffer
		{
			DirectX::XMFLOAT4X4	worldViewProjection;
			DirectX::XMFLOAT4X4	world;
			DirectX::XMFLOAT4	eyePosition;
			DirectX::XMFLOAT4	lightDirection;
			DirectX::XMFLOAT4	materialColor;
		};
		struct MaterialConstantBuffer
		{
			DirectX::XMFLOAT4	ambient;
			DirectX::XMFLOAT4	bump;
			DirectX::XMFLOAT4	diffuse;
			DirectX::XMFLOAT4	emissive;
			DirectX::XMFLOAT4	specular;
			float				shininess;
			DirectX::XMFLOAT3	padding;
		public:
			MaterialConstantBuffer() : ambient(), bump(), diffuse(), emissive(), specular(), shininess()
			{}
		};
		struct Material
		{
			DirectX::XMFLOAT3	ambient;
			DirectX::XMFLOAT3	bump;
			DirectX::XMFLOAT3	diffuse;
			DirectX::XMFLOAT3	emissive;
			DirectX::XMFLOAT3	specular;
			float				transparency;
			float				shininess;
			struct Texture
			{
				std::string fileName;	// absolute path.
				D3D11_TEXTURE2D_DESC texture2DDesc;
				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	iSRV;
				Microsoft::WRL::ComPtr<ID3D11SamplerState>			iSampler;
			public:
				Texture() : fileName( "" ), texture2DDesc(), iSRV(), iSampler() {}
			};
			std::vector<Texture> textures;
		public:
			Material() : ambient(), bump(), diffuse(), emissive(), specular(), transparency(), shininess(), textures() {}
		};
		struct Subset
		{

		};
	private:
		size_t vertexCount;
	#define	COM_PTR Microsoft::WRL::ComPtr
		COM_PTR<ID3D11Buffer>				iVertexBuffer;
		COM_PTR<ID3D11Buffer>				iIndexBuffer;
		COM_PTR<ID3D11Buffer>				iConstantBuffer;
		COM_PTR<ID3D11Buffer>				iMaterialConstantBuffer;
		COM_PTR<ID3D11InputLayout>			iInputLayout;
		COM_PTR<ID3D11VertexShader>			iVertexShader;
		COM_PTR<ID3D11PixelShader>			iPixelShader;
		COM_PTR<ID3D11RasterizerState>		iRasterizerStateWire;
		COM_PTR<ID3D11RasterizerState>		iRasterizerStateSurface;
		COM_PTR<ID3D11DepthStencilState>	iDepthStencilState;
	#undef	COM_PTR
		std::vector<Material> materials;
	public:
		SkinnedMesh( const std::vector<size_t> &indices, const std::vector<Vertex> &vertices, const std::vector<Material> &loadedMaterials );
		~SkinnedMesh();
	public:
		void Render
		(
			const DirectX::XMFLOAT4X4	&worldViewProjection,
			const DirectX::XMFLOAT4X4	&world,
			const DirectX::XMFLOAT4		&eyePosition,
			const DirectX::XMFLOAT4		&lightDirection,
			const DirectX::XMFLOAT4		&materialColor,
			bool isEnableFill = true
		);
	};
}

#endif // _INCLUDED_SKINNED_MESH_H_