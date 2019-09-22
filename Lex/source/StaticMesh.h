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
		/// You can specify vertex-shader and pixel-shader with cso-file with set to the second-argument and third-argument(If set to nullptr, default shader is used).<para></para>
		/// If create failed, return nullptr.<para></para>
		/// If return valid instance, that is usable(The LoadObjFile() is unnecessary).
		/// </summary>
		static std::shared_ptr<StaticMesh> Create( const Loader &loader, const std::string *pVSCsoFilePath = nullptr, const std::string *pPSCsoFilePath = nullptr );
	public:
		struct Vertex
		{
			DirectX::XMFLOAT3 pos;
			DirectX::XMFLOAT3 normal;
			DirectX::XMFLOAT2 texCoord;
		};
		struct ConstantBuffer
		{
			DirectX::XMFLOAT4X4	worldViewProjection;
			DirectX::XMFLOAT4X4	world;
			DirectX::XMFLOAT4	lightDirection;
			DirectX::XMFLOAT4	materialColor;
			DirectX::XMFLOAT4	cameraPos;
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
			Material() : color( 0, 0, 0, 0 ), iSampler(), textures()
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
	private:
		const std::string  CSO_PATH_PS{};
		const std::string  CSO_PATH_VS{};
		const std::wstring OBJ_FILE_PATH{};

	#define	COM_PTR Microsoft::WRL::ComPtr
		mutable COM_PTR<ID3D11Buffer>				iVertexBuffer;
		mutable COM_PTR<ID3D11Buffer>				iIndexBuffer;
		mutable COM_PTR<ID3D11Buffer>				iConstantBuffer;
		mutable COM_PTR<ID3D11Buffer>				iMaterialConstBuffer;
		mutable COM_PTR<ID3D11InputLayout>			iInputLayout;
		mutable COM_PTR<ID3D11VertexShader>			iVertexShader;
		mutable COM_PTR<ID3D11PixelShader>			iPixelShader;
		mutable COM_PTR<ID3D11RasterizerState>		iRasterizerStateSurface;
		mutable COM_PTR<ID3D11RasterizerState>		iRasterizerStateWire;
		mutable COM_PTR<ID3D11DepthStencilState>	iDepthStencilState;
	#undef	COM_PTR
		std::vector<Subset>							subsets;
		bool wasLoaded;
	public:
		StaticMesh( const std::wstring &objFileName, const std::string &vertexShaderCsoPath, const std::string &pixelShaderCsoPath );
		virtual ~StaticMesh();
	private:
		void Init( const std::vector<Vertex> &connectedVertices, const std::vector<size_t> &connectedIndices, const std::vector<Subset> &subsets );
	public:
		/// <summary>
		/// If failed load, or already loaded, returns false.<para></para>
		/// Also doing initialize.
		/// </summary>
		bool LoadObjFile();

		void Render
		(
			const DirectX::XMFLOAT4X4	&worldViewProjection,
			const DirectX::XMFLOAT4X4	&world,
			const DirectX::XMFLOAT4		&lightDirection,
			const DirectX::XMFLOAT4		&materialColor,
			const DirectX::XMFLOAT4		&cameraPos,
			bool isEnableFill = true
		) const;
	};

}

#endif // !INCLUDED_STATIC_MESH_H_
