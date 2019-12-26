#ifndef INCLUDED_DONYA_GEOMETRIC_PRIMITIVE_H_
#define INCLUDED_DONYA_GEOMETRIC_PRIMITIVE_H_

#include <array>
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>

#include "Color.h"		// Use for Line.
#include "Shader.h"		// Use for Line.
#include "Quaternion.h"	// Use for Billboard's freeze axis.
#include "Vector.h"

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

			size_t	indicesCount;
			bool	wasCreated;
		public:
			Base();
			virtual ~Base() = default;
		public:
			virtual void Init() = 0;
			virtual void Uninit() = 0;
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
			) const;
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

			Sphere( const Sphere & );
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
			) const;
		};

		/// <summary>
		/// The board of duplex printed.<para></para>
		/// This class's has structs("Vertex", "ConstantBuffer") is default settings.
		/// </summary>
		class TextureBoard : public Base
		{
		private:
			// Front-face and back-face.
			static constexpr int VERTEX_COUNT = 4 * 2;
		public:
			struct Vertex : public Base::Vertex
			{
				DirectX::XMFLOAT2 texCoord;
				DirectX::XMFLOAT4 texCoordTransform; // X, Y is left-top. Z, W is size(Z:width, W:height).
			};
		private:
			const std::wstring FILE_PATH{};	// Contain file-directory + file-name.

			mutable std::array<Vertex, VERTEX_COUNT>					vertices;
			mutable D3D11_TEXTURE2D_DESC								textureDesc;
			mutable Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	iSRV;
			mutable Microsoft::WRL::ComPtr<ID3D11SamplerState>			iSampler;
		public:
			TextureBoard( std::wstring filePath );
			~TextureBoard();

			TextureBoard( const TextureBoard & );
		public:
			void Init() override;
			void Uninit() override;

			// Note : I want provide a method of render like billboard, but I don't have affine matrix, so I provide calculate method of rotation matrix like billboard.
			/// <summary>
			/// Returns the matrix whose rotation follows "lookDirection". That direction will be normalize.<para></para>
			/// The "freezeDirection" is specify a fixed direction(ex. if set to "Up", it up direction will not rotation).
			/// </summary>
			Donya::Vector4x4 CalcBillboardRotation( const Donya::Vector3 &lookDirection, float zRadian, Donya::Quaternion::Freeze freezeDirection = Donya::Quaternion::Freeze::Front ) const;

			/// <summary>
			/// If the "pImmediateContext" is null, use default(library's) context.<para></para>
			/// If "useDefaultShading" is true, I settings : ConstantBuffer, InputLayout, VertexShader, PixelShader.<para></para>
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
			/// <summary>
			/// "texPartPosLT" is specify the part position of left-top.<para></para>
			/// "texPartWholeSize" is specify the part whole size(width, height).<para></para>
			/// If the "pImmediateContext" is null, use default(library's) context.<para></para>
			/// If "useDefaultShading" is true, I settings : ConstantBuffer, InputLayout, VertexShader, PixelShader.<para></para>
			/// In using a default shading, this render method do Donya::Color::FilteringAlpha() internally.
			/// </summary>
			void RenderPart
			(
				const Donya::Vector2		&texPartPosLT,
				const Donya::Vector2		&texPartWholeSize,
				ID3D11DeviceContext			*pImmediateContext = nullptr,
				bool useDefaultShading		= true,
				bool isEnableFill			= true,
				const DirectX::XMFLOAT4X4	&defaultMatWVP		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
				const DirectX::XMFLOAT4X4	&defaultMatW		= { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
				const DirectX::XMFLOAT4		&defaultLightDir	= { 0.0f, 1.0f, 1.0f, 0.0f },
				const DirectX::XMFLOAT4		&defaultMtlColor	= { 1.0f, 1.0f, 1.0f, 1.0f }
			) const;
		};

		class Line
		{
		public:
			struct Vertex
			{
				// The "matVP" is not need here,
				// but I don't want use a constant-buffer,
				// because the shader is made from embeded source-code,
				// so the slot of constant-buffer can not change by external.
				// So I decide to place here.

				DirectX::XMFLOAT3	pos{};				// Default is place to one(1,1,1).
				DirectX::XMFLOAT4X4	matVP{};			// The View-Projection matrix.
			};
			struct Instance
			{
				float				scaling{ 1.0f };	// The length of line.
				DirectX::XMFLOAT3	translation{};
				DirectX::XMFLOAT4X4	rotation{};			// The rotation from front(0,0,1) vector to direction(end-start).
				DirectX::XMFLOAT4	color{ 0.0f, 0.0f, 0.0f, 1.0f };	// RGBA.
			};
		private:
			int  idDepthStencil;
			int  idRasterizer;
			bool wasCreated;

			Donya::VertexShader lineVS;
			Donya::PixelShader  linePS;

			const   size_t MAX_INSTANCES;
			mutable size_t reserveCount;
			mutable std::vector<Instance>		instances;
			template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
			mutable ComPtr<ID3D11Buffer>		pVertexBuffer;
			mutable ComPtr<ID3D11Buffer>		pInstanceBuffer;
		public:
			Line( size_t maxInstanceCount = 32U );
			~Line();

			Line( const Line & );
		public:
			/// <summary>
			/// A current reserving instances will be discard.<para></para>
			/// Returns false if faileded an initialize.
			/// </summary>
			bool Init();
			void Uninit();

			/// <summary>
			/// This method only reserve to a internal bundle-list.<para></para>
			/// Returns false if a current reserving count over than internal capacity.
			/// </summary>
			bool Reserve( const Donya::Vector3 &wsStartPoint, const Donya::Vector3 &wsEndPoint, const Donya::Vector4 &color = { 0.0f, 0.0f, 0.0f, 1.0f } ) const;
			/// <summary>
			/// This method only reserve to a internal bundle-list.<para></para>
			/// Returns false if a current reserving count over than internal capacity.
			/// </summary>
			bool Reserve( const Donya::Vector3 &wsStartPoint, const Donya::Vector3 &wsEndPoint, float alpha, Donya::Color::Code color = Donya::Color::Code::BLACK ) const;

			/// <summary>
			/// Draw current reserving instances.
			/// </summary>
			void Flush( const Donya::Vector4x4 &matViewProjection ) const;
		};

		Cube			CreateCube();
		Sphere			CreateSphere( size_t horizontalSliceCount = 12U, size_t verticalSliceCount = 6U );
		TextureBoard	CreateTextureBoard( std::wstring textureFilePath );
	}
}

#endif // !INCLUDED_DONYA_GEOMETRIC_PRIMITIVE_H_
