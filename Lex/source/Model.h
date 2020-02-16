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
	template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	namespace Strategy
	{
		/// <summary>
		/// Use for toggle a Vertex structure by the specification.
		/// </summary>
		class VertexBase
		{
		protected:
			ComPtr<ID3D11Buffer> pBuffer; // Internal vertex-buffer.
		public:
			ID3D11Buffer *Get() const
			{
				return pBuffer.Get();
			}
			ID3D11Buffer * const *GetAddressOf() const
			{
				return pBuffer.GetAddressOf();
			}
		public:
			/// <summary>
			/// Returns: sizeof(struct Vertex);
			/// </summary>
			virtual size_t  SizeofVertex() const = 0;
			/// <summary>
			/// Make an internal vertex-buffer.
			/// </summary>
			virtual HRESULT CreateVertexBuffer
			(
				ID3D11Device *pDevice,
				const std::vector<Donya::Vertex::Pos>  &sourcePositions,
				const std::vector<Donya::Vertex::Tex>  &sourceTexCoords,
				const std::vector<Donya::Vertex::Bone> &sourceBoneInfluences
			) = 0;
		};

		class VertexSkinned final : public VertexBase
		{
		public:
			struct Vertex
			{
				Donya::Vertex::Pos	position;
				Donya::Vertex::Tex	texCoord;
				Donya::Vertex::Bone	boneInfluence;
			};
		public:
			size_t  SizeofVertex() const override
			{
				return sizeof( Vertex );
			}
			HRESULT CreateVertexBuffer
			(
				ID3D11Device *pDevice,
				const std::vector<Donya::Vertex::Pos>  &sourcePositions,
				const std::vector<Donya::Vertex::Tex>  &sourceTexCoords,
				const std::vector<Donya::Vertex::Bone> &sourceBoneInfluences
			) override;
		};
		class VertexStatic final : public VertexBase
		{
		public:
			struct Vertex
			{
				Donya::Vertex::Pos	position;
				Donya::Vertex::Tex	texCoord;
			};
		public:
			size_t SizeofVertex() const override
			{
				return sizeof( Vertex );
			}
			HRESULT CreateVertexBuffer
			(
				ID3D11Device *pDevice,
				const std::vector<Donya::Vertex::Pos>  &sourcePositions,
				const std::vector<Donya::Vertex::Tex>  &sourceTexCoords,
				const std::vector<Donya::Vertex::Bone> &sourceBoneInfluences
			) override;
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
			std::vector<Donya::Vector4x4>			boneOffsets;	// The bone-offset(inverse initial-pose) matrices of associated nodes. You can access to that associated nodes with the index of "nodeIndices".
			/*
			Note:
			The "boneOffsets" contain values are same as "ModelSource::Mesh::boneOffsets".
			So now implement has an extra array of vec4x4.
			You can more small with changing the type of "boneOffsets" to "unsigned int" from "Vector4x4",
			Then rename to "useBoneOffsetIndices" from "boneOffsets", and store the index of the original array.
			*/

			std::shared_ptr<Strategy::VertexBase>	pVertex;
			std::vector<Subset>						subsets;

			ComPtr<ID3D11Buffer>					indexBuffer;
		};
		/// <summary>
		/// The stored transforming data are local space(Calculated in: ParentGlobal.Inverse * Global).<para></para>
		/// The "parentIndex" is valid only if used as an index of the Bone's array(it means skeletal).
		/// </summary>
		struct Bone
		{
			std::string			name;
			int					parentIndex = -1; // -1 is invalid.
			Donya::Vector3		scale{ 1.0f, 1.0f, 1.0f };
			Donya::Quaternion	rotation;
			Donya::Vector3		translation;
		};
	private:
		std::shared_ptr<ModelSource>	pSource;
		std::string						fileDirectory;	// Use for making file path.
		std::vector<Mesh>				meshes;
		std::vector<Bone>				skeletal;		// Represent bones of initial pose(like T-pose).
	public:
		/// <summary>
		/// If set nullptr to "pDevice", use default device.
		/// </summary>
		Model( ModelSource &rvLoadedSource, const std::string &fileDirectory, Donya::ModelUsage usage, ID3D11Device *pDevice = nullptr );
	private:
		// These initialize method are built by "pSource".

		void InitMeshes( ID3D11Device *pDevice, Donya::ModelUsage usage );
		void AssignVertexStructure( Model::Mesh *pDestination, Donya::ModelUsage usage );
		void CreateBuffers( ID3D11Device *pDevice, Model::Mesh *pDestination, const ModelSource::Mesh &source );

		void InitSubsets( ID3D11Device *pDevice, Model::Mesh *pDestination, const std::vector<ModelSource::Subset> &source );
		void InitSubset( ID3D11Device *pDevice, Model::Subset *pDestination, const ModelSource::Subset &source );
		void CreateMaterial( Model::Material *pDestination, ID3D11Device *pDevice );

		void InitSkeletal();
	};
}
