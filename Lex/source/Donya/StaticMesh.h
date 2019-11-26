#ifndef INCLUDED_STATIC_MESH_H_
#define INCLUDED_STATIC_MESH_H_

#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>
#include <wrl.h>

namespace Donya
{
	class Loader;

	/// <summary>
	/// If you want load the obj-file, you specify obj-file-path then you call LoadObjFile().
	/// </summary>
	class StaticMesh
	{
	public:
		/// <summary>
		/// Create from Loader object.<para></para>
		/// If create failed, return nullptr.<para></para>
		/// If return valid instance, that is usable(The LoadObjFile() is unnecessary).
		/// </summary>
		static std::shared_ptr<StaticMesh> Create( const Loader &loader );
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
			DirectX::XMFLOAT4	lightDirection;
			DirectX::XMFLOAT4	lightColor;
			DirectX::XMFLOAT4	materialColor;
		};
		struct MaterialConstBuffer
		{
			DirectX::XMFLOAT4	ambient;
			DirectX::XMFLOAT4	diffuse;
			DirectX::XMFLOAT4	specular;
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
			Material() : color( 0, 0, 0, 1 ), iSampler(), textures()
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
			Mesh() :
				coordinateConversion
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
		template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
		mutable ComPtr<ID3D11Buffer>			iDefaultCBuffer;
		mutable ComPtr<ID3D11Buffer>			iDefaultMaterialCBuffer;
		mutable ComPtr<ID3D11InputLayout>		iDefaultInputLayout;
		mutable ComPtr<ID3D11VertexShader>		iDefaultVS;
		mutable ComPtr<ID3D11PixelShader>		iDefaultPS;
		mutable ComPtr<ID3D11RasterizerState>	iRasterizerStateSurface;
		mutable ComPtr<ID3D11RasterizerState>	iRasterizerStateWire;
		mutable ComPtr<ID3D11DepthStencilState>	iDepthStencilState;
	
		std::vector<Mesh>						meshes;
		bool wasLoaded;
	public:
		StaticMesh();
		virtual ~StaticMesh();
	private:
		void CreateDefaultSettings( ID3D11Device *pDevice );
		void CreateRasterizerState( ID3D11Device *pDevice );
		void CreateDepthStencilState( ID3D11Device *pDevice );
		void LoadTextures( ID3D11Device *pDevice );

		void Init( const std::vector<std::vector<Vertex>> &verticesPerMesh, const std::vector<std::vector<size_t>> &indicesPerMesh, const std::vector<Mesh> &loadedMeshes );
	public:
		/// <summary>
		/// If failed load, or already loaded, returns false.<para></para>
		/// Also doing initialize.
		/// </summary>
		bool LoadObjFile( const std::wstring &objFilePath );

		/// <summary>
		/// In using a default shading, this render method do Donya::Color::FilteringAlpha() internally.
		/// </summary>
		void Render
		(
			ID3D11DeviceContext			*pImmediateContext = nullptr,
			bool useDefaultShading		= true,
			bool isEnableFill			= true,
			const DirectX::XMFLOAT4X4	&defaultMatWVP		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
			const DirectX::XMFLOAT4X4	&defaultMatW		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
			const DirectX::XMFLOAT4		&defaultLightDir	= { 0.0f, 1.0f, 1.0f, 0.0f },
			const DirectX::XMFLOAT4		&defaultMtlColor	= { 1.0f, 1.0f, 1.0f, 1.0f }
		) const;
	};

}

#endif // !INCLUDED_STATIC_MESH_H_
