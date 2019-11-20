#pragma once

#include <D3D11.h>
#include <DirectXMath.h>	// Use for XMFLOAT2, 3, 4.
#include <memory>
#include <string>
#include <vector>
#include <wrl.h>

#include "Vector.h" // Use Donya::Int2

namespace Donya
{
	namespace Sprite
	{
		/// <summary>
		/// Use bitwise operation. ex) X_** | Y_**<para></para>
		/// default is *_MIDDLE.
		/// </summary>
		enum Origin
		{
			X_LEFT		= 1 << 0,
			X_MIDDLE	= 1 << 1,
			X_RIGHT		= 1 << 2,
			Y_TOP		= 1 << 3,
			Y_MIDDLE	= 1 << 4,
			Y_BOTTOM	= 1 << 5,

			CENTER		= X_MIDDLE	| Y_MIDDLE,	// X_MIDDLE | Y_MIDDLE.
			LEFT_TOP	= X_LEFT	| Y_TOP,	// X_LEFT | Y_TOP.
			FOOT		= X_MIDDLE	| Y_BOTTOM	// X_MIDDLE | Y_BOTTOM.
		};
		static Origin operator | ( const Origin &lhs, const Origin &rhs )
		{
			return static_cast<Origin>( static_cast<int>( lhs ) | static_cast<int>( rhs ) );
		}
		static Origin operator & ( const Origin &lhs, const Origin &rhs )
		{
			return static_cast<Origin>( static_cast<int>( lhs ) & static_cast<int>( rhs ) );
		}

		enum class Color : int
		{
			AQUA		= 0x00FFFF,	// #00FFFF
			BLACK		= 0x000000,	// #000000
			BLUE		= 0x0000FF,	// #0000FF
			CYAN		= AQUA,		// #00FFFF
			DARK_GRAY	= 0x575757,	// #575757
			FUCHSIA		= 0xCC1669,	// #CC1669
			GRAY		= 0x808080,	// #808080
			GREEN		= 0x008000,	// #008000
			LIME		= 0x00FF00,	// #00FF00
			LIGHT_GRAY	= 0xD3D3D3,	// #D3D3D3
			MAGENTA		= 0xFF00FF,	// #FF00FF
			MAROON		= 0x800000,	// #800000
			NAVY		= 0x1F2F54,	// #1F2F54
			OLIVE		= 0x808000,	// #808000
			ORANGE		= 0xF39800,	// #F39800
			PURPLE		= 0xA758A8,	// #A758A8
			RED			= 0xFF0000,	// #FF0000
			SILVER		= 0xC0C0C0,	// #C0C0C0
			TEAL		= 0x006956,	// #006956
			WHITE		= 0xFFFFFF,	// #FFFFFF
			YELLOW		= 0xFFFF00,	// #FFFF00
		};

		static constexpr DirectX::XMFLOAT3 MakeColor( const int &color )
		{
			return DirectX::XMFLOAT3
			{
				static_cast<float>( ( color >> 16 ) & 0xff ) / 255.0f,
				static_cast<float>( ( color >> 8  ) & 0xff ) / 255.0f,
				static_cast<float>( ( color >> 0  ) & 0xff ) / 255.0f,
			};
		}
		static constexpr DirectX::XMFLOAT3 MakeColor( const Color &color )
		{
			return MakeColor( static_cast<int>( color ) );
		}

		/// <summary>
		/// Set drawing depth(default is 1.0f). this value is permanence.<para></para>
		/// Depth value is : [Near 0.0f - 1.0f Far].<para></para>
		/// I will draw if less than the depth of an existing sprite.<para></para>
		/// Depth will be clamp if out of range.
		/// </summary>
		void  SetDrawDepth( float depth );
		float GetDrawDepth();

