#ifndef INCLUDED_DONYA_SHADER_H
#define INCLUDED_DONYA_SHADER_H

#include <d3d11.h>
#include <string>
#include <vector>
#include <wrl.h>

namespace Donya
{
	/// <summary>
	/// This class have vertex-shader object and input-layout object.<para></para>
	/// It can create(from cso-file), activate&lt;-&gt;de-activate.
	/// </summary>
	class VertexShader
	{
	private:
		bool wasCreated;	// If create failed, this will false.

		template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
		mutable ComPtr<ID3D11InputLayout>	iInputLayout;
		mutable ComPtr<ID3D11VertexShader>	iVertexShader;
		mutable ComPtr<ID3D11InputLayout>	iDefaultInputLayout;
		mutable ComPtr<ID3D11VertexShader>	iDefaultVertexShader;
	public:
		VertexShader();
		virtual ~VertexShader();

		VertexShader( const VertexShader & ) = delete;
		const VertexShader &operator = ( const VertexShader & ) = delete;
	public:
		/// <summary>
		/// Create vertex-shader and input-layout by compiled-shader-object file.<para></para>
		/// If the "pDevice" is null, use default(library's) device.<para></para>
		/// Returns true if succeeded to create or already created.
		/// </summary>
		bool CreateByCSO( const std::string &filePath, const std::vector<D3D11_INPUT_ELEMENT_DESC> &inputElementDescs, bool isEnableCache = true, ID3D11Device *pDevice = nullptr );
	public:
		/// <summary>
		/// If the "pImmediateContext" is null, use default(libray's) immediate-context.
		/// </summary>
		void Activate  ( ID3D11DeviceContext *pImmediateContext = nullptr ) const;
		/// <summary>
		/// If the "pImmediateContext" is null, use default(libray's) immediate-context.
		/// </summary>
		void Deactivate( ID3D11DeviceContext *pImmediateContext = nullptr ) const;
	};
	
	/// <summary>
	/// This class have pixel-shader object.<para></para>
	/// It can create(from cso-file), activate&lt;-&gt;de-activate.
	/// </summary>
	class PixelShader
	{
	private:
		bool wasCreated;	// If create failed, this will false.

		template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
		mutable ComPtr<ID3D11PixelShader>	iPixelShader;
		mutable ComPtr<ID3D11PixelShader>	iDefaultPixelShader;
	public:
		PixelShader();
		virtual ~PixelShader();

		PixelShader( const PixelShader & ) = delete;
		const PixelShader &operator = ( const PixelShader & ) = delete;
	public:
		/// <summary>
		/// Create pixel-shader by compiled-shader-object file.<para></para>
		/// If the "pDevice" is null, use default(library's) device.<para></para>
		/// Returns true if succeeded to create or already created.
		/// </summary>
		bool CreateByCSO( const std::string &filePath, bool isEnableCache = true, ID3D11Device *pDevice = nullptr );
	public:
		/// <summary>
		/// If the "pImmediateContext" is null, use default(libray's) immediate-context.
		/// </summary>
		void Activate  ( ID3D11DeviceContext *pImmediateContext = nullptr ) const;
		/// <summary>
		/// If the "pImmediateContext" is null, use default(libray's) immediate-context.
		/// </summary>
		void Deactivate( ID3D11DeviceContext *pImmediateContext = nullptr ) const;
	};
}

#endif // !INCLUDED_DONYA_SHADER_H
