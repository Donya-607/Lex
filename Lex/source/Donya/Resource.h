#pragma once

#include <string>
#include <vector>
#include <D3D11.h>
#include <DirectXMath.h>
#include <wrl.h>

namespace Donya
{
	namespace Resource
	{
		#pragma region Shader

		/// <summary>
		/// If file path is invalid, returns false.<para></para>
		/// I doing; read cso-file, CreateVertexShader().<para></para>
		/// If unnecessary ID3D11InputLayout, you can set nullptr.
		/// </summary>
		bool CreateVertexShaderFromCso
		(
			ID3D11Device					*pd3dDevice,
			const char						*csoname,
			const char						*openMode,
			ID3D11VertexShader				**pOutVertexShaderAddress,
			ID3D11InputLayout				**pOutInputLayoutAddress,
			const D3D11_INPUT_ELEMENT_DESC	*d3dInputElementsDescs,
			size_t							inputElementSize,
			bool isEnableCache = true
		);
		/// <summary>
		/// Create vertex-shader from embedded source code.<para></para>
		/// "shaderIdentifyName" are used to key of map.<para></para>
		/// "shaderEntryPointName" is shader files entry point(e.g. "main").<para></para>
		/// Returns false when failed to create shader.<para></para>
		/// If unnecessary ID3D11InputLayout, you can set nullptr.
		/// </summary>
		bool CreateVertexShaderFromSource
		(
			ID3D11Device					*pDevice,
			const std::string				&shaderIdentifyName,
			const std::string				&shaderCode,
			const std::string				&shaderEntryPointName,
			ID3D11VertexShader				**pOutVertexShaderAddress,
			ID3D11InputLayout				**pOutInputLayoutAddress,
			const D3D11_INPUT_ELEMENT_DESC	*pInputElementsDesc,
			size_t							inputElementsCount,
			bool isEnableCache = true
		);

		void ReleaseAllVertexShaderCaches();

		/// <summary>
		/// If file path is invalid, returns false.<para></para>
		/// I doing; read cso-file, CreatePixelShader().
		/// </summary>
		bool CreatePixelShaderFromCso
		(
			ID3D11Device					*pd3dDevice,
			const char						*csoname,
			const char						*openMode,
			ID3D11PixelShader				**pOutPixelShaderAddress,
			bool enableCache = true
		);
		/// <summary>
		/// Create pixel-shader from embedded source code.<para></para>
		/// "shaderIdentifyName" are used to key of map.<para></para>
		/// "shaderEntryPointName" is shader files entry point(e.g. "main").<para></para>
		/// Returns false when failed to create shader.
		/// </summary>
		bool CreatePixelShaderFromSource
		(
			ID3D11Device					*pDevice,
			const std::string				&shaderIdentifyName,
			const std::string				&shaderCode,
			const std::string				&shaderEntryPointName,
			ID3D11PixelShader				**pOutPixelShaderAddress,
			bool isEnableCache = true
		);

		void ReleaseAllPixelShaderCaches();

		#pragma endregion

		#pragma region Texture

		/// <summary>
		/// If file path is invalid, returns false.<para></para>
		/// I doing; CreateWICTextureFromFile(), QueryInterface(),<para></para>
		/// ID3D11Texture2D::GetDesc().<para></para>
		/// These arguments must be not null.
		/// </summary>
		bool CreateTexture2DFromFile
		(
			ID3D11Device				*pd3dDevice,
			const std::wstring			&filename,
			ID3D11ShaderResourceView	**pd3dShaderResourceView,
			D3D11_TEXTURE2D_DESC		*pd3dTexture2DDesc,
			bool isEnableCache = true
		);

		/// <summary>
		/// The color range are 0.0f ~ 1.0f.
		/// </summary>
		void CreateUnicolorTexture
		(
			ID3D11Device *pDevice,
			ID3D11ShaderResourceView **pOutputShaderResourceView,
			D3D11_TEXTURE2D_DESC *pOutputTexture2DDesc,
			unsigned int dimensions = 1,
			float R = 1.0f,
			float G = 1.0f,
			float B = 1.0f,
			float A = 1.0f,
			bool isEnableCache = true
		);

		void ReleaseAllTexture2DCaches();

		#pragma endregion

		#pragma region Sampler

		void CreateSamplerState
		(
			ID3D11Device *pDevice,
			Microsoft::WRL::ComPtr<ID3D11SamplerState> *pOutputSamplerState,
			const D3D11_SAMPLER_DESC &samplerDesc,
			bool isEnableCache = true
		);

		/// <summary>
		/// Returns invalid ID3D11SamplerState.GetAddressOf.
		/// </summary>
		Microsoft::WRL::ComPtr<ID3D11SamplerState> &RequireInvalidSamplerStateComPtr();

		void ReleaseAllSamplerStateCaches();

		#pragma endregion

		#pragma region OBJ

		/// <summary>
		/// It is one of material in mtl-file.
		/// </summary>
		struct Material
		{
			// TODO:I wanna separate subset parameter from material.

			size_t	indexCount = 0;		// zero-based number.
			size_t	indexStart = 0;		// zero-based number.

			int		illuminate = 0;		// 0 ~ 10
			float	shininess = 0;		// 0.0f ~ 1000.0f
			float	ambient[3]{};		// RGB, 0.0f ~ 1.0f
			float	diffuse[3]{};		// RGB, 0.0f ~ 1.0f
			float	specular[3]{};		// RGB, 0.0f ~ 1.0f

			struct TextureMap
			{
				std::wstring mapName{};	// fileName.extension
				D3D11_TEXTURE2D_DESC texture2DDesc{};
				Microsoft::WRL::ComPtr<ID3D11SamplerState>			sampler{};
				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	srv{};
			};

			TextureMap diffuseMap{};
		public:
			Material() {}
			~Material() = default;
		public:
			void CreateDiffuseMap( ID3D11Device *pDevice, const D3D11_SAMPLER_DESC &samplerDesc )
			{
				CreateSamplerState
				(
					pDevice,
					&diffuseMap.sampler,
					samplerDesc
				);
				CreateTexture2DFromFile
				(
					pDevice,
					diffuseMap.mapName,
					diffuseMap.srv.GetAddressOf(),
					&diffuseMap.texture2DDesc
				);
			}
		};

		/// <summary>
		/// If file path is invalid, returns false.<para></para>
		/// If setting nullptr to argument, skip that item.<para></para>
		/// these pointers: ID3D11ShaderResourceView, ID3D11SamplerState, D3D11_TEXTURE2D_DESC, bool *, are can setting nullptr.<para></para>
		/// that bool pointer indicate has loaded material or texture.<para></para>
		/// </summary>
		bool LoadObjFile
		(
			ID3D11Device					*piDevice,
			const std::wstring				&objFileName,
			std::vector<DirectX::XMFLOAT3>	*pVertices,
			std::vector<DirectX::XMFLOAT3>	*pNormals,
			std::vector<DirectX::XMFLOAT2>	*pTexCoords,
			std::vector<size_t>				*pIndices,
			std::vector<Material>			*pMaterials,
			bool							*hasLoadedMaterial = nullptr,
			bool isEnableCache = true
		);

		void ReleaseAllObjFileCaches();

		#pragma endregion

		/// <summary>
		/// I doing:<para></para>
		/// ReleaseAllVertexShaderCaches,<para></para>
		/// ReleaseAllPixelShaderCaches,<para></para>
		/// ReleaseAllTexture2DCaches<para></para>
		/// ReleaseAllSamplerStateCaches<para></para>
		/// ReleaseAllObjFileCaches.
		/// </summary>
		void ReleaseAllCachedResources();
	}
}