		/// <summary>
		/// Not doing batching processs.<para></para>
		/// deprecated.
		/// </summary>
		class Single
		{
		public:
			struct Vertex
			{
				DirectX::XMFLOAT3 pos;
				DirectX::XMFLOAT4 color;
				DirectX::XMFLOAT2 texCoord;
			};
		public:
			D3D11_TEXTURE2D_DESC								d3dTexture2DDesc;
			Microsoft::WRL::ComPtr<ID3D11VertexShader>			d3dVertexShader;
			Microsoft::WRL::ComPtr<ID3D11PixelShader>			d3dPixelShader;
			Microsoft::WRL::ComPtr<ID3D11InputLayout>			d3dInputLayout;
			Microsoft::WRL::ComPtr<ID3D11Buffer>				d3dVertexBuffer;
			Microsoft::WRL::ComPtr<ID3D11RasterizerState>		d3dRasterizerState;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	d3dShaderResourceView;
			Microsoft::WRL::ComPtr<ID3D11SamplerState>			d3dSamplerState;
			Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		d3dDepthStencilState;
		public:
			Single( const std::wstring spriteFilename );
			~Single();
		public:
			/// <summary>
			/// Returns whole-size.<para></para>
			/// You can ignore to setting nullptr.
			/// </summary>
			void GetTextureSize( int *width, int *height );
			/// <summary>
			/// Returns whole-size.<para></para>
			/// You can ignore to setting nullptr.
			/// </summary>
			void GetTextureSize( float *width, float *height );
		public:
			/// <summary>
			/// Rotation center is origin.
			/// </summary>
			void RenderExt
			(
				float screenX,		float screenY,		// Coordinate of sprite's center in screen space.
				float screenW,		float screenH,		// Whole Size of sprite in screen space.
				float textureX,		float textureY,		// Coordinate of sprite's center in texture space.
				float textureW,		float textureH,		// Whole Size of sprite in texture space.
				float degree,							// Rotation angle.
				float rotCenterX,	float rotCenterY,	// Rotation center in sprite space.
				float R = 1.0f,							// Color of sprite's each vertices.
				float G = 1.0f,
				float B = 1.0f,
				float A = 1.0f
			);
			/// <summary>
			/// Rotation center is origin.<para></para>
			/// Rotation center is sprite center.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.
			/// </summary>
			void Render
			(
				float screenX,	float screenY,	// Coordinate of sprite's center in screen space.
				float screenW,	float screenH,	// Whole Size of sprite in screen space.
				float degree = 0.0f,			// Rotation angle.
				float R = 1.0f,					// Color of sprite's each vertices
				float G = 1.0f,
				float B = 1.0f,
				float A = 1.0f
			);
			/// <summary>
			/// Rotation center is origin.<para></para>
			/// Drawing size is sprite size.<para></para>
			/// Rotation center is sprite center.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			void Render
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float degree = 0.0f				// Rotation angle.
			);
		};

		/// <summary>
		/// How to use:<para></para>
		/// Use ReserveXX() method, you can register drawing sprite.<para></para>
		/// Then Call Render() method, this is actually drawing sprites that reserved.
		/// </summary>
		class Batch
		{
		public:
			struct Vertex
			{
				DirectX::XMFLOAT3 pos;
				DirectX::XMFLOAT2 texCoord;
			};
		private:
			const size_t MAX_INSTANCES;
			size_t reserveCount;

			struct Instance
			{
				DirectX::XMFLOAT4	color;
				DirectX::XMFLOAT4X4	NDCTransform;
				DirectX::XMFLOAT4	texCoordTransform;
			};

			// std::unique_ptr<Instance[]> pInstances; // see https://qiita.com/bluepost59/items/b7490ee0cb19857b8cd0
			// Instance *pInstances;
			std::vector<Instance> instances;

			D3D11_TEXTURE2D_DESC								d3dTexture2DDesc;

			Microsoft::WRL::ComPtr<ID3D11Buffer>				d3dInstanceBuffer;

			Microsoft::WRL::ComPtr<ID3D11Buffer>				d3dVertexBuffer;

			Microsoft::WRL::ComPtr<ID3D11VertexShader>			d3dVertexShader;
			Microsoft::WRL::ComPtr<ID3D11PixelShader>			d3dPixelShader;
			Microsoft::WRL::ComPtr<ID3D11InputLayout>			d3dInputLayout;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	d3dShaderResourceView;

			Microsoft::WRL::ComPtr<ID3D11RasterizerState>		d3dRasterizerState;
			Microsoft::WRL::ComPtr<ID3D11SamplerState>			d3dSamplerState;
			Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		d3dDepthStencilState;
		public:
			Batch( const std::wstring spriteFilename, size_t maxInstancesCount = 32U );
			~Batch();
			Batch( const Batch & ) = delete;
			Batch &operator = ( const Batch & ) = delete;
		public:
			/// <summary>
			/// Returns whole-size.
			/// </summary>
			int GetTextureWidth() const;
			/// <summary>
			/// Returns height-size.
			/// </summary>
			int GetTextureHeight() const;
			/// <summary>
			/// Returns whole-size.
			/// </summary>
			float GetTextureWidthF() const;
			/// <summary>
			/// Returns height-size.
			/// </summary>
			float GetTextureHeightF() const;
			/// <summary>
			/// Returns whole-size.<para></para>
			/// You can ignore to setting nullptr.
			/// </summary>
			void GetTextureSize( int *width, int *height ) const;
			/// <summary>
			/// Returns whole-size.<para></para>
			/// You can ignore to setting nullptr.
			/// </summary>
			void GetTextureSize( float *width, float *height ) const;
		public:
			/// <summary>
			/// Calculate center pos of sprite space by "center" bit, from whole size of sprite.
			/// </summary>
			DirectX::XMFLOAT2 MakeSpriteCenter( Donya::Sprite::Origin center, float scaleX, float scaleY ) const;
			/// <summary>
			/// Calculate center pos of sprite space by specified size.
			/// </summary>
			DirectX::XMFLOAT2 MakeSpecifiedCenter( Donya::Sprite::Origin center, float textureW, float textureH, float scaleX, float scaleY ) const;
		public:

