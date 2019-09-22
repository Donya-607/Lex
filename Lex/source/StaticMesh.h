#ifndef INCLUDED_STATIC_MESH_H_
#define INCLUDED_STATIC_MESH_H_

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <wrl.h>

#include "Resource.h"

namespace Donya
{
	class Loader;

	class StaticMesh
	{
	public:
		/// <summary>
		/// Create from Loader object.<para></para>
		/// If create successed, return true.
		/// </summary>
		static bool Create( const Loader &loader, StaticMesh *pOutput );
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
	#define	COM_PTR Microsoft::WRL::ComPtr
		COM_PTR<ID3D11Buffer>				iVertexBuffer;
		COM_PTR<ID3D11Buffer>				iIndexBuffer;
		COM_PTR<ID3D11Buffer>				iConstantBuffer;
		COM_PTR<ID3D11Buffer>				iMaterialConstBuffer;
		COM_PTR<ID3D11InputLayout>			iInputLayout;
		COM_PTR<ID3D11VertexShader>			iVertexShader;
		COM_PTR<ID3D11PixelShader>			iPixelShader;
		COM_PTR<ID3D11RasterizerState>		iRasterizerStateWire;
		COM_PTR<ID3D11RasterizerState>		iRasterizerStateSurface;
		COM_PTR<ID3D11DepthStencilState>	iDepthStencilState;
	#undef	COM_PTR
		std::vector<Resource::Material>		materials;
		bool	isEnableTexture;
	public:
		StaticMesh( const std::wstring &objFileName, const std::string &vertexShaderCsoPath, const std::string &pixelShaderCsoPath );
		virtual ~StaticMesh();
	public:
		void Render
		(
			const DirectX::XMFLOAT4X4	&worldViewProjection,
			const DirectX::XMFLOAT4X4	&world,
			const DirectX::XMFLOAT4		&lightDirection,
			const DirectX::XMFLOAT4		&materialColor,
			const DirectX::XMFLOAT4		&cameraPos,
			bool isEnableFill = true
		);
	};

}

#endif // !INCLUDED_STATIC_MESH_H_
