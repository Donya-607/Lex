#pragma once

#include <array>
#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>
#include <wrl.h>

#include "Donya/CBuffer.h"

#include "Motion.h" // For update CB's bone matrix. I should refactoring this.

// Program version : 0

namespace Donya
{
	class Loader;

	class SkinnedMesh
	{
	public:
		/// <summary>
		/// Create from Loader object.<para></para>
		/// if create failed, or already loaded, returns false.<para></para>
		/// If you set nullptr to "pDevice", use default(library's) device.
		/// </summary>
		static bool Create( const Loader &loader, SkinnedMesh *pOutput, ID3D11Device *pDevice = nullptr );
	public:
		static constexpr const int MAX_BONE_COUNT		= 64;
		static constexpr const int MAX_BONE_INFLUENCES	= 4;
		struct Vertex
		{
			DirectX::XMFLOAT3	pos{};
			DirectX::XMFLOAT3	normal{};
			DirectX::XMFLOAT2	texCoord{};
			std::array<int,		MAX_BONE_INFLUENCES> boneIndices{};
			std::array<float,	MAX_BONE_INFLUENCES> boneWeights{ 1.0f, 0.0f, 0.0f, 0.0f };
		};

		/// <summary>
		/// This constants are use to constant-buffer internally. You must provide cbuffer of this in HLSL.
		/// </summary>
		struct CBufferPerMesh
		{
			DirectX::XMFLOAT4X4	worldViewProjection;
			DirectX::XMFLOAT4X4	world;
			std::array<DirectX::XMFLOAT4X4, MAX_BONE_COUNT> boneTransforms;
		};
		/// <summary>
		/// This constants are use to constant-buffer internally. You must provide cbuffer of this in HLSL.
		/// </summary>
		struct CBufferPerSubset
		{
			DirectX::XMFLOAT4	ambient;
			DirectX::XMFLOAT4	bump;
			DirectX::XMFLOAT4	diffuse;
			DirectX::XMFLOAT4	emissive;
			DirectX::XMFLOAT4	specular;
		};

		struct Material
		{
			Donya::Vector4 color;	// w channel is used as shininess by only specular.
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
			Material() :color( 1.0f, 1.0f, 1.0f, 1.0f ), iSampler(), textures()
			{}
		};

		struct Subset
		{
			size_t		indexStart;
			size_t		indexCount;
			float		transparency;
			Material	ambient;
			Material	bump;
			Material	diffuse;
			Material	emissive;
			Material	specular;
		public:
			Subset() : indexStart( NULL ), indexCount( NULL ), transparency( 0 ), ambient(), bump(), diffuse(), emissive(), specular()
			{}
		};

		struct Mesh
		{
			int										meshNo;
			Donya::Vector4x4						coordinateConversion;
			Donya::Vector4x4						globalTransform;
			Microsoft::WRL::ComPtr<ID3D11Buffer>	iIndexBuffer;
			Microsoft::WRL::ComPtr<ID3D11Buffer>	iVertexBuffer;
			std::vector<Subset>						subsets;
		public:
			Mesh() :
				meshNo(),
				coordinateConversion(), globalTransform(), // These are constructed in identity matrix.
				iVertexBuffer(), iIndexBuffer(),
				subsets()
			{}
			Mesh( const Mesh & ) = default;
		};
	private:
		bool										wasCreated;
		std::vector<Mesh>							meshes;

		mutable Donya::CBuffer<CBufferPerMesh>		cbPerMesh;
		mutable Donya::CBuffer<CBufferPerSubset>	cbPerSubset;

		template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
		mutable ComPtr<ID3D11RasterizerState>		iRasterizerStateWire;
		mutable ComPtr<ID3D11RasterizerState>		iRasterizerStateSurface;
		mutable ComPtr<ID3D11DepthStencilState>		iDepthStencilState;
	public:
		SkinnedMesh();
		~SkinnedMesh();
	public:
		/// <summary>
		/// If you set nullptr to "pDevice", use default(library's) device.<para></para>
		/// If create failed, or already created, returns false.
		/// </summary>
		bool Init
		(
			const std::vector<std::vector<size_t>>				&allMeshesIndex,
			const std::vector<std::vector<SkinnedMesh::Vertex>>	&allMeshesVertices,
			const std::vector<SkinnedMesh::Mesh>				&loadedMeshes,
			ID3D11Device *pDevice = nullptr
		);

		struct CBSetOption
		{
			unsigned int setSlot;
			bool setVS;
			bool setPS;
		};
		/// <summary>
		/// Please set a input-layout, vertex-shader and pixel-shader before call this.
		/// </summary>
		void Render
		(
			const Donya::MotionChunk	&motionPerMesh,
			const Donya::Animator		&currentAnimation,
			const Donya::Vector4x4		&matWorldViewProjection,
			const Donya::Vector4x4		&matWorld,
			const CBSetOption			&cbPerMeshOption,
			const CBSetOption			&cbPerSubsetOption,
			unsigned int				psSetSamplerSlot,
			unsigned int				psSetDiffuseMapSlot,
			bool isEnableFill = true
		) const;
	};
}
