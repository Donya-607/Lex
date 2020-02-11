#pragma once

#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>
#include <wrl.h>

#include "Donya/Vector.h"

#include "ModelSource.h"
#include "ModelMaker.h"

namespace Donya
{
	namespace Strategy
	{
		/// <summary>
		/// Use for toggle a Vertex structure by the specification.
		/// </summary>
		class IVertex
		{
		public:
			/// <summary>
			/// Returns: sizeof(struct Vertex);
			/// </summary>
			virtual size_t SizeofVertex() const = 0;
			/// <summary>
			/// Make a vertex-buffer by some Vertex struct.
			/// </summary>
			virtual HRESULT CreateVertexBuffer( ID3D11Device *pDevice, const std::vector<ModelSource::Vertex> &verticesSource, ID3D11Buffer **bufferAddress ) const = 0;
		};

		class VertexSkinned final : public IVertex
		{
		public:
			struct Vertex
			{
				Donya::Vector3	position;
				Donya::Vector3	normal;
				Donya::Vector2	texCoord;
				Donya::Vector4	boneWeights; // Each element is used as like array(e.g. x:[0], y:[1], ...).
				Donya::Int4		boneIndices; // Each element is used as like array(e.g. x:[0], y:[1], ...).
			};
		public:
			size_t SizeofVertex() const override
			{
				return sizeof( Vertex );
			}
			HRESULT CreateVertexBuffer( ID3D11Device *pDevice, const std::vector<ModelSource::Vertex> &verticesSource, ID3D11Buffer **bufferAddress ) const override;
		};
		class VertexStatic final : public IVertex
		{
		public:
			struct Vertex
			{
				Donya::Vector3	position;
				Donya::Vector3	normal;
				Donya::Vector2	texCoord;
			};
		public:
			size_t SizeofVertex() const override
			{
				return sizeof( Vertex );
			}
			HRESULT CreateVertexBuffer( ID3D11Device *pDevice, const std::vector<ModelSource::Vertex> &verticesSource, ID3D11Buffer **bufferAddress ) const override;
		};
	}

	/// <summary>
	/// Build, and store a data of "ModelSource" to usable.
	/// </summary>
	class Model
	{
		template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
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
			std::string							name;

			int									nodeIndex;		// The index of this mesh's node.
			std::vector<int>					nodeIndices;	// The indices of associated nodes with this mesh and this mesh's node.
			std::vector<Donya::Vector4x4>		boneOffsets;	// The bone-offset(inverse initial-pose) matrices of associated nodes. You can access to that associated nodes with the index of "nodeIndices".
			/*
			Note:
			The "boneOffsets" contain values are same as "ModelSource::Mesh::boneOffsets".
			So now implement has an extra array of vec4x4.
			You can more small with changing the type of "boneOffsets" to "unsigned int" from "Vector4x4",
			Then rename to "useBoneOffsetIndices" from "boneOffsets", and store the index of the original array.
			*/

			std::shared_ptr<Strategy::IVertex>	pVertex;
			std::vector<Subset>					subsets;

			ComPtr<ID3D11Buffer>				vertexBuffer;	// Creates by "pVertex".
			ComPtr<ID3D11Buffer>				indexBuffer;
		};
	private:
		std::shared_ptr<ModelSource>	pSource;
		std::string						fileDirectory;	// Use for making file path.
		std::vector<Mesh>				meshes;
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
	};
}
