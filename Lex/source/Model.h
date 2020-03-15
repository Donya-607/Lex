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
		/// Build, and store a data of "ModelSource" to usable.
		/// </summary>
		class Model
		{
		public:
			/// <summary>
			/// This method is equivalent to call StaticModel::Create().<para></para>
			/// If you set nullptr to "pDevice", use default device.
			/// </summary>
			static std::unique_ptr<StaticModel> CreateStatic( const ModelSource &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice = nullptr );
			/// <summary>
			/// This method is equivalent to call SkinningModel::Create().<para></para>
			/// If you set nullptr to "pDevice", use default device.
			/// </summary>
			static std::unique_ptr<SkinningModel> CreateSkinning( const ModelSource &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice = nullptr );
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

				Donya::Vector4x4						coordinateConversion;
				// Donya::Vector4x4						globalTransform;

				int										boneIndex;		// The index of this mesh's bone.
				std::vector<int>						boneIndices;	// The indices of associated bone-offset matrix.
				std::vector<Animation::Bone>			boneOffsets;	// Used as the bone-offset(inverse initial-pose) matrices of associated nodes. You can access to that associated nodes with the index of "nodeIndices".
				
				std::shared_ptr<Strategy::VertexBase>	pVertex;
				std::vector<Subset>						subsets;

				ComPtr<ID3D11Buffer>					indexBuffer;
			};
		private:
			std::string			fileDirectory;	// Use for making file path.
			std::vector<Mesh>	meshes;
			Pose				pose;
		protected: // Prevent a user forgot to call the BuildMyself() when creation.
			Model()								= default;
		public:
			Model( const Model & )				= default;
			Model &operator = ( const Model & )	= default;
			Model( Model && )					= default;
			Model &operator = ( Model && )		= default;
			virtual ~Model()					= default;
		protected:
			bool BuildMyself( const ModelSource &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice );
		private:
			bool InitMeshes( ID3D11Device *pDevice, const ModelSource &loadedSource );
			bool CreateVertexBuffers( ID3D11Device *pDevice, const ModelSource &source );
			bool CreateIndexBuffers( ID3D11Device *pDevice, const ModelSource &source );

			bool InitSubsets( ID3D11Device *pDevice, Model::Mesh *pDestination, const std::vector<ModelSource::Subset> &source );
			bool InitSubset( ID3D11Device *pDevice, Model::Subset *pDestination, const ModelSource::Subset &source );
			bool CreateMaterial( Model::Material *pDestination, ID3D11Device *pDevice );

			bool InitPose( const ModelSource &loadedSource );
		protected:
			virtual bool CreateVertices( std::vector<Mesh> *pDest ) = 0;
		public:
			/// <summary>
			/// Assign a skeletal if that has compatible with the skeletal of me. That result will return.
			/// </summary>
			bool UpdateSkeletal( const std::vector<Animation::Bone> &currentSkeletal );
			/// <summary>
			/// Assign a skeletal if that has compatible with the skeletal of me. That result will return.
			/// </summary>
			bool UpdateSkeletal( const Animation::KeyFrame &currentSkeletal );
		public:
			const Pose				&GetPose()		const
			{
				return pose;
			}
			const std::vector<Mesh>	&GetMeshes()	const
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
			static std::unique_ptr<StaticModel> Create( const ModelSource &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice = nullptr );
		private:
			bool CreateVertices( std::vector<Mesh> *pDest ) override;
		};

		class SkinningModel : public Model
		{
		public:
			/// <summary>
			/// If you set nullptr to "pDevice", use default device.
			/// </summary>
			static std::unique_ptr<SkinningModel> Create( const ModelSource &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice = nullptr );
		private:
			bool CreateVertices( std::vector<Mesh> *pDest ) override;
		};
	}
}
