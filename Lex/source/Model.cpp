#include "Model.h"

#include "Donya/Constant.h"		// Use DEBUG_MODE macro.
#include "Donya/Direct3DUtil.h"	// Use for create some buffers.
#include "Donya/Donya.h"		// Use GetDevice().
#include "Donya/Resource.h"		// Use for create a texture.
#include "Donya/Useful.h"		// Use convert string functions.

namespace
{
	void AssertCreation( const std::string &contentName, const std::string &identityName )
	{
		const std::string errMsg =
			"Failed : The creation of model's " + contentName +
			", that is : " + identityName + ".";
		Donya::OutputDebugStr( errMsg.c_str() );
		_ASSERT_EXPR( 0, Donya::MultiToWide( errMsg ).c_str() );
	}
}

namespace Donya
{
	namespace Model
	{
		namespace Strategy
		{
			constexpr D3D11_USAGE	BUFFER_USAGE		= D3D11_USAGE_IMMUTABLE;
			constexpr UINT			CPU_ACCESS_FLAG		= NULL;

			template<typename Struct>
			HRESULT	MakeBuffer( const std::vector<Struct> &source, ComPtr<ID3D11Buffer> *pComBuffer, ID3D11Device *pDevice )
			{
				return Donya::CreateVertexBuffer<Struct>
				(
					pDevice, source,
					BUFFER_USAGE, CPU_ACCESS_FLAG,
					pComBuffer->GetAddressOf()
				);
			}
			
			HRESULT	VertexBase::CreateVertexBufferPos( ID3D11Device *pDevice, const std::vector<Vertex::Pos> &source )
			{
				return MakeBuffer<Vertex::Pos>( source, &pBufferPos, pDevice );
			}

			HRESULT	StaticVertex::CreateVertexBufferTex( ID3D11Device *pDevice, const std::vector<Vertex::Tex> &source )
			{
				return MakeBuffer<Vertex::Tex>( source, &pBufferTex, pDevice );
			}
			void	StaticVertex::SetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const
			{
				constexpr std::array<UINT, BUFFER_COUNT> strides
				{
					sizeof( Vertex::Pos ),
					sizeof( Vertex::Tex )
				};
				constexpr std::array<UINT, BUFFER_COUNT> offsets
				{
					0,
					0
				};
				std::array<ID3D11Buffer *, BUFFER_COUNT> bufferPointers
				{
					pBufferPos.Get(),
					pBufferTex.Get()
				};

				pImmediateContext->IASetVertexBuffers
				(
					0,
					BUFFER_COUNT,
					bufferPointers.data(),
					strides.data(),
					offsets.data()
				);
			}

			HRESULT	SkinningVertex::CreateVertexBufferBone( ID3D11Device *pDevice, const std::vector<Vertex::Bone> &source )
			{
				return MakeBuffer<Vertex::Bone>( source, &pBufferBone, pDevice );
			}
			void	SkinningVertex::SetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const
			{
				constexpr std::array<UINT, BUFFER_COUNT> strides
				{
					sizeof( Vertex::Pos  ),
					sizeof( Vertex::Tex  ),
					sizeof( Vertex::Bone )
				};
				constexpr std::array<UINT, BUFFER_COUNT> offsets
				{
					0,
					0,
					0
				};
				std::array<ID3D11Buffer *, BUFFER_COUNT> bufferPointers
				{
					pBufferPos.Get(),
					pBufferTex.Get(),
					pBufferBone.Get()
				};

				pImmediateContext->IASetVertexBuffers
				(
					0,
					BUFFER_COUNT,
					bufferPointers.data(),
					strides.data(),
					offsets.data()
				);
			}
		}


		std::unique_ptr<StaticModel> Model::CreateStatic( const ModelSource &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice )
		{
			return StaticModel::Create( loadedSource, fileDirectory, pDevice );
		}
		std::unique_ptr<SkinningModel> Model::CreateSkinning( const ModelSource &loadedSource, const std::string &fileDirectory, ID3D11Device *pDevice )
		{
			return SkinningModel::Create( loadedSource, fileDirectory, pDevice );
		}

		bool Model::BuildMyself( const ModelSource &source, const std::string &argFileDirectory, ID3D11Device *pDevice )
		{
			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			fileDirectory	= argFileDirectory;

			bool result		= true;
			bool succeeded	= true;

			result = InitMeshes( pDevice, source );
			if ( !result ) { succeeded = false; }
			result = InitPose( source );
			if ( !result ) { succeeded = false; }

			return succeeded;
		}

