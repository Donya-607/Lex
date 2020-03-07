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

			HRESULT	VertexSkinned::CreateVertexBufferTex ( ID3D11Device *pDevice, const std::vector<Vertex::Tex> &source )
			{
				return MakeBuffer<Vertex::Tex>( source, &pBufferTex, pDevice );
			}
			HRESULT	VertexSkinned::CreateVertexBufferBone( ID3D11Device *pDevice, const std::vector<Vertex::Bone> &source )
			{
				return MakeBuffer<Vertex::Bone>( source, &pBufferBone, pDevice );
			}
			void	VertexSkinned::SetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const
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

			HRESULT	VertexStatic::CreateVertexBufferTex( ID3D11Device *pDevice, const std::vector<Vertex::Tex> &source )
			{
				return MakeBuffer<Vertex::Tex>( source, &pBufferTex, pDevice );
			}
			void	VertexStatic::SetVertexBuffers( ID3D11DeviceContext *pImmediateContext ) const
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
		}

		Model::Model( ModelSource &rvSource, const std::string &fileDirectory, ModelUsage usage, ID3D11Device *pDevice ) :
			pSource( nullptr ), fileDirectory( fileDirectory ), meshes()
		{
			pSource = std::make_shared<ModelSource>( std::move( rvSource ) );

			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			InitMeshes( pDevice, usage );
		}

		void Model::InitMeshes( ID3D11Device *pDevice, ModelUsage usage )
		{
			const size_t meshCount = pSource->meshes.size();
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
					Assign( &meshes[i], pSource->meshes[i] );
				}
			}

			for ( auto &mesh : meshes )
			{
				AssignVertexStructure( &mesh, usage );
			}
			for ( size_t i = 0; i < meshCount; ++i )
			{
				InitSubsets( pDevice, &meshes[i], pSource->meshes[i].subsets );
			}
			for ( size_t i = 0; i < meshCount; ++i )
			{
				CreateBuffers( pDevice, &meshes[i], pSource->meshes[i] );
			}
		}
		void Model::AssignVertexStructure( Model::Mesh *pDest, ModelUsage usage )
		{
			switch ( usage )
			{
			case ModelUsage::Static:
				pDest->pVertex = std::make_shared<Strategy::VertexStatic>();
				return;
			case ModelUsage::Skinned:
				pDest->pVertex = std::make_shared<Strategy::VertexSkinned>();
				return;
			default:
				_ASSERT_EXPR( 0, L"Error : That model-usage is not supported!" );
				return;
			}
		}
		void Model::CreateBuffers( ID3D11Device *pDevice, Model::Mesh *pDest, const ModelSource::Mesh &source )
		{
			auto Assert = []( const std::string &bufferName )
			{
				AssertCreation( "buffer", bufferName );
			};

			HRESULT hr = S_OK;

			hr = pDest->pVertex->CreateVertexBufferPos( pDevice, source.positions );
			if ( FAILED( hr ) ) { Assert( "Vertex::Pos" ); return; }
			// else
			hr = pDest->pVertex->CreateVertexBufferTex( pDevice, source.texCoords );
			if ( FAILED( hr ) ) { Assert( "Vertex::Tex" ); return; }
			// else
			hr = pDest->pVertex->CreateVertexBufferBone( pDevice, source.boneInfluences );
			if ( FAILED( hr ) ) { Assert( "Vertex::Bone" ); return; }
			// else

			hr = Donya::CreateIndexBuffer
			(
				pDevice,
				source.indices,
				pDest->indexBuffer.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				Assert( "Index" );
				return;
			}
			// else
		}

		void Model::InitSubsets( ID3D11Device *pDevice, Model::Mesh *pDest, const std::vector<ModelSource::Subset> &source )
		{
			const size_t subsetCount = source.size();
			pDest->subsets.resize( subsetCount );

			for ( size_t i = 0; i < subsetCount; ++i )
			{
				InitSubset( pDevice, &pDest->subsets[i], source[i] );
			}
		}
		void Model::InitSubset( ID3D11Device *pDevice, Model::Subset *pDest, const ModelSource::Subset &source )
		{
			auto AssignMaterial = []( Model::Material *pDest, const ModelSource::Material &source )
			{
				pDest->color		= source.color;
				pDest->textureName	= source.textureName;
			};
			
			pDest->name			= source.name;
			pDest->indexCount	= source.indexCount;
			pDest->indexStart	= source.indexStart;

			AssignMaterial( &pDest->ambient,	source.ambient	);
			CreateMaterial( &pDest->ambient,	pDevice			);

			AssignMaterial( &pDest->diffuse,	source.diffuse	);
			CreateMaterial( &pDest->diffuse,	pDevice			);

			AssignMaterial( &pDest->specular,	source.specular	);
			CreateMaterial( &pDest->specular,	pDevice			);
		}
		void Model::CreateMaterial( Model::Material *pDest, ID3D11Device *pDevice )
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
		}
	}
}
