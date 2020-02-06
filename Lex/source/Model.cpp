#include "Model.h"

#include "Donya/Constant.h"		// Use DEBUG_MODE macro.
#include "Donya/Direct3DUtil.h"	// Use for create some buffers.
#include "Donya/Donya.h"		// Use GetDevice().
#include "Donya/Resource.h"		// Use for create a texture.
#include "Donya/Useful.h"		// Use convert string functions.

namespace Donya
{
	namespace Strategy
	{
		constexpr D3D11_USAGE	BUFFER_USAGE		= D3D11_USAGE_IMMUTABLE;
		constexpr UINT			CPU_ACCESS_FLAG		= NULL;

		// HACK: A difference of these CreateVertexBuffer implements is the "Assign" lambda only.
		// So maybe we can optimize more.

		HRESULT VertexSkinned::CreateVertexBuffer( ID3D11Device *pDevice, const std::vector<ModelSource::Vertex> &source, ID3D11Buffer **bufferAddress ) const
		{
			auto Assign = []( VertexSkinned::Vertex *pDest, const ModelSource::Vertex &source )->void
			{
				pDest->position		= source.position;
				pDest->normal		= source.normal;
				pDest->texCoord		= source.texCoord;
				pDest->boneWeights	= source.boneWeights;
				pDest->boneIndices	= source.boneIndices;
			};

			const size_t vertexCount = source.size();
			std::vector<VertexSkinned::Vertex> dest{ vertexCount };

			for ( size_t i = 0; i < vertexCount; ++i )
			{
				Assign( &dest[i], source[i] );
			}

			HRESULT hr = Donya::CreateVertexBuffer<VertexSkinned::Vertex>
			(
				pDevice, dest,
				BUFFER_USAGE, CPU_ACCESS_FLAG,
				bufferAddress
			);
			return hr;
		}
		
		HRESULT VertexStatic::CreateVertexBuffer( ID3D11Device *pDevice, const std::vector<ModelSource::Vertex> &source, ID3D11Buffer **bufferAddress ) const
		{
			auto Assign = []( VertexStatic::Vertex *pDest, const ModelSource::Vertex &source )->void
			{
				pDest->position		= source.position;
				pDest->normal		= source.normal;
				pDest->texCoord		= source.texCoord;
			};

			const size_t vertexCount = source.size();
			std::vector<VertexStatic::Vertex> dest{};

			for ( size_t i = 0; i < vertexCount; ++i )
			{
				Assign( &dest[i], source[i] );
			}

			HRESULT hr = Donya::CreateVertexBuffer<VertexStatic::Vertex>
			(
				pDevice, dest,
				BUFFER_USAGE, CPU_ACCESS_FLAG,
				bufferAddress
			);
			return hr;
		}
	}

	Model::Model( ModelSource &rvSource, Donya::ModelUsage usage, ID3D11Device *pDevice ) :
		pSource( nullptr ), materials(), meshes()
	{
		pSource = std::make_shared<ModelSource>( std::move( rvSource ) );

		if ( !pDevice )
		{
			pDevice = Donya::GetDevice();
		}

		InitMaterials( pDevice );

		InitMeshes( pDevice, usage );
	}

	void Model::InitMaterials( ID3D11Device *pDevice )
	{
		const size_t materialCount = pSource->materials.size();
		materials.resize( materialCount );

		// Assign source data.
		{
			auto Assign = []( Model::Material *pDest, const ModelSource::Material &source )
			{
				pDest->color		= source.color;
				pDest->textureName	= source.textureName;
			};

			for ( size_t i = 0; i < materialCount; ++i )
			{
				Assign( &materials[i], pSource->materials[i] );
			}
		}

		CreateTextures( pDevice );
	}
	void Model::CreateTextures( ID3D11Device *pDevice )
	{
		auto Load = [&pDevice]( Model::Material &mtl, bool isEnableCache )
		{
			return Resource::CreateTexture2DFromFile
			(
				pDevice,
				Donya::MultiToWide( mtl.textureName ),
				mtl.pSRV.GetAddressOf(),
				&mtl.textureDesc,
				isEnableCache
			);
		};

		bool isEnableCache{};
	#if DEBUG_MODE
		isEnableCache = false;
	#else
		isEnableCache = true;
	#endif // DEBUG_MODE

		bool succeeded = true;
		for ( auto &mtl : materials )
		{
			succeeded = Load( mtl, isEnableCache );
			if ( !succeeded )
			{
				const std::string errMsg =
						"Failed : The model's texture, that is : "
						+ mtl.textureName;
				Donya::OutputDebugStr( errMsg.c_str() );
				_ASSERT_EXPR( 0, Donya::MultiToWide( errMsg ).c_str() );
			}
		}
	}

	void Model::InitMeshes( ID3D11Device *pDevice, Donya::ModelUsage usage )
	{
		const size_t meshCount = pSource->meshes.size();
		meshes.resize( meshCount );

		// Assign source data.
		{
			auto Assign = []( Model::Mesh *pDest, const ModelSource::Mesh &source )
			{
				auto AssignSubset = []( Model::Subset *pDest, const ModelSource::Subset &source )
				{
					pDest->indexCount		= source.indexCount;
					pDest->indexStart		= source.indexStart;
					pDest->useMaterialIndex	= source.materialIndex;
				};

				pDest->nodeIndex	= source.nodeIndex;
				pDest->nodeIndices	= source.nodeIndices;
				pDest->boneOffsets	= source.boneOffsets;

				const size_t subsetCount = source.subsets.size();
				for ( size_t i = 0; i < subsetCount; ++i )
				{
					AssignSubset( &pDest->subsets[i], source.subsets[i] );
				}
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
			CreateBuffers( pDevice, &meshes[i], pSource->meshes[i] );
		}
	}
	void Model::AssignVertexStructure( Model::Mesh *pMesh, Donya::ModelUsage usage )
	{
		switch ( usage )
		{
		case Donya::ModelUsage::Static:
			pMesh->pVertex = std::make_shared<Strategy::VertexStatic>();
			return;
		case Donya::ModelUsage::Skinned:
			pMesh->pVertex = std::make_shared<Strategy::VertexSkinned>();
			return;
		default:
			_ASSERT_EXPR( 0, L"Error : That model-usage is not supported!" );
			return;
		}
	}
	void Model::CreateBuffers( ID3D11Device *pDevice, Model::Mesh *pMesh, const ModelSource::Mesh &source )
	{
		auto Assert = []( const std::string &bufferName )
		{
			const std::string errMsg = "Failed : The model's " + bufferName + "buffer.";
			Donya::OutputDebugStr( errMsg.c_str() );
			_ASSERT_EXPR( 0, Donya::MultiToWide( errMsg ).c_str() );
		};

		HRESULT hr = S_OK;

		hr = pMesh->pVertex->CreateVertexBuffer
		(
			pDevice,
			source.vertices,
			pMesh->vertexBuffer.GetAddressOf()
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
			pMesh->indexBuffer.GetAddressOf()
		);
		if ( FAILED( hr ) )
		{
			Assert( "Index" );
			return;
		}
		// else
	}
}
