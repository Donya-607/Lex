#pragma once

#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>
#include <wrl.h>

#include "Donya/Vector.h"

#include "ModelCommon.h"
#include "ModelSource.h"
#include "ModelRenderer.h" // For friend declaration.

namespace Donya
{
	namespace Model
	{
		template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

		namespace Strategy
		{
			/// <summary>
			/// Use for toggle a Vertex structure by the specification.
			/// </summary>
			class VertexBase
			{
			protected:
				ComPtr<ID3D11Buffer> pBufferPos;
			public:
				HRESULT CreateVertexBufferPos( ID3D11Device *pDevice, const std::vector<Vertex::Pos> &source );
				virtual HRESULT CreateVertexBufferTex ( ID3D11Device *pDevice, const std::vector<Vertex::Tex>  &source ) { return S_OK; }
				virtual HRESULT CreateVertexBufferBone( ID3D11Device *pDevice, const std::vector<Vertex::Bone> &source ) { return S_OK; }
			public:
				virtual void SetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const = 0;
				// virtual void ResetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const = 0;
			};

			class VertexSkinned final : public VertexBase
			{
			private:
				static constexpr size_t BUFFER_COUNT = 3U;
			private:
				ComPtr<ID3D11Buffer> pBufferTex;
				ComPtr<ID3D11Buffer> pBufferBone;
			public:
				HRESULT CreateVertexBufferTex ( ID3D11Device *pDevice, const std::vector<Vertex::Tex>  &source );
				HRESULT CreateVertexBufferBone( ID3D11Device *pDevice, const std::vector<Vertex::Bone> &source );
			public:
				void SetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const override;
			};
			class VertexStatic final : public VertexBase
			{
			private:
				static constexpr size_t BUFFER_COUNT = 2U;
			private:
				ComPtr<ID3D11Buffer> pBufferTex;
			public:
				HRESULT CreateVertexBufferTex( ID3D11Device *pDevice, const std::vector<Vertex::Tex> &source );
			public:
				void SetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const override;
			};
		}

		/// <summary>
		/// Build, and store a data of "ModelSource" to usable.
		/// </summary>
		class Model
		{
			friend ModelRenderer; // To usable for render.
		public:
			struct Material
			{
				Donya::Vector4						color;
				std::string							textureName;	// Relative file-path. No support to multiple texture currently.
				D3D11_TEXTURE2D_DESC				textureDesc;
				ComPtr<ID3D11ShaderResourceView>	pSRV;
			};
			struct Subset
			{
				std::string		name;
				size_t			indexCount;
				size_t			indexStart;
				Material		ambient;
				Material		diffuse;
				Material		specular;
			};
			/// <summary>
			/// If you have this with some array, you should align the dynamic type of "pVertex".<para></para>
			/// Because I did not support to toggle shading per mesh currently.
			/// </summary>
			struct Mesh
			{
				std::string								name;

				Donya::Vector4x4						coordinateConversion;
				Donya::Vector4x4						globalTransform;

				int										boneIndex;		// The index of this mesh's node.
				std::vector<int>						boneIndices;	// The indices of associated nodes with this mesh and this mesh's node.
				std::vector<Animation::Bone>			boneOffsets;	// Used as the bone-offset(inverse initial-pose) matrices of associated nodes. You can access to that associated nodes with the index of "nodeIndices".
				
				std::shared_ptr<Strategy::VertexBase>	pVertex;
				std::vector<Subset>						subsets;

				ComPtr<ID3D11Buffer>					indexBuffer;
			};
		private:
			std::shared_ptr<ModelSource>	pSource;
			std::string						fileDirectory;	// Use for making file path.
			std::vector<Mesh>				meshes;
		public:
			/// <summary>
			/// If set nullptr to "pDevice", use default device.
			/// </summary>
			Model( ModelSource &rvLoadedSource, const std::string &fileDirectory, ModelUsage usage, ID3D11Device *pDevice = nullptr );
		private:
			// These initialize method are built by "pSource".

			void InitMeshes( ID3D11Device *pDevice, ModelUsage usage );
			void AssignVertexStructure( Model::Mesh *pDestination, ModelUsage usage );
			void CreateBuffers( ID3D11Device *pDevice, Model::Mesh *pDestination, const ModelSource::Mesh &source );

			void InitSubsets( ID3D11Device *pDevice, Model::Mesh *pDestination, const std::vector<ModelSource::Subset> &source );
			void InitSubset( ID3D11Device *pDevice, Model::Subset *pDestination, const ModelSource::Subset &source );
			void CreateMaterial( Model::Material *pDestination, ID3D11Device *pDevice );
		public:
			std::shared_ptr<ModelSource> AcquireModelSource() const
			{
				return pSource;
			}
		};
	}
}
