#ifndef INCLUDED_RESOURCE_H_
#define INCLUDED_RESOURCE_H_

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
		/// You can register the directory used by Donya::Resource::CreateVertexShaderFromCso().<para></para>
		/// registered name are used just combine before CreateVertexShaderFromCso()'s file name.<para></para>
		/// for exanmple:<para></para>
		/// RegisterDirectoryOfVertexShader( "./Shader/" );<para></para>
		/// CreateVertexShaderFromCso( "foo.cso" ); // Loading cso name is "./Shader/foo.cso".<para></para>
		/// </summary>
		void RegisterDirectoryOfVertexShader( const char *fileDirectory );
		/// <summary>
		/// I doing; read cso-file, CreateVertexShader().<para></para>
		/// If unnecessary ID3D11InputLayout, you can set nullptr.
		/// </summary>
		void CreateVertexShaderFromCso
		(
			ID3D11Device *pd3dDevice,
			const char *csoname,
			const char *openMode,
			ID3D11VertexShader **pd3dVertexShader,
			ID3D11InputLayout **pd3dInputLayout,
			D3D11_INPUT_ELEMENT_DESC *d3dInputElementsDesc,
			size_t									inputElementSize,
			bool enableCache
		);

		void ReleaseAllVertexShaderCaches();

		/// <summary>
		/// You can register the directory used by Donya::Resource::CreatePixelShaderFromCso().<para></para>
		/// registered name are used just combine before CreatePixelShaderFromCso()'s file name.<para></para>
		/// for exanmple:<para></para>
		/// RegisterDirectoryOfPixelShader( "./Shader/" );<para></para>
		/// CreatePixelShaderFromCso( "foo.cso" ); // Loading cso name is "./Shader/foo.cso".<para></para>
		/// </summary>
		void RegisterDirectoryOfPixelShader( const char *fileDirectory );
		/// <summary>
		/// I doing; read cso-file, CreatePixelShader().
		/// </summary>
		void CreatePixelShaderFromCso
		(
			ID3D11Device *pd3dDevice,
			const char *csoname,
			const char *openMode,
			ID3D11PixelShader **pd3dPixelShader,
			bool enableCache
		);

		void ReleaseAllPixelShaderCaches();

	#pragma endregion

	#pragma region Texture

		/// <summary>
		/// You can register the directory used by Donya::Resource::CreateTexture2DFromFile().<para></para>
		/// registered name are used just combine before CreateTexture2DFromFile()'s file name.<para></para>
		/// for exanmple:<para></para>
		/// RegisterDirectoryOfTexture( L"./Data/Images/" );<para></para>
		/// CreateTexture2DFromFile( L"foo/bar.png" ); // Loading cso name is L"./Data/Images/foo/bar.png".<para></para>
		/// </summary>
		void RegisterDirectoryOfTexture( const wchar_t *fileDirectory );
		/// <summary>
		/// I doing; CreateWICTextureFromFile(), QueryInterface(),<para></para>
		/// ID3D11Texture2D::GetDesc(), CreateSamplerState(), CreateShaderResourceView().<para></para>
		/// These arguments must be not null.
		/// </summary>
		void CreateTexture2DFromFile
		(
			ID3D11Device *pd3dDevice,
			const std::wstring &filename,
			ID3D11ShaderResourceView **pd3dShaderResourceView,
			ID3D11SamplerState **pd3dSamplerState,
			D3D11_TEXTURE2D_DESC *pd3dTexture2DDesc,
			const D3D11_SAMPLER_DESC *pd3dSamplerDesc,
			bool isEnableCache = true
		);

		void ReleaseAllTexture2DCaches();

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
				CreateTexture2DFromFile
				(
					pDevice,
					diffuseMap.mapName,
					diffuseMap.srv.GetAddressOf(),
					diffuseMap.sampler.GetAddressOf(),
					&diffuseMap.texture2DDesc,
					&samplerDesc
				);
			}
		};

		/// <summary>
		/// If setting nullptr to argument, skip that item.<para></para>
		/// these pointers: ID3D11ShaderResourceView, ID3D11SamplerState, D3D11_TEXTURE2D_DESC, bool *, are can setting nullptr.<para></para>
		/// that bool pointer indicate has loaded material or texture.<para></para>
		/// </summary>
		void LoadObjFile
		(
			ID3D11Device *piDevice,
			const std::wstring &objFileName,
			std::vector<DirectX::XMFLOAT3> *pVertices,
			std::vector<DirectX::XMFLOAT3> *pNormals,
			std::vector<DirectX::XMFLOAT2> *pTexCoords,
			std::vector<size_t> *pIndices,
			std::vector<Material> *pMaterials,
			bool *hasLoadedMaterial = nullptr,
			bool isEnableCache = true
		);

		void ReleaseAllObjFileCaches();

	#pragma endregion

	#pragma region ID3DObject

		/// <summary>
		/// Returns invalid ID3D11SamplerState.GetAddressOf.
		/// </summary>
		ID3D11SamplerState **RequireInvalidSamplerState();

	#pragma endregion

		/// <summary>
		/// I doing:<para></para>
		/// ReleaseAllVertexShaderCaches,<para></para>
		/// ReleaseAllPixelShaderCaches,<para></para>
		/// ReleaseAllTexture2DCaches<para></para>
		/// ReleaseAllObjFileCaches.
		/// </summary>
		void ReleaseAllCachedResources();
	}

}

#endif // INCLUDED_RESOURCE_H_