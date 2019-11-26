#ifndef INCLUDED_DONYA_GEOMETRIC_PRIMITIVE_H_
#define INCLUDED_DONYA_GEOMETRIC_PRIMITIVE_H_

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>

namespace Donya
{
	namespace Geometric
	{
		/// <summary>
		/// This class's has structs("Vertex", "ConstantBuffer") is default settings.
		/// </summary>
		class Base
		{
		public:
			struct Vertex
			{
				DirectX::XMFLOAT3 pos;
				DirectX::XMFLOAT3 normal;
			};
			struct ConstantBuffer
			{
				DirectX::XMFLOAT4X4	worldViewProjection;
				DirectX::XMFLOAT4X4	world;
				DirectX::XMFLOAT4	lightDirection;
				DirectX::XMFLOAT4	lightColor;
				DirectX::XMFLOAT4	materialColor;
			};
		protected:
			template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
			mutable ComPtr<ID3D11Buffer>			iVertexBuffer;
			mutable ComPtr<ID3D11Buffer>			iIndexBuffer;
			mutable ComPtr<ID3D11Buffer>			iConstantBuffer;
			mutable ComPtr<ID3D11InputLayout>		iInputLayout;
			mutable ComPtr<ID3D11VertexShader>		iVertexShader;
			mutable ComPtr<ID3D11PixelShader>		iPixelShader;
			mutable ComPtr<ID3D11RasterizerState>	iRasterizerStateWire;
			mutable ComPtr<ID3D11RasterizerState>	iRasterizerStateSurface;
			mutable ComPtr<ID3D11DepthStencilState>	iDepthStencilState;

			size_t indicesCount;
		public:
			Base();
			virtual ~Base() = default;
		public:
			virtual void Init() = 0;
			virtual void Uninit() = 0;

			/// <summary>
			/// If the "pImmediateContext" is null, use default(library's) context.<para></para>
			/// If "useDefaultShading" is true, I settings : ConstantBuffer, InputLayout, VertexShader, PixelShader.
			/// </summary>
			virtual void Render
			(
				ID3D11DeviceContext			*pImmediateContext = nullptr,
				bool useDefaultShading		= true,
				bool isEnableFill			= true,
				const DirectX::XMFLOAT4X4	&defaultMatWVP		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
				const DirectX::XMFLOAT4X4	&defaultMatW		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
				const DirectX::XMFLOAT4		&defaultLightDir	= { 0.0f, 1.0f, 1.0f, 0.0f },
				const DirectX::XMFLOAT4		&defaultMtlColor	= { 1.0f, 1.0f, 1.0f, 1.0f }
			) const = 0;
		};

		/// <summary>
		/// This class's has structs("Vertex", "ConstantBuffer") is default settings.
		/// </summary>
		class Cube : public Base
		{
		public:
			Cube();
			~Cube();
		public:
			void Init() override;
			void Uninit() override;

			/// <summary>
			/// If the "pImmediateContext" is null, use default(library's) context.<para></para>
			/// If "useDefaultShading" is true, I settings : ConstantBuffer, InputLayout, VertexShader, PixelShader.<para></para>
			/// In using a default shading, this render method do Donya::Color::FilteringAlpha() internally.
			/// </summary>
			virtual void Render
			(
				ID3D11DeviceContext			*pImmediateContext = nullptr,
				bool useDefaultShading		= true,
				bool isEnableFill			= true,
				const DirectX::XMFLOAT4X4	&defaultMatWVP		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
				const DirectX::XMFLOAT4X4	&defaultMatW		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
				const DirectX::XMFLOAT4		&defaultLightDir	= { 0.0f, 1.0f, 1.0f, 0.0f },
				const DirectX::XMFLOAT4		&defaultMtlColor	= { 1.0f, 1.0f, 1.0f, 1.0f }
			) const override;
		};

		/// <summary>
		/// This class's has structs("Vertex", "ConstantBuffer") is default settings.
		/// </summary>
		class Sphere : public Base
		{
		private:
			const size_t HORIZONTAL_SLICE{};
			const size_t VERTICAL_SLICE{};
		public:
			Sphere( size_t horizontalSliceCount = 12U, size_t verticalSliceCount = 6U );
			~Sphere();
		public:
			void Init() override;
			void Uninit() override;

			/// <summary>
			/// If the "pImmediateContext" is null, use default(library's) context.<para></para>
			/// If "useDefaultShading" is true, I settings : ConstantBuffer, InputLayout, VertexShader, PixelShader.<para></para>
			/// In using a default shading, this render method do Donya::Color::FilteringAlpha() internally.
			/// </summary>
			virtual void Render
			(
				ID3D11DeviceContext			*pImmediateContext = nullptr,
				bool useDefaultShading		= true,
				bool isEnableFill			= true,
				const DirectX::XMFLOAT4X4	&defaultMatWVP		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
				const DirectX::XMFLOAT4X4	&defaultMatW		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
				const DirectX::XMFLOAT4		&defaultLightDir	= { 0.0f, 1.0f, 1.0f, 0.0f },
				const DirectX::XMFLOAT4		&defaultMtlColor	= { 1.0f, 1.0f, 1.0f, 1.0f }
			) const override;
		};

		/// <summary>
		/// The board of duplex printed.<para></para>
		/// This class's has structs("Vertex", "ConstantBuffer") is default settings.
		/// </summary>
		class TextureBoard : public Base
		{
		public:
			// TODO:Support drawing part of texture.
			struct Vertex : public Base::Vertex
			{
				DirectX::XMFLOAT2 texCoord;
				// DirectX::XMFLOAT4 texCoordTransform;
			};
		private:
			const std::wstring FILE_PATH{};	// Contain file-directory + file-name.
			mutable D3D11_TEXTURE2D_DESC textureDesc;
			mutable Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	iSRV;
			mutable Microsoft::WRL::ComPtr<ID3D11SamplerState>			iSampler;
		public:
			TextureBoard( std::wstring filePath );
			~TextureBoard();
		public:
			void Init() override;
			void Uninit() override;

			/// <summary>
			/// If the "pImmediateContext" is null, use default(library's) context.<para></para>
			/// If "useDefaultShading" is true, I settings : ConstantBuffer, InputLayout, VertexShader, PixelShader.<para></para>
			/// In using a default shading, this render method do Donya::Color::FilteringAlpha() internally.
			/// </summary>
			virtual void Render
			(
				ID3D11DeviceContext			*pImmediateContext = nullptr,
				bool useDefaultShading		= true,
				bool isEnableFill			= true,
				const DirectX::XMFLOAT4X4	&defaultMatWVP		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
				const DirectX::XMFLOAT4X4	&defaultMatW		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
				const DirectX::XMFLOAT4		&defaultLightDir	= { 0.0f, 1.0f, 1.0f, 0.0f },
				const DirectX::XMFLOAT4		&defaultMtlColor	= { 1.0f, 1.0f, 1.0f, 1.0f }
			) const override;
		};
	}
}

#endif // !INCLUDED_DONYA_GEOMETRIC_PRIMITIVE_H_
