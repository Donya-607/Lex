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
	namespace Strategy
	{
		constexpr D3D11_USAGE	BUFFER_USAGE		= D3D11_USAGE_IMMUTABLE;
		constexpr UINT			CPU_ACCESS_FLAG		= NULL;

		HRESULT AssertNotAligned()
		{
			_ASSERT_EXPR( 0, L"Error : Passed model's data array sizes aren't aligned!" );
			return E_INVALIDARG;
		}

		// HACK : Maybe we can optimize more these assign templates.
		// Because a difference of these is the part of duck-typing only.

		template<typename SomeVertex>
		void AssignPositions( std::vector<SomeVertex> *pDestVertices, const std::vector<Donya::Vertex::Pos> &source )
		{
			if ( pDestVertices->size() != source.size() )
			{
				HRESULT hr = AssertNotAligned();
				return;
			}
			// else

			const size_t size = source.size();
			for ( size_t i = 0; i < size; ++i )
			{
				pDestVertices->at( i ).position	= source[i];
			}
		}
		template<typename SomeVertex>
		void AssignTexCoords( std::vector<SomeVertex> *pDestVertices, const std::vector<Donya::Vertex::Tex> &source )
		{
			if ( pDestVertices->size() != source.size() )
			{
				HRESULT hr = AssertNotAligned();
				return;
			}
			// else

			const size_t size = source.size();
			for ( size_t i = 0; i < size; ++i )
			{
				pDestVertices->at( i ).texCoord	= source[i];
			}
		}
		template<typename SomeVertex>
		void AssignBoneInfluences( std::vector<SomeVertex> *pDestVertices, const std::vector<Donya::Vertex::Bone> &source )
		{
			if ( pDestVertices->size() != source.size() )
			{
				HRESULT hr = AssertNotAligned();
				return;
			}
			// else

			const size_t size = source.size();
			for ( size_t i = 0; i < size; ++i )
			{
				pDestVertices->at( i ).boneInfluence = source[i];
			}
		}

		HRESULT VertexSkinned::CreateVertexBuffer( ID3D11Device *pDevice, const std::vector<Donya::Vertex::Pos> &srcPos, const std::vector<Donya::Vertex::Tex> &srcTex, const std::vector<Donya::Vertex::Bone> &srcInfl )
		{
			// I expect each source array sizes is the same.
			if ( srcPos.size() != srcTex.size() || srcPos.size() != srcInfl.size() || srcTex.size() != srcInfl.size() )
			{
				return AssertNotAligned();
			}
			// else

			const size_t vertexCount = srcPos.size();
			std::vector<VertexSkinned::Vertex> dest{ vertexCount };

			AssignPositions( &dest, srcPos );
			AssignTexCoords( &dest, srcTex );
			AssignBoneInfluences( &dest, srcInfl );

			HRESULT hr = Donya::CreateVertexBuffer<VertexSkinned::Vertex>
			(
				pDevice, dest,
				BUFFER_USAGE, CPU_ACCESS_FLAG,
				pBuffer.GetAddressOf()
			);
			return hr;
		}
		
		HRESULT VertexStatic::CreateVertexBuffer( ID3D11Device *pDevice, const std::vector<Donya::Vertex::Pos> &srcPos, const std::vector<Donya::Vertex::Tex> &srcTex, const std::vector<Donya::Vertex::Bone> &srcInfl )
		{
			// I expect each source array sizes is the same.
			if ( srcPos.size() != srcTex.size() || srcPos.size() != srcInfl.size() || srcTex.size() != srcInfl.size() )
			{
				return AssertNotAligned();
			}
			// else

			const size_t vertexCount = srcPos.size();
			std::vector<VertexStatic::Vertex> dest{ vertexCount };

			AssignPositions( &dest, srcPos );
			AssignTexCoords( &dest, srcTex );
			// AssignBoneInfluences( &dest, srcInfl );

			HRESULT hr = Donya::CreateVertexBuffer<VertexStatic::Vertex>
			(
				pDevice, dest,
				BUFFER_USAGE, CPU_ACCESS_FLAG,
				pBuffer.GetAddressOf()
			);
			return hr;
		}
	}

	Model::Model( ModelSource &rvSource, const std::string &fileDirectory, Donya::ModelUsage usage, ID3D11Device *pDevice ) :
		pSource( nullptr ), fileDirectory( fileDirectory ), meshes()
	{
		pSource = std::make_shared<ModelSource>( std::move( rvSource ) );

		if ( !pDevice )
		{
			pDevice = Donya::GetDevice();
		}

		InitMeshes( pDevice, usage );

		InitSkeletal();
	}

	void Model::InitMeshes( ID3D11Device *pDevice, Donya::ModelUsage usage )
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
	void Model::AssignVertexStructure( Model::Mesh *pDest, Donya::ModelUsage usage )
	{
		switch ( usage )
		{
		case Donya::ModelUsage::Static:
			pDest->pVertex = std::make_shared<Strategy::VertexStatic>();
			return;
		case Donya::ModelUsage::Skinned:
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

		hr = pDest->pVertex->CreateVertexBuffer
		(
			pDevice,
			source.positions,
			source.texCoords,
			source.boneInfluences
		);
		if ( FAILED( hr ) )
		{
			Assert( "Vertex" );
			return;
		}
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

	void Model::InitSkeletal()
	{
		auto AssignBone = []( Model::Bone *pDest, const Animation::Bone &source )
		{
			pDest->name			= source.name;
			pDest->parentIndex	= source.parentIndex;
			pDest->scale		= source.scale;
			pDest->rotation		= source.rotation;
			pDest->translation	= source.translation;
		};

		const size_t boneCount = pSource->skeletal.size();
		skeletal.resize( ( boneCount ) );
		for ( size_t i = 0; i < boneCount; ++i )
		{
			AssignBone( &skeletal[i], pSource->skeletal[i] );
		}
	}
}
