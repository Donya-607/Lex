#ifndef INCLUDED_DONYA_STATIC_MESH_H_
#define INCLUDED_DONYA_STATIC_MESH_H_

#include <array>
#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>
#include <wrl.h>

#include "Vector.h"	// Use at Face.

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
		/// If create failed, returns false.<para></para>
		/// If return valid instance, that is usable(The LoadObjFile() is unnecessary).
		/// </summary>
		static bool Create( const Loader &loader, StaticMesh &outputInstance );
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
			Donya::Vector4x4 coordinateConversion;
			Donya::Vector4x4 globalTransform;
			Microsoft::WRL::ComPtr<ID3D11Buffer> iIndexBuffer;
			Microsoft::WRL::ComPtr<ID3D11Buffer> iVertexBuffer;
			std::vector<Subset> subsets;
		public:
			Mesh() :
				coordinateConversion(), globalTransform(),
				iVertexBuffer(), iIndexBuffer(), subsets()
			{}
			Mesh( const Mesh & ) = default;
		};

		/// <summary>
		/// Use for collision.
		/// </summary>
		struct Face
		{
			int materialIndex{ -1 };				// -1 is invalid.
			std::array<Donya::Vector3, 3> points;	// Store local-space vertices of a triangle. CW.
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
		std::vector<Face>						collisionFaces;

		bool wasLoaded;
	public:
		StaticMesh();
		virtual ~StaticMesh();
	private:
		void CreateDefaultSettings( ID3D11Device *pDevice );
		void CreateRasterizerState( ID3D11Device *pDevice );
		void CreateDepthStencilState( ID3D11Device *pDevice );
		void LoadTextures( ID3D11Device *pDevice );

		/// <summary>
		/// Return false if the initialize failed, or already initialized.
		/// </summary>
		bool Init( const std::vector<std::vector<Vertex>> &verticesPerMesh, const std::vector<std::vector<size_t>> &indicesPerMesh, const std::vector<Mesh> &loadedMeshes, const std::vector<Face> &loadedFaces );
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
			ID3D11DeviceContext		*pImmediateContext = nullptr,
			bool useDefaultShading	= true,
			bool isEnableFill		= true,
			const Donya::Vector4x4	&defaultMatWVP		= {},
			const Donya::Vector4x4	&defaultMatW		= {},
			const Donya::Vector4	&defaultLightDir	= { 0.0f, 1.0f, 1.0f, 0.0f },
			const Donya::Vector4	&defaultMtlColor	= { 1.0f, 1.0f, 1.0f, 1.0f }
		) const;
	public:
		/// <summary>
		/// The members are valid when the "wasHit" is true.
		/// </summary>
		struct RayPickResult
		{
			int		materialIndex{};
			float	distanceToIP{};				// Store distance of nearest intersection-point.
			Donya::Vector3 intersectionPoint{};	// Store intersection-point of nearest face.
			Donya::Vector3 normal{};			// Store normalized normal.
			bool	wasHit{ false };			// If hit to any face, will be true.
		};
		/// <summary>
		/// Calculate the nearest intersection-point between the ray of arguments and faces of myself.<para></para>
		/// This method work in local-space of mesh, so you should transform to local-space the arguments.<para></para>
		/// If you set true to "enoughOnlyPickFirst", returns intersection-point that found at first. a little fast.
		/// </summary>
		RayPickResult RayPick( const Donya::Vector3 &rayStartPosition, const Donya::Vector3 &rayEndPosition, bool enoughOnlyPickFirst = false );
	};
}

#endif // !INCLUDED_DONYA_STATIC_MESH_H_