		bool Model::InitMeshes( ID3D11Device *pDevice, const ModelSource &source )
		{
			const size_t meshCount = source.meshes.size();
			meshes.resize( meshCount );

			// Assign source data.
			{
				auto Assign = []( Model::Mesh *pDest, const ModelSource::Mesh &source )
				{
					pDest->name					= source.name;
					pDest->coordinateConversion	= source.coordinateConversion;
					pDest->globalTransform		= source.globalTransform;
					pDest->boneIndex			= source.boneIndex;
					pDest->boneIndices			= source.boneIndices;
					pDest->boneOffsets			= source.boneOffsets;
				};

				for ( size_t i = 0; i < meshCount; ++i )
				{
					Assign( &meshes[i], source.meshes[i] );
				}
			}

			bool result		= true;
			bool succeeded	= true;

			result = CreateVertices( meshCount );
			if ( !result ) { succeeded = false; }
			result = CreateVertexBuffers( pDevice, source );
			if ( !result ) { succeeded = false; }
			result = CreateIndexBuffers ( pDevice, source );
			if ( !result ) { succeeded = false; }

			for ( size_t i = 0; i < meshCount; ++i )
			{
				result = InitSubsets( pDevice, &meshes[i], source.meshes[i].subsets );
				if ( !result ) { succeeded = false; }
			}

			return succeeded;
		}
		bool Model::CreateIndexBuffers( ID3D11Device *pDevice, const ModelSource &source )
		{
			HRESULT hr = S_OK;

			// Expect the meshes already has resized.
			const size_t meshCount = meshes.size();
			for ( size_t i = 0; i < meshCount; ++i )
			{
				hr = Donya::CreateIndexBuffer
				(
					pDevice,
					source.meshes[i].indices,
					meshes[i].indexBuffer.GetAddressOf()
				);
				if ( FAILED( hr ) )
				{
					AssertCreation( "buffer", "Index" );
					return false;
				}
			}

			return true;
		}

		bool Model::InitSubsets( ID3D11Device *pDevice, Model::Mesh *pDest, const std::vector<ModelSource::Subset> &source )
		{
			const size_t subsetCount = source.size();
			pDest->subsets.resize( subsetCount );

			bool succeeded = true;
			for ( size_t i = 0; i < subsetCount; ++i )
			{
				bool  result = InitSubset( pDevice, &pDest->subsets[i], source[i] );
				if ( !result ) { succeeded = false; }
			}
			return succeeded;
		}
		bool Model::InitSubset( ID3D11Device *pDevice, Model::Subset *pDest, const ModelSource::Subset &source )
		{
			pDest->name			= source.name;
			pDest->indexCount	= source.indexCount;
			pDest->indexStart	= source.indexStart;

			auto AssignMaterial = []( Model::Material *pDest, const ModelSource::Material &source )
			{
				pDest->color		= source.color;
				pDest->textureName	= source.textureName;
			};
			
			struct Bundle { Model::Material *dest; const ModelSource::Material &source; };
			Bundle createList[]
			{
				{ &pDest->ambient,  source.ambient  },
				{ &pDest->diffuse,  source.diffuse  },
				{ &pDest->specular, source.specular },
			};

			bool succeeded = true;
			for ( auto &it : createList )
			{
				/*	INDENT	*/ AssignMaterial( it.dest, it.source );
				bool  result = CreateMaterial( it.dest, pDevice   );
				if ( !result ) { succeeded = false; }
			}
			return succeeded;
		}
		bool Model::CreateMaterial( Model::Material *pDest, ID3D11Device *pDevice )
		{
			// This method regard as the material info was already fetched by source data.
			// Because this behave only create a texture.

			bool isEnableCache{};
		#if DEBUG_MODE
			isEnableCache = false;
		#else
			isEnableCache = true;
		#endif // DEBUG_MODE

			bool succeeded = true;
			bool hasTexture = ( pDest->textureName.empty() ) ? false : true;
			if ( hasTexture )
			{
				succeeded = Resource::CreateTexture2DFromFile
				(
					pDevice,
					Donya::MultiToWide( fileDirectory + pDest->textureName ),
					pDest->pSRV.GetAddressOf(),
					&pDest->textureDesc,
					isEnableCache
				);
			}
			else
			{
				pDest->textureName = "[EMPTY]";
				Resource::CreateUnicolorTexture
				(
					pDevice,
					pDest->pSRV.GetAddressOf(),
					&pDest->textureDesc,
					isEnableCache
				);
			}


			if ( !succeeded )
			{
				AssertCreation( "texture", fileDirectory + pDest->textureName );
			}
			return succeeded;
		}

		bool Model::InitPose( const ModelSource &source )
		{
			pose.AssignSkeletal( source.skeletal );
			pose.UpdateTransformMatrices();

			return true;
		}

