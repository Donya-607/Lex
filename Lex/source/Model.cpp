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

		// The meshes must be initialized after the materials.
		// Because the Subset is using an index of materials.

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

		// Load textures.
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
			for ( size_t i = 0; i < materialCount; ++i )
			{
				succeeded = Load( materials[i], isEnableCache );
				if ( !succeeded )
				{
					const std::string errMsg = "Failed : The texture load is failed, that is : " + materials[i].textureName;
					Donya::OutputDebugStr( errMsg.c_str() );
					_ASSERT_EXPR( 0, Donya::MultiToWide( errMsg ).c_str() );
				}
			}
		}
	}
	void Model::InitMeshes( ID3D11Device *pDevice, Donya::ModelUsage usage )
	{

	}
}