		#pragma region Normal
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is sprite size.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool Reserve
			(
				float  screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float  degree,					// Rotation angle, Unit is degree.
				DirectX::XMFLOAT2 center,
				float  alpha = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is sprite size.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool Reserve
			(
				float  screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float  degree,					// Rotation angle, Unit is degree.
				Origin center,
				float  alpha = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is sprite size.<para></para>
			/// Rotation center is sprite center.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool Reserve
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
				float alpha  = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is sprite size.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReserveExt
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float scaleX,  float scaleY,	// Magnification.
				float degree,					// Rotation angle, Unit is degree.
				DirectX::XMFLOAT2 center,
				float alpha = 1.0f,
				float R = 1.0f,					// Blend-color of sprite's each vertices
				float G = 1.0f,
				float B = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is sprite size.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReserveExt
			(
				float  screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float  scaleX,  float scaleY,	// Magnification.
				float  degree,					// Rotation angle, Unit is degree.
				Origin center,
				float  alpha  = 1.0f,
				float  R = 1.0f,				// Blend-color of sprite's each vertices
				float  G = 1.0f,
				float  B = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is sprite size.<para></para>
			/// Rotation center is sprite center.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReserveExt
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float scaleX,  float scaleY,	// Magnification.
				float degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
				float alpha  = 1.0f,
				float R = 1.0f,					// Blend-color of sprite's each vertices
				float G = 1.0f,
				float B = 1.0f
			);
		#pragma endregion

		#pragma region Stretched
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReserveStretched
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float screenW, float screenH,	// Whole Size of sprite in screen space.
				float degree,					// Rotation angle, Unit is degree.
				DirectX::XMFLOAT2 center,
				float alpha = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReserveStretched
			(
				float  screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float  screenW, float screenH,	// Whole Size of sprite in screen space.
				float  degree,					// Rotation angle, Unit is degree.
				Origin center,
				float  alpha = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Rotation center is sprite center.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReserveStretched
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float screenW, float screenH,	// Whole Size of sprite in screen space.
				float degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
				float alpha  = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReserveStretchedExt
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float screenW, float screenH,	// Whole Size of sprite in screen space.
				float scaleX,  float scaleY,	// Magnification.
				float degree,					// Rotation angle, Unit is degree.
				DirectX::XMFLOAT2 center,
				float alpha = 1.0f,
				float R = 1.0f,					// Blend-color of sprite's each vertices
				float G = 1.0f,
				float B = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReserveStretchedExt
			(
				float  screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float  screenW, float screenH,	// Whole Size of sprite in screen space.
				float  scaleX, float scaleY,	// Magnification.
				float  degree,					// Rotation angle, Unit is degree.
				Origin center,
				float  alpha = 1.0f,
				float  R = 1.0f,				// Blend-color of sprite's each vertices
				float  G = 1.0f,
				float  B = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Rotation center is sprite center.<para></para>
			/// Texture origin is left-top(0, 0), using whole size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReserveStretchedExt
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float screenW, float screenH,	// Whole Size of sprite in screen space.
				float scaleX,  float scaleY,	// Magnification.
				float degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
				float alpha  = 1.0f,
				float R = 1.0f,					// Blend-color of sprite's each vertices
				float G = 1.0f,
				float B = 1.0f
			);
		#pragma endregion

		#pragma region Part
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is specified texture size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReservePart
			(
				float screenX,  float screenY,	// Coordinate of sprite's center in screen space.
				float textureX, float textureY,	// Coordinate of sprite's center in texture space.
				float textureW, float textureH,	// Whole Size of sprite in texture space.
				float degree,					// Rotation angle, Unit is degree.
				DirectX::XMFLOAT2 center,
				float alpha = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is specified texture size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReservePart
			(
				float  screenX,  float screenY,	// Coordinate of sprite's center in screen space.
				float  textureX, float textureY,// Coordinate of sprite's center in texture space.
				float  textureW, float textureH,// Whole Size of sprite in texture space.
				float  degree,					// Rotation angle, Unit is degree.
				Origin center,
				float  alpha  = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is specified texture size.<para></para>
			/// Rotation center is sprite center.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReservePart
			(
				float screenX,  float screenY,	// Coordinate of sprite's center in screen space.
				float textureX, float textureY,	// Coordinate of sprite's center in texture space.
				float textureW, float textureH,	// Whole Size of sprite in texture space.
				float degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
				float alpha = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is specified texture size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReservePartExt
			(
				float screenX,  float screenY,	// Coordinate of sprite's center in screen space.
				float textureX, float textureY,	// Coordinate of sprite's center in texture space.
				float textureW, float textureH,	// Whole Size of sprite in texture space.
				float scaleX,   float scaleY,	// Magnification.
				float degree,					// Rotation angle, Unit is degree.
				DirectX::XMFLOAT2 center,
				float alpha = 1.0f,
				float R = 1.0f,					// Blend-color of sprite's each vertices
				float G = 1.0f,
				float B = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is specified texture size.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReservePartExt
			(
				float  screenX,  float screenY,	// Coordinate of sprite's center in screen space.
				float  textureX, float textureY,// Coordinate of sprite's center in texture space.
				float  textureW, float textureH,// Whole Size of sprite in texture space.
				float  scaleX,   float scaleY,	// Magnification.
				float  degree,					// Rotation angle, Unit is degree.
				Origin center,
				float  alpha  = 1.0f,
				float  R = 1.0f,				// Blend-color of sprite's each vertices
				float  G = 1.0f,
				float  B = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Drawing size is specified texture size.<para></para>
			/// Rotation center is sprite center.<para></para>
			/// Colors are 1.0f.
			/// </summary>
			bool ReservePartExt
			(
				float screenX,  float screenY,	// Coordinate of sprite's center in screen space.
				float textureX, float textureY,	// Coordinate of sprite's center in texture space.
				float textureW, float textureH,	// Whole Size of sprite in texture space.
				float scaleX,   float scaleY,	// Magnification.
				float degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
				float alpha  = 1.0f,
				float R = 1.0f,					// Blend-color of sprite's each vertices
				float G = 1.0f,
				float B = 1.0f
			);
		#pragma endregion

		#pragma region Tiled

		#pragma endregion

		#pragma region General
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// </summary>
			bool ReserveGeneral
			(
				float screenX,  float screenY,	// Coordinate of sprite's center in screen space.
				float screenW,  float screenH,	// Whole Size of sprite in screen space.
				float textureX, float textureY,	// Coordinate of sprite's center in texture space.
				float textureW, float textureH,	// Whole Size of sprite in texture space.
				float degree,					// Rotation angle, Unit is degree.
				DirectX::XMFLOAT2 center,
				float alpha = 1.0f,
				float R = 1.0f,
				float G = 1.0f,
				float B = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// </summary>
			bool ReserveGeneral
			(
				float  screenX,  float screenY,	// Coordinate of sprite's center in screen space.
				float  screenW,  float screenH,	// Whole Size of sprite in screen space.
				float  textureX, float textureY,// Coordinate of sprite's center in texture space.
				float  textureW, float textureH,// Whole Size of sprite in texture space.
				float  degree = 0.0f,			// Rotation angle, Unit is degree.
				Origin center = X_MIDDLE | Y_MIDDLE,
				float  alpha  = 1.0f,
				float  R = 1.0f,
				float  G = 1.0f,
				float  B = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// </summary>
			bool ReserveGeneralExt
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float screenW, float screenH,	// Whole Size of sprite in screen space.
				float textureX, float textureY,	// Coordinate of sprite's center in texture space.
				float textureW, float textureH,	// Whole Size of sprite in texture space.
				float scaleX, float scaleY,		// Magnification.
				float degree,					// Rotation angle, Unit is degree.
				DirectX::XMFLOAT2 center,
				float alpha = 1.0f,
				float R = 1.0f,
				float G = 1.0f,
				float B = 1.0f
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// </summary>
			bool ReserveGeneralExt
			(
				float  screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float  screenW, float screenH,	// Whole Size of sprite in screen space.
				float  textureX, float textureY,// Coordinate of sprite's center in texture space.
				float  textureW, float textureH,// Whole Size of sprite in texture space.
				float  scaleX, float scaleY,	// Magnification.
				float  degree = 0.0f,			// Rotation angle, Unit is degree.
				Origin center = X_MIDDLE | Y_MIDDLE,
				float  alpha = 1.0f,
				float  R = 1.0f,
				float  G = 1.0f,
				float  B = 1.0f
			);
		#pragma endregion

			void Render();
		};

		/// <summary>
		/// No-texture version.<para></para>
		/// This class supporting batching-process.
		/// </summary>
		class Rect
		{
		public:
			struct Vertex
			{
				DirectX::XMFLOAT3 pos;
			};
		private:
			const size_t MAX_INSTANCES;
			size_t reserveCount;

			struct Instance
			{
				DirectX::XMFLOAT4	color;
				DirectX::XMFLOAT4X4	NDCTransform;
			};

			// std::unique_ptr<Instance[]> pInstances; // see https://qiita.com/bluepost59/items/b7490ee0cb19857b8cd0
			// Instance *pInstances;
			std::vector<Instance> instances;

			Microsoft::WRL::ComPtr<ID3D11Buffer>				d3dInstanceBuffer;
			Microsoft::WRL::ComPtr<ID3D11Buffer>				d3dVertexBuffer;
			Microsoft::WRL::ComPtr<ID3D11VertexShader>			d3dVertexShader;
			Microsoft::WRL::ComPtr<ID3D11PixelShader>			d3dPixelShader;
			Microsoft::WRL::ComPtr<ID3D11InputLayout>			d3dInputLayout;
			Microsoft::WRL::ComPtr<ID3D11RasterizerState>		d3dRasterizerState;
			Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		d3dDepthStencilState;
		public:
			Rect( size_t maxInstances = 128U );
			~Rect();
			Rect( const Rect & ) = delete;
			Rect &operator = ( const Rect & ) = delete;
		public:
			DirectX::XMFLOAT2 MakeCenter( Origin center, float width, float height ) const;
		public:
		#pragma region Reserves
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// </summary>
			bool Reserve
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float screenW, float screenH,	// Whole Size of sprite in screen space.
				float R, float G, float B,		// Color of each vertices.
				float alpha,					// Alpha of each vertices.
				float degree,					// Rotation angle, Unit is degree.
				DirectX::XMFLOAT2 center
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// </summary>
			bool Reserve
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float screenW, float screenH,	// Whole Size of sprite in screen space.
				Color color,					// Color of each vertices.
				float alpha,					// Alpha of each vertices.
				float degree,					// Rotation angle, Unit is degree.
				DirectX::XMFLOAT2 center
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// </summary>
			bool Reserve
			(
				float  screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float  screenW, float screenH,	// Whole Size of sprite in screen space.
				float  R, float G, float B,		// Color of each vertices.
				float  alpha,					// Alpha of each vertices.
				float  degree,					// Rotation angle, Unit is degree.
				Origin center
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// </summary>
			bool Reserve
			(
				float  screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float  screenW, float screenH,	// Whole Size of sprite in screen space.
				Color  color,					// Color of each vertices.
				float  alpha,					// Alpha of each vertices.
				float  degree,					// Rotation angle, Unit is degree.
				Origin center
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Rotation center is sprite center.<para></para>
			/// </summary>
			bool Reserve
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float screenW, float screenH,	// Whole Size of sprite in screen space.
				float R, float G, float B,		// Color of each vertices.
				float alpha,					// Alpha of each vertices.
				float degree = 0.0f				// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// Rotation center is sprite center.<para></para>
			/// </summary>
			bool Reserve
			(
				float screenX, float screenY,	// Coordinate of sprite's center in screen space.
				float screenW, float screenH,	// Whole Size of sprite in screen space.
				Color color,					// Color of each vertices.
				float alpha,					// Alpha of each vertices.
				float degree = 0.0f				// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
			);
		#pragma endregion

			void Render();
		};

		/// <summary>
		/// No-texture version.<para></para>
		/// This class supporting batching-process.
		/// </summary>
		class Circle
		{
		public:
			struct Vertex
			{
				DirectX::XMFLOAT3 pos;
			};
		private:
			const size_t MAX_INSTANCES;
			size_t reserveCount;
			size_t currentDetail;	// Store specified vertex count.

			struct Instance
			{
				DirectX::XMFLOAT4	color;
				DirectX::XMFLOAT4X4	NDCTransform;
			};

			std::vector<Instance> instances;

			Microsoft::WRL::ComPtr<ID3D11Buffer>				d3dInstanceBuffer;
			Microsoft::WRL::ComPtr<ID3D11Buffer>				d3dIndexBuffer;
			Microsoft::WRL::ComPtr<ID3D11Buffer>				d3dVertexBuffer;
			Microsoft::WRL::ComPtr<ID3D11VertexShader>			d3dVertexShader;
			Microsoft::WRL::ComPtr<ID3D11PixelShader>			d3dPixelShader;
			Microsoft::WRL::ComPtr<ID3D11InputLayout>			d3dInputLayout;
			Microsoft::WRL::ComPtr<ID3D11RasterizerState>		d3dRasterizerState;
			Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		d3dDepthStencilState;
		public:
			Circle( size_t vertexCount = 8U, size_t maxInstances = 128U );
			~Circle();
			Circle( const Circle & ) = delete;
			Circle &operator = ( const Circle & ) = delete;
		public:
			DirectX::XMFLOAT2 MakeCenter( Origin center, float diameter ) const;
		public:
		#pragma region Reserves
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// </summary>
			bool Reserve
			(
				float  screenX, float screenY,	// Coordinate of circle's center in screen space.
				float  screenDiameter,			// Diameter is size of circle in screen space.
				float  R, float G, float B,		// Color of each vertices.
				float  alpha,					// Alpha of each vertices.
				Origin center = Origin::CENTER
			);
			/// <summary>
			/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
			/// </summary>
			bool Reserve
			(
				float  screenX, float screenY,	// Coordinate of circle's center in screen space.
				float  screenDiameter,			// Diameter is size of circle in screen space.
				Color  color,					// Color of each vertices.
				float  alpha,					// Alpha of each vertices.
				Origin center = Origin::CENTER
			);
		private:
			bool Reserve
			(
				float  screenX, float screenY,	// Coordinate of circle's center in screen space.
				float  screenDiameter,			// Diameter is size of circle in screen space.
				float  R, float G, float B,		// Color of each vertices.
				float  alpha,					// Alpha of each vertices.
				DirectX::XMFLOAT2 center
			);
		public:
		#pragma endregion

			void Render();
		};

		/// <summary>
		/// The "maxInstanceCountOfPrimitive" is specify the count of primitives(drawing by DrawRect(), DrawCircle()).<para></para>
		/// The "vertexCountOfCirclePerQuadrant" is used at circle of primitive(drawing by DrawCircle()).
		/// </summary>
		void Init( unsigned int maxInstanceCountOfPrimitive = 128U, unsigned int vertexCountOfCirclePerQuadrant = 8U );

		void Uninit();

		/// <summary>
		/// Returns created sprite's identifier,<para></para>
		/// but if failed to create, returns NULL.(NULL is invalid identifier)<para></para>
		/// "maxInstancesCount" is upper-limit of batching of sprite,<para></para>
		/// and relate in memory usage.
		/// </summary>
		size_t Load( const std::wstring &spriteFileName, size_t maxInstancesCount = 32 );

	#pragma region GetTextureSizes

		/// <summary>
		/// Returns whole-size.<para></para>
		/// If "spriteIdentifier" was invalid, returns NULL.
		/// </summary>
		int GetTexturWidth( size_t spriteIdentifier );
		/// <summary>
		/// Returns whole-size.<para></para>
		/// If "spriteIdentifier" was invalid, returns NULL.
		/// </summary>
		int GetTextureHeight( size_t spriteIdentifier );
		/// <summary>
		/// Returns whole-size.<para></para>
		/// If "spriteIdentifier" was invalid, returns NULL.
		/// </summary>
		float GetTexturWidthF( size_t spriteIdentifier );
		/// <summary>
		/// Returns whole-size.<para></para>
		/// If "spriteIdentifier" was invalid, returns NULL.
		/// </summary>
		float GetTextureHeightF( size_t spriteIdentifier );
		/// <summary>
		/// Returns whole-size.<para></para>
		/// You can ignore by setting nullptr.<para></para>
		/// If "spriteIdentifier" was invalid, returns false.
		/// </summary>
		bool GetTextureSize( size_t spriteIdentifier, int *width, int *height );
		/// <summary>
		/// Returns whole-size.<para></para>
		/// You can ignore by setting nullptr.<para></para>
		/// If "spriteIdentifier" was invalid, returns false.
		/// </summary>
		bool GetTextureSize( size_t spriteIdentifier, float *width, float *height );

	#pragma endregion

	#pragma region Normal
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is sprite size.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool Draw
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  degree,					// Rotation angle, Unit is degree.
			DirectX::XMFLOAT2 center,
			float  alpha = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is sprite size.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool Draw
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  degree,					// Rotation angle, Unit is degree.
			Origin center,
			float  alpha = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is sprite size.<para></para>
		/// Rotation center is sprite center.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool Draw
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
			float  alpha = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is sprite size.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  scaleX,  float scaleY,	// Magnification.
			float  degree,					// Rotation angle, Unit is degree.
			DirectX::XMFLOAT2 center,
			float  alpha = 1.0f,
			float  R = 1.0f,				// Blend-color of sprite's each vertices
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is sprite size.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  scaleX, float scaleY,	// Magnification.
			float  degree,					// Rotation angle, Unit is degree.
			Origin center,
			float  alpha = 1.0f,
			float  R = 1.0f,				// Blend-color of sprite's each vertices
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is sprite size.<para></para>
		/// Rotation center is sprite center.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  scaleX, float scaleY,	// Magnification.
			float  degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
			float  alpha = 1.0f,
			float  R = 1.0f,					// Blend-color of sprite's each vertices
			float  G = 1.0f,
			float  B = 1.0f
		);
	#pragma endregion

	#pragma region Stretched
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawStretched
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  screenWholeWidth,
			float  screenWholeHeight,
			float  degree,					// Rotation angle, Unit is degree.
			DirectX::XMFLOAT2 center,
			float  alpha = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawStretched
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  screenWholeWidth,
			float  screenWholeHeight,
			float  degree,					// Rotation angle, Unit is degree.
			Origin center,
			float  alpha = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Rotation center is sprite center.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawStretched
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  screenWholeWidth,
			float  screenWholeHeight,
			float  degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
			float  alpha = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawStretchedExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  screenWholeWidth,
			float  screenWholeHeight,
			float  scaleX, float scaleY,	// Magnification.
			float  degree,					// Rotation angle, Unit is degree.
			DirectX::XMFLOAT2 center,
			float  alpha = 1.0f,
			float  R = 1.0f,				// Blend-color of sprite's each vertices
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawStretchedExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  screenWholeWidth,
			float  screenWholeHeight,
			float  scaleX, float scaleY,	// Magnification.
			float  degree,					// Rotation angle, Unit is degree.
			Origin center,
			float  alpha = 1.0f,
			float  R = 1.0f,				// Blend-color of sprite's each vertices
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Rotation center is sprite center.<para></para>
		/// Texture origin is left-top(0, 0), using whole size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawStretchedExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  screenWholeWidth,
			float  screenWholeHeight,
			float  scaleX, float scaleY,	// Magnification.
			float  degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
			float  alpha = 1.0f,
			float  R = 1.0f,				// Blend-color of sprite's each vertices
			float  G = 1.0f,
			float  B = 1.0f
		);
	#pragma endregion

	#pragma region Part
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is specified texture size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawPart
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  textureLeftTopPosX,
			float  textureLeftTopPosY,
			float  textureWholeWidth,
			float  textureWholeHeight,
			float  degree,					// Rotation angle, Unit is degree.
			DirectX::XMFLOAT2 center,
			float  alpha = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is specified texture size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawPart
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  textureLeftTopPosX,
			float  textureLeftTopPosY,
			float  textureWholeWidth,
			float  textureWholeHeight,
			float  degree,					// Rotation angle, Unit is degree.
			Origin center,
			float  alpha = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is specified texture size.<para></para>
		/// Rotation center is sprite center.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawPart
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  textureLeftTopPosX,
			float  textureLeftTopPosY,
			float  textureWholeWidth,
			float  textureWholeHeight,
			float  degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
			float  alpha = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is specified texture size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawPartExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  textureLeftTopPosX,
			float  textureLeftTopPosY,
			float  textureWholeWidth,
			float  textureWholeHeight,
			float  scaleX,   float scaleY,	// Magnification.
			float  degree,					// Rotation angle, Unit is degree.
			DirectX::XMFLOAT2 center,
			float  alpha = 1.0f,
			float  R = 1.0f,				// Blend-color of sprite's each vertices
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is specified texture size.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawPartExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  textureLeftTopPosX,
			float  textureLeftTopPosY,
			float  textureWholeWidth,
			float  textureWholeHeight,
			float  scaleX,   float scaleY,	// Magnification.
			float  degree,					// Rotation angle, Unit is degree.
			Origin center,
			float  alpha = 1.0f,
			float  R = 1.0f,				// Blend-color of sprite's each vertices
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// Drawing size is specified texture size.<para></para>
		/// Rotation center is sprite center.<para></para>
		/// Colors are 1.0f.<para></para>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawPartExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX,
			float  screenCenterPosY,
			float  textureLeftTopPosX,
			float  textureLeftTopPosY,
			float  textureWholeWidth,
			float  textureWholeHeight,
			float  scaleX,   float scaleY,	// Magnification.
			float  degree = 0.0f,			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
			float  alpha = 1.0f,
			float  R = 1.0f,					// Blend-color of sprite's each vertices
			float  G = 1.0f,
			float  B = 1.0f
		);
	#pragma endregion

	#pragma region Tiled

	#pragma endregion

	#pragma region General
		/// <summary>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawGeneral
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX, float screenCenterPosY,
			float  screenWholeWidth, float screenWholeHeight,
			float  textureLeftTopPosX, float textureLeftTopPosY,
			float  textureWholeWidth,  float textureWholeHeight,
			float  degree,					// Rotation angle, Unit is degree.
			DirectX::XMFLOAT2 center,
			float  alpha = 1.0f,
			float  R = 1.0f,
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawGeneral
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX, float screenCenterPosY,
			float  screenWholeWidth, float screenWholeHeight,
			float  textureLeftTopPosX, float textureLeftTopPosY,
			float  textureWholeWidth,  float textureWholeHeight,
			float  degree = 0.0f,			// Rotation angle, Unit is degree.
			Origin center = X_MIDDLE | Y_MIDDLE,
			float  alpha = 1.0f,
			float  R = 1.0f,
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawGeneralExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX, float screenCenterPosY,
			float  screenWholeWidth, float screenWholeHeight,
			float  textureLeftTopPosX, float textureLeftTopPosY,
			float  textureWholeWidth,  float textureWholeHeight,
			float  scaleX, float scaleY,	// Magnification.
			float  degree,					// Rotation angle, Unit is degree.
			DirectX::XMFLOAT2 center,
			float  alpha = 1.0f,
			float  R = 1.0f,
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawGeneralExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			float  screenCenterPosX, float screenCenterPosY,
			float  screenWholeWidth, float screenWholeHeight,
			float  textureLeftTopPosX, float textureLeftTopPosY,
			float  textureWholeWidth,  float textureWholeHeight,
			float  scaleX, float scaleY,	// Magnification.
			float  degree = 0.0f,			// Rotation angle, Unit is degree.
			Origin center = X_MIDDLE | Y_MIDDLE,
			float  alpha  = 1.0f,
			float  R  = 1.0f,
			float  G  = 1.0f,
			float  B  = 1.0f
		);
	#pragma endregion

	#pragma region Text
		/// <summary>
		/// Calculate place(0-based) of specified character in texture.<para></para>
		/// Returns place(x,y) is count of division(ex:'A'=(1,4)).<para></para>
		/// This function follow the ASCII-code table.
		/// </summary>
		Donya::Int2 CalcTextCharPlace( char character );
		/// <summary>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawString
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			std::string drawingString,
			float  screenCenterPosX, float screenCenterPosY,
			float  screenWholeWidth, float screenWholeHeight,
			float  textureWholeWidth, float textureWholeHeight,
			float  degree,					// Rotation angle.
			DirectX::XMFLOAT2 center,
			float  alpha = 1.0f,
			float  R = 1.0f,
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawString
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			std::string drawingString,
			float  screenCenterPosX, float screenCenterPosY,
			float  screenWholeWidth, float screenWholeHeight,
			float  textureWholeWidth, float textureWholeHeight,
			float  degree = 0.0f,			// Rotation angle.
			Origin center = X_MIDDLE | Y_MIDDLE,
			float  alpha = 1.0f,
			float  R = 1.0f,
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawStringExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			std::string drawingString,
			float  screenCenterPosX, float screenCenterPosY,
			float  screenWholeWidth, float screenWholeHeight,
			float  textureWholeWidth, float textureWholeHeight,
			float  scaleX, float scaleY,	// Magnification.
			float  degree,					// Rotation angle.
			DirectX::XMFLOAT2 center,
			float  alpha = 1.0f,
			float  R = 1.0f,
			float  G = 1.0f,
			float  B = 1.0f
		);
		/// <summary>
		/// In case of we can not drawing, returns false.<para></para>
		/// may be considered why it can not drawn, the following reasons:<para></para>
		/// * the "spriteIdentifier" was invalid identifier.<para></para>
		/// * drawn count ware over than "maxInstancesCount" of when created.
		/// </summary>
		bool DrawStringExt
		(
			size_t spriteIdentifier,		// NULL is invalid identifier.
			std::string drawingString,
			float  screenCenterPosX, float screenCenterPosY,
			float  screenWholeWidth, float screenWholeHeight,
			float  textureWholeWidth, float textureWholeHeight,
			float  scaleX, float scaleY,	// Magnification.
			float  degree = 0.0f,			// Rotation angle.
			Origin center = X_MIDDLE | Y_MIDDLE,
			float  alpha = 1.0f,
			float  R = 1.0f,
			float  G = 1.0f,
			float  B = 1.0f
		);
	#pragma endregion

	#pragma region Rect
		bool DrawRect
		(
			float screenCenterPosX, float screenCenterPosY,
			float screenWholeWidth, float screenWholeHeight,
			float R, float G, float B,		// Color of each vertices.
			float alpha,					// Alpha of each vertices.
			float degree,					// Rotation angle, Unit is degree.
			DirectX::XMFLOAT2 center
		);
		bool DrawRect
		(
			float screenCenterPosX, float screenCenterPosY,
			float screenWholeWidth, float screenWholeHeight,
			Color color,					// Color of each vertices.
			float alpha,					// Alpha of each vertices.
			float degree,					// Rotation angle, Unit is degree.
			DirectX::XMFLOAT2 center
		);
		bool DrawRect
		(
			float  screenCenterPosX, float screenCenterPosY,
			float  screenWholeWidth, float screenWholeHeight,
			float  R, float G, float B,		// Color of each vertices.
			float  alpha,					// Alpha of each vertices.
			float  degree,					// Rotation angle, Unit is degree.
			Origin center
		);
		bool DrawRect
		(
			float  screenCenterPosX, float screenCenterPosY,
			float  screenWholeWidth, float screenWholeHeight,
			Color  color,					// Color of each vertices.
			float  alpha,					// Alpha of each vertices.
			float  degree,					// Rotation angle, Unit is degree.
			Origin center
		);
		/// <summary>
		/// Rotation center is sprite center.<para></para>
		/// </summary>
		bool DrawRect
		(
			float screenCenterPosX, float screenCenterPosY,
			float screenWholeWidth, float screenWholeHeight,
			float R, float G, float B,		// Color of each vertices.
			float alpha,					// Alpha of each vertices.
			float degree = 0.0f				// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
		);
		/// <summary>
		/// Rotation center is sprite center.<para></para>
		/// </summary>
		bool DrawRect
		(
			float screenCenterPosX, float screenCenterPosY,
			float screenWholeWidth, float screenWholeHeight,
			Color color,				// Color of each vertices.
			float alpha,				// Alpha of each vertices.
			float degree = 0.0f			// Rotation angle ( Rotation center is sprite's center ), Unit is degree.
		);
	#pragma endregion

	#pragma region Circle
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// </summary>
		bool DrawCircle
		(
			float screenCenterPosX, float screenCenterPosY,
			float  screenDiameter,			// Diameter is size of circle in screen space.
			float  R, float G, float B,		// Color of each vertices.
			float  alpha,					// Alpha of each vertices.
			Origin center = Origin::CENTER
		);
		/// <summary>
		/// If current reserving instances over than MAX_INSTACES, returns false, and not reserving.<para></para>
		/// </summary>
		bool DrawCircle
		(
			float screenCenterPosX, float screenCenterPosY,
			float  screenDiameter,			// Diameter is size of circle in screen space.
			Color  color,					// Color of each vertices.
			float  alpha,					// Alpha of each vertices.
			Origin center = Origin::CENTER
		);
	#pragma endregion

		/// <summary>
		/// Drawing current batching sprites.
		/// </summary>
		void Flush();

		/// <summary>
		/// Please call before Present().
		/// </summary>
		void PostDraw();
	}
}
