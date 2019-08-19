#pragma once

#include <array>
#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>
#include <wrl.h>

// If your project isn't using Donya::Loader, you should set to false.
#define IS_SUPPORT_LEX_LOADER ( true )

namespace Donya
{
#if IS_SUPPORT_LEX_LOADER
	class Loader;
#endif // IS_SUPPORT_LEX_LOADER

	class SkinnedMesh
	{
	#if IS_SUPPORT_LEX_LOADER
	public:
		/// <summary>
		/// Create from Loader object.<para></para>
		/// if create successed, return true.
		/// </summary>
		static bool Create( const Loader *loader, std::unique_ptr<SkinnedMesh> *ppOutput );
	#endif // IS_SUPPORT_LEX_LOADER
	public:
		static constexpr const int MAX_BONE_INFLUENCES = 4;
		struct Vertex
		{
			DirectX::XMFLOAT3	pos{};
			DirectX::XMFLOAT3	normal{};
			DirectX::XMFLOAT2	texCoord{};
			std::array<int,		MAX_BONE_INFLUENCES> boneIndices{};
			std::array<float,	MAX_BONE_INFLUENCES> boneWeights{ 1.0f, 0.0f, 0.0f, 0.0f };
		};

		struct ConstantBuffer
		{
			DirectX::XMFLOAT4X4	worldViewProjection;
			DirectX::XMFLOAT4X4	world;
			// DirectX::XMFLOAT4	eyePosition;
			DirectX::XMFLOAT4	lightColor;
			DirectX::XMFLOAT4	lightDir;
		};

		struct MaterialConstantBuffer
		{
			DirectX::XMFLOAT4	ambient;
			DirectX::XMFLOAT4	bump;
			DirectX::XMFLOAT4	diffuse;
			DirectX::XMFLOAT4	emissive;
			DirectX::XMFLOAT4	specular;
		public:
			MaterialConstantBuffer() : ambient(), bump(), diffuse(), emissive(), specular()
			{}
		};

		struct Material
		{
			DirectX::XMFLOAT4 color;	// w channel is used as shininess by only specular.
			Microsoft::WRL::ComPtr<ID3D11SamplerState> iSampler;
			struct Texture
			{
				std::string fileName;	// absolute path.
				D3D11_TEXTURE2D_DESC texture2DDesc;
				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	iSRV;
			public:
				Texture() : fileName( "" ), texture2DDesc(), iSRV() {}
			};
			std::vector<Texture> textures;
		public:
			Material() :color( 0, 0, 0, 0 ), iSampler(), textures()
			{}
		};

		struct Subset
		{
			size_t indexStart;
			size_t indexCount;
			float  transparency;
			Material ambient;
			Material bump;
			Material diffuse;
			Material emissive;
			Material specular;
		public:
			Subset() : indexStart( NULL ), indexCount( NULL ), transparency( 0 ), ambient(), bump(), diffuse(), emissive(), specular()
			{}
		};

		struct Mesh
		{
			DirectX::XMFLOAT4X4 coordinateConversion;
			DirectX::XMFLOAT4X4 globalTransform;
			Microsoft::WRL::ComPtr<ID3D11Buffer> iIndexBuffer;
			Microsoft::WRL::ComPtr<ID3D11Buffer> iVertexBuffer;
			std::vector<Subset> subsets;
		public:
			Mesh() : coordinateConversion
			(
				{
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
				}
			),
			globalTransform
			(
				{
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
				}
			),
			iVertexBuffer(), iIndexBuffer(), subsets()
			{}
			Mesh( const Mesh & ) = default;
		};
	private:
		std::vector<Mesh> meshes;
	#define	COM_PTR Microsoft::WRL::ComPtr
		COM_PTR<ID3D11Buffer>				iConstantBuffer;
		COM_PTR<ID3D11Buffer>				iMaterialConstantBuffer;
		COM_PTR<ID3D11InputLayout>			iInputLayout;
		COM_PTR<ID3D11VertexShader>			iVertexShader;
		COM_PTR<ID3D11PixelShader>			iPixelShader;
		COM_PTR<ID3D11RasterizerState>		iRasterizerStateWire;
		COM_PTR<ID3D11RasterizerState>		iRasterizerStateSurface;
		COM_PTR<ID3D11DepthStencilState>	iDepthStencilState;
	#undef	COM_PTR
	public:
		SkinnedMesh();
		~SkinnedMesh();
	public:
		void Init( const std::vector<std::vector<size_t>> &allMeshesIndex, const std::vector<std::vector<SkinnedMesh::Vertex>> &allMeshesVertices, const std::vector<SkinnedMesh::Mesh> &loadedMeshes );
		void Render
		(
			const DirectX::XMFLOAT4X4	&worldViewProjection,
			const DirectX::XMFLOAT4X4	&world,
			const DirectX::XMFLOAT4		&eyePosition,
			const DirectX::XMFLOAT4		&lightColor,
			const DirectX::XMFLOAT4		&lightDirection,
			bool isEnableFill = true
		);
	};
}