		bool Model::UpdateSkeletal( const std::vector<Animation::Bone> &currentSkeletal )
		{
			if ( !pose.HasCompatibleWith( currentSkeletal ) ) { return false; }
			// else

			pose.AssignSkeletal( currentSkeletal );
			pose.UpdateTransformMatrices();
		}
		bool Model::UpdateSkeletal( const Animation::KeyFrame &currentSkeletal )
		{
			return UpdateSkeletal( currentSkeletal.keyPose );
		}



		template<class StrategyVertex>
		bool CreateVertexBuffersImpl( ID3D11Device *pDevice, std::vector<std::unique_ptr<StrategyVertex>> *ppVertices, const ModelSource &modelSource )
		{
			auto Assert = []( const std::string &kindName, size_t vertexIndex )
			{
				const std::string indexStr = "[" + std::to_string( vertexIndex ) + "]";
				AssertCreation( "buffer", "Vertex::" + kindName + indexStr );
			};

			HRESULT hr			= S_OK;
			size_t  vertexCount	= ppVertices->size();
			for (   size_t i = 0; i < vertexCount; ++i )
			{
				const auto &source = modelSource.meshes[i];
				auto &pVertex = ( *ppVertices )[i];

				hr = pVertex->CreateVertexBufferPos( pDevice, source.positions );
				if ( FAILED( hr ) ) { Assert( "Pos", i ); return false; }
				// else
				hr = pVertex->CreateVertexBufferTex( pDevice, source.texCoords );
				if ( FAILED( hr ) ) { Assert( "Tex", i ); return false; }
				// else
				hr = pVertex->CreateVertexBufferBone( pDevice, source.boneInfluences );
				if ( FAILED( hr ) ) { Assert( "Bone", i ); return false; }
			}

			return true;
		}


		std::unique_ptr<StaticModel> StaticModel::Create( const ModelSource &source, const std::string &fileDirectory, ID3D11Device *pDevice )
		{
			class MakeUniqueEnabler : public StaticModel {};
			MakeUniqueEnabler instance{};
			bool  result = instance.BuildMyself( source, fileDirectory, pDevice );
			if ( !result ) { return nullptr; }
			// else
			return std::move( std::make_unique<MakeUniqueEnabler>( std::move( instance ) ) );
		}

		StaticModel::StaticModel() : Model(), pVertices()
		{}
		bool StaticModel::CreateVertices( size_t meshCount )
		{
			pVertices.resize( meshCount );

			for ( auto &pIt : pVertices )
			{
				pIt = std::make_unique<Strategy::StaticVertex>();
			}

			return true;
		}
		bool StaticModel::CreateVertexBuffers( ID3D11Device *pDevice, const ModelSource &source )
		{
			return CreateVertexBuffersImpl<Strategy::StaticVertex>( pDevice, &pVertices, source );
		}
		void StaticModel::SetVertexBuffers( size_t meshIndex, ID3D11DeviceContext *pImmediateContext ) const
		{
			_ASSERT_EXPR( meshIndex < pVertices.size(), L"Error : Passed index out of range!" );

			// The "StaticVertex::" modifier to choose the Static version even if myself is SkinningModel.
			// Because I want the model classes to support the Static version even if using Skinning version.
			pVertices[meshIndex]->StaticVertex::SetVertexBuffers( pImmediateContext );
		}


		std::unique_ptr<SkinningModel> SkinningModel::Create( const ModelSource &source, const std::string &fileDirectory, ID3D11Device *pDevice )
		{
			class MakeUniqueEnabler : public SkinningModel {};
			MakeUniqueEnabler instance{};
			bool  result = instance.BuildMyself( source, fileDirectory, pDevice );
			if ( !result ) { return nullptr; }
			// else
			return std::move( std::make_unique<MakeUniqueEnabler>( std::move( instance ) ) );
		}

		SkinningModel::SkinningModel() : StaticModel(), pVertices()
		{}
		bool SkinningModel::CreateVertices( size_t meshCount )
		{
			pVertices.resize( meshCount );

			for ( auto &pIt : pVertices )
			{
				pIt = std::make_unique<Strategy::SkinningVertex>();
			}

			return true;
		}
		bool SkinningModel::CreateVertexBuffers( ID3D11Device *pDevice, const ModelSource &source )
		{
			return CreateVertexBuffersImpl<Strategy::SkinningVertex>( pDevice, &pVertices, source );
		}
		void SkinningModel::SetVertexBuffers( size_t meshIndex, ID3D11DeviceContext *pImmediateContext ) const
		{
			_ASSERT_EXPR( meshIndex < pVertices.size(), L"Error : Passed index out of range!" );

			pVertices[meshIndex]->SkinningVertex::SetVertexBuffers( pImmediateContext );
		}
	}
}
