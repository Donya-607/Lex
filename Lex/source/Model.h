#pragma once

#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>
#include <wrl.h>

#include "Donya/Vector.h"

#include "ModelCommon.h"
#include "ModelSource.h"
#include "ModelPose.h"

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
				// virtual void UnsetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const = 0;
			};

			class StaticVertex : public VertexBase
			{
			private:
				static constexpr size_t BUFFER_COUNT = 2U;
			protected:
				ComPtr<ID3D11Buffer> pBufferTex;
			public:
				HRESULT CreateVertexBufferTex( ID3D11Device *pDevice, const std::vector<Vertex::Tex> &source ) override;
			public:
				virtual void SetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const override;
			};

			class SkinningVertex : public StaticVertex
			{
			private:
				static constexpr size_t BUFFER_COUNT = 3U;
			private:
				ComPtr<ID3D11Buffer> pBufferBone;
			public:
				HRESULT CreateVertexBufferBone( ID3D11Device *pDevice, const std::vector<Vertex::Bone> &source ) override;
			public:
				void SetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const override;
			};
		}

		class StaticModel;
		class SkinningModel;

		/// <summary>
		/// Build, and store a data of "Model::Source" to usable.
		/// </summary>
		class Model
		{
		public:
			/// <summary>
			/// This method is equivalent to call StaticModel::Create().<para></para>
			/// If you set nullptr to "pDevice", use default device.
			/// </summary>
			static StaticModel   CreateStatic( const Source &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice = nullptr );
			/// <summary>
			/// This method is equivalent to call SkinningModel::Create().<para></para>
			/// If you set nullptr to "pDevice", use default device.
			/// </summary>
			static SkinningModel CreateSkinning( const Source &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice = nullptr );
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
			struct Mesh
			{
				std::string								name;

				int										boneIndex;		// The index of associated skeletal with this mesh.
				std::vector<int>						boneIndices;	// The indices of associated skeletal with this mesh and this mesh's node. You can access to that associated skeletal with the index of "nodeIndices", like this: "currentPose = mesh.boneOffsets[i] * model.skeletal[boneIndices[i]].global;".
				std::vector<Animation::Node>			boneOffsets;	// Used as the bone-offset(inverse initial-pose) matrices of model's skeletal. You can access to that associated skeletal with the index of "nodeIndices", like this: "currentPose = mesh.boneOffsets[i] * model.skeletal[boneIndices[i]].global;".
				
				std::shared_ptr<Strategy::VertexBase>	pVertex;
				std::vector<Subset>						subsets;

				ComPtr<ID3D11Buffer>					indexBuffer;
			};
		private:
			std::string			fileDirectory;	// Use for making file path.
			std::vector<Mesh>	meshes;
			Donya::Vector4x4	coordinateConversion;
			bool				initializeResult = false;
		protected: // Prevent a user forgot to call the BuildMyself() when creation.
			Model()								= default;
		public:
			Model( const Model & )				= default;
			Model &operator = ( const Model & )	= default;
			Model( Model && )					= default;
			Model &operator = ( Model && )		= default;
			virtual ~Model()					= default;
		protected:
			bool BuildMyself( const Source &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice );
		private:
			bool InitMeshes( ID3D11Device *pDevice, const Source &loadedSource );
			bool CreateVertexBuffers( ID3D11Device *pDevice, const Source &source );
			bool CreateIndexBuffers( ID3D11Device *pDevice, const Source &source );

			bool InitSubsets( ID3D11Device *pDevice, Model::Mesh *pDestination, const std::vector<Source::Subset> &source );
			bool InitSubset( ID3D11Device *pDevice, Model::Subset *pDestination, const Source::Subset &source );
			void AssignMaterial( Model::Material *pDest, const Source::Material &source );
			bool CreateMaterial( Model::Material *pDestination, ID3D11Device *pDevice );
		protected:
			virtual bool CreateVertices( std::vector<Mesh> *pDest ) = 0;
		public:
			Donya::Vector4x4 GetCoordinateConversion()	const { return coordinateConversion; }
			void SetCoordinateConversion( const Donya::Vector4x4 &newMatrix )
			{
				coordinateConversion = newMatrix;
			}

			/// <summary>
			/// Returns false if the source has not compatible.
			/// </summary>
			bool UpdateMeshColor( const Source &loadedSource );
		public:
			bool WasInitializeSucceeded()				const { return initializeResult; }
			const std::vector<Mesh>	&GetMeshes()		const
			{
				return meshes;
			}
		};

		class StaticModel : public Model
		{
		public:
			/// <summary>
			/// If you set nullptr to "pDevice", use default device.
			/// </summary>
			static StaticModel Create( const Source &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice = nullptr );
		private:
			bool CreateVertices( std::vector<Mesh> *pDest ) override;
		};

		class SkinningModel : public Model
		{
		public:
			/// <summary>
			/// If you set nullptr to "pDevice", use default device.
			/// </summary>
			static SkinningModel Create( const Source &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice = nullptr );
		private:
			bool CreateVertices( std::vector<Mesh> *pDest ) override;
		};
	}
}
