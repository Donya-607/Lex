#include "Loader.h"

#include <array>
#include <crtdbg.h>
#include <Windows.h>

#if USE_FBX_SDK
#include <fbxsdk.h>
#endif // USE_FBX_SDK

#include "Benchmark.h"
#include "Common.h"
#include "Useful.h"

#undef min
#undef max

#if USE_FBX_SDK
namespace FBX = fbxsdk;
#endif // USE_FBX_SDK

namespace Donya
{
	Loader::Loader() :
		absFilePath(), fileName(), fileDirectory(),
		meshes()
	{

	}
	Loader::~Loader()
	{
		meshes.clear();
		meshes.shrink_to_fit();
	}

	std::mutex Loader::cerealMutex{};

#if USE_FBX_SDK
	std::mutex Loader::fbxMutex{};

	Donya::Vector2 Convert( const FBX::FbxDouble2 &source )
	{
		return Donya::Vector2
		{
			scast<float>( source.mData[0] ),
			scast<float>( source.mData[1] )
		};
	}
	Donya::Vector3 Convert( const FBX::FbxDouble3 &source )
	{
		return Donya::Vector3
		{
			scast<float>( source.mData[0] ),
			scast<float>( source.mData[1] ),
			scast<float>( source.mData[2] )
		};
	}
	Donya::Vector4 Convert( const FBX::FbxDouble4 &source )
	{
		return Donya::Vector4
		{
			scast<float>( source.mData[0] ),
			scast<float>( source.mData[1] ),
			scast<float>( source.mData[2] ),
			scast<float>( source.mData[3] )
		};
	}
	void ConvertFloat4x4( DirectX::XMFLOAT4X4 *pOutput, const FBX::FbxAMatrix &affineMatrix )
	{
		for ( int r = 0; r < 4; ++r )
		{
			for ( int c = 0; c < 4; ++c )
			{
				pOutput->m[r][c] = scast<float>( affineMatrix[r][c] );
			}
		}
	}

	void Traverse( FBX::FbxNode *pNode, std::vector<FBX::FbxNode *> *pFetchedMeshes )
	{
		if ( !pNode ) { return; }
		// else

		FBX::FbxNodeAttribute *pNodeAttr = pNode->GetNodeAttribute();
		if ( pNodeAttr )
		{
			auto eType = pNodeAttr->GetAttributeType();
			switch ( eType )
			{
			case FBX::FbxNodeAttribute::eMesh:
				{
					pFetchedMeshes->push_back( pNode );
				}
				break;
			default:
				break;
			}
		}

		int end = pNode->GetChildCount();
		for ( int i = 0; i < end; ++i )
		{
			Traverse( pNode->GetChild( i ), pFetchedMeshes );
		}
	}

	void FetchBoneInfluences( const FBX::FbxMesh *pMesh, std::vector<Loader::BoneInfluencesPerControlPoint> &influences )
	{
		const int ctrlPointCount = pMesh->GetControlPointsCount();
		influences.resize( ctrlPointCount );

		auto FetchInfluenceFromCluster =
		[]( std::vector<Loader::BoneInfluencesPerControlPoint> &influences, const FBX::FbxCluster *pCluster, int clustersIndex )
		{
			const int		ctrlPointIndicesSize	= pCluster->GetControlPointIndicesCount();
			const int		*ctrlPointIndices		= pCluster->GetControlPointIndices();
			const double	*ctrlPointWeights		= pCluster->GetControlPointWeights();

			if ( !ctrlPointIndicesSize || !ctrlPointIndices || !ctrlPointWeights ) { return; }
			// else

			for ( int i = 0; i < ctrlPointIndicesSize; ++i )
			{
				auto	&data	= influences[ctrlPointIndices[i]].cluster;
				float	weight	= scast<float>( ctrlPointWeights[i] );
				data.emplace_back( clustersIndex, weight );
			}
		};
		auto FetchClusterFromSkin =
		[&FetchInfluenceFromCluster]( std::vector<Loader::BoneInfluencesPerControlPoint> &influences, const FBX::FbxSkin *pSkin )
		{
			const int clusterCount = pSkin->GetClusterCount();
			for ( int i = 0; i < clusterCount; ++i )
			{
				const FBX::FbxCluster *pCluster = pSkin->GetCluster( i );
				FetchInfluenceFromCluster( influences, pCluster, i );
			}
		};

		const int deformersCount = pMesh->GetDeformerCount( FBX::FbxDeformer::eSkin );
		for ( int i = 0; i < deformersCount; ++i )
		{
			FBX::FbxSkin *pSkin = scast<FBX::FbxSkin *>( pMesh->GetDeformer( i, FBX::FbxDeformer::eSkin ) );
			
			FetchClusterFromSkin( influences, pSkin );
		}
	}
	void FetchBoneMatrices( FBX::FbxTime time, const FBX::FbxMesh *pMesh, std::vector<Loader::Bone> *pSkeletal )
	{
		auto FetchMatricesFromSkin = []( FBX::FbxTime time, const FBX::FbxMesh *pMesh, const FBX::FbxSkin *pSkin, std::vector<Loader::Bone> *pSkeletal )
		{
			const int clusterCount = pSkin->GetClusterCount();

		#define APPEND ( false )
		#if APPEND
			const size_t oldSkeletalCount = pSkeletal->size();
			pSkeletal->resize( scast<size_t>( clusterCount ) + oldSkeletalCount );
		#else
			pSkeletal->resize( scast<size_t>( clusterCount ) );
		#endif // APPEND
		#undef APPEND

			for ( int i = 0; i < clusterCount; ++i )
			{
				Loader::Bone &bone = pSkeletal->at( i );

				// Each joint.
				const FbxCluster *pCluster = pSkin->GetCluster( i );

				// This matrix transforms coordinates of the initial pose from mesh space to global space.
				FBX::FbxAMatrix reference_global_init_position;
				pCluster->GetTransformMatrix( reference_global_init_position );

				// This matrix transforms coordinates of the initial pose from bone space to global space.
				FBX::FbxAMatrix cluster_global_init_position;
				pCluster->GetTransformLinkMatrix( cluster_global_init_position );

				// This matrix transforms coordinates of the current pose from mesh space to global space.
				FBX::FbxAMatrix reference_global_current_position;
				reference_global_current_position = pMesh->GetNode()->EvaluateGlobalTransform( time );

				// This matrix transforms coordinates of the current pose from bone space to global space.
				FBX::FbxAMatrix cluster_global_current_position;
				cluster_global_current_position = pCluster->GetLink()->EvaluateGlobalTransform( time );

				// FbxAMatrix is column-major, so multiply from right side.
				// "initial : mesh->global->bone", "current : bone->global->mesh"
				// First, transform to initial bone space from initial mesh space.
				// Then,  transfrom to current mesh space from initial bone space.

				FBX::FbxAMatrix transform = reference_global_current_position.Inverse() * cluster_global_current_position
					* cluster_global_init_position.Inverse() * reference_global_init_position;

				// convert FbxAMatrix(transform) to XMFLOAT4X4(bone.transform) 
			}
		};

		const int deformersCount = pMesh->GetDeformerCount( FBX::FbxDeformer::eSkin );
		for ( int i = 0; i < deformersCount; ++i )
		{
			FBX::FbxSkin *pSkin = scast<FBX::FbxSkin *>( pMesh->GetDeformer( i, FBX::FbxDeformer::eSkin ) );

			FetchMatricesFromSkin( time, pMesh, pSkin, pSkeletal );
		}
	}

#endif // USE_FBX_SDK

	bool Loader::Load( const std::string &filePath, std::string *outputErrorString )
	{
		std::string fullPath = ToFullPath( filePath );

	#if USE_FBX_SDK

		auto ShouldUseFBXSDK = []( const std::string &filePath )
		{
			constexpr std::array<const char *, 4> EXTENSIONS
			{
				".obj", ".OBJ",
				".fbx", ".FBX"
			};

			for ( size_t i = 0; i < EXTENSIONS.size(); ++i )
			{
				if ( filePath.find( EXTENSIONS[i] ) != std::string::npos )
				{
					return true;
				}
			}

			return false;
		};

		if ( ShouldUseFBXSDK( fullPath ) )
		{
			return LoadByFBXSDK( fullPath, outputErrorString );
		}
		// else

	#endif // USE_FBX_SDK

		auto ShouldLoadByCereal = []( const std::string &filePath )->const char *
		{
			constexpr std::array<const char *, 1> EXTENSIONS
			{
				".bin"
			};

			for ( size_t i = 0; i < EXTENSIONS.size(); ++i )
			{
				if ( filePath.find( EXTENSIONS[i] ) != std::string::npos )
				{
					return EXTENSIONS[i];
				}
			}

			return "NOT FOUND";
		};

		auto resultExt = ShouldLoadByCereal( fullPath );
		if ( !strcmp( ".bin", resultExt ) )
		{
			return LoadByCereal( fullPath, outputErrorString );
		}

		return false;
	}

	void Loader::SaveByCereal( const std::string &filePath ) const
	{
		Serializer::Extension bin  = Serializer::Extension::BINARY;

		std::lock_guard<std::mutex> lock( cerealMutex );
		
		Serializer seria;
		seria.Save( bin, filePath.c_str(),  SERIAL_ID, *this );
	}
	
	bool Loader::LoadByCereal( const std::string &filePath, std::string *outputErrorString )
	{
		Serializer::Extension ext = Serializer::Extension::BINARY;

		std::lock_guard<std::mutex> lock( cerealMutex );
		
		Serializer seria;
		seria.Load( ext, filePath.c_str(), SERIAL_ID, *this );

		// I should overwrite file-directory after load, because this will overwritten by Serializer::Load().
		fileDirectory = ExtractFileDirectoryFromFullPath( filePath );

		return true;
	}

#if USE_FBX_SDK

#define USE_TRIANGULATE ( false )

	bool Loader::LoadByFBXSDK( const std::string &filePath, std::string *outputErrorString )
	{
		fileDirectory	= ExtractFileDirectoryFromFullPath( filePath );
		fileName		= filePath.substr( fileDirectory.size() );

		MakeAbsoluteFilePath( filePath );

		std::unique_ptr<std::lock_guard<std::mutex>> pLock{}; // Use scoped-lock without code-bracket.
		pLock = std::make_unique<std::lock_guard<std::mutex>>( fbxMutex );

		FBX::FbxManager		*pManager		= FBX::FbxManager::Create();
		FBX::FbxIOSettings	*pIOSettings	= FBX::FbxIOSettings::Create( pManager, IOSROOT );
		pManager->SetIOSettings( pIOSettings );

		auto Uninitialize =
		[&]
		{
			pManager->Destroy();
		};

		FBX::FbxScene *pScene = FBX::FbxScene::Create( pManager, "" );
		#pragma region Import
		{
			FBX::FbxImporter *pImporter		= FBX::FbxImporter::Create( pManager, "" );
			if ( !pImporter->Initialize( absFilePath.c_str(), -1, pManager->GetIOSettings() ) )
			{
				if ( outputErrorString != nullptr )
				{
					*outputErrorString =  "Failed : FbxImporter::Initialize().\n";
					*outputErrorString += "Error message is : ";
					*outputErrorString += pImporter->GetStatus().GetErrorString();
				}

				Uninitialize();
				return false;
			}

			if ( !pImporter->Import( pScene ) )
			{
				if ( outputErrorString != nullptr )
				{
					*outputErrorString =  "Failed : FbxImporter::Import().\n";
					*outputErrorString += "Error message is :";
					*outputErrorString += pImporter->GetStatus().GetErrorString();
				}

				Uninitialize();
				return false;
			}

			pImporter->Destroy();
		}
		#pragma endregion

		pLock.reset( nullptr );

	#ifdef USE_TRIANGULATE
		{
			FBX::FbxGeometryConverter geometryConverter( pManager );
			bool replace = true;
			geometryConverter.Triangulate( pScene, replace );
		}
	#endif

		std::vector<FBX::FbxNode *> fetchedMeshes{};
		Traverse( pScene->GetRootNode(), &fetchedMeshes );

		std::vector<BoneInfluencesPerControlPoint> influencesPerCtrlPoints{};

		size_t meshCount = fetchedMeshes.size();
		meshes.resize( meshCount );
		for ( size_t i = 0; i < meshCount; ++i )
		{
			FBX::FbxMesh *pMesh = fetchedMeshes[i]->GetMesh();

			influencesPerCtrlPoints.clear();
			FetchBoneInfluences( pMesh, influencesPerCtrlPoints );

			FetchVertices( i, pMesh, influencesPerCtrlPoints );
			FetchMaterial( i, pMesh );
			FetchGlobalTransform( i, pMesh );
		}

		Uninitialize();
		return true;
	}

	std::string GetUTF8FullPath( const std::string &inputFilePath, size_t filePathLength = 512U )
	{
		// reference to http://blog.livedoor.jp/tek_nishi/archives/9446152.html

		std::unique_ptr<char[]> fullPath = std::make_unique<char[]>( filePathLength );
		auto writeLength = GetFullPathNameA( inputFilePath.c_str(), filePathLength, fullPath.get(), nullptr );

		char *convertedPath = nullptr;
		FBX::FbxAnsiToUTF8( fullPath.get(), convertedPath );

		std::string convertedStr( convertedPath );

		FBX::FbxFree( convertedPath );

		return convertedStr;
	}
	void Loader::MakeAbsoluteFilePath( const std::string &filePath )
	{
		constexpr size_t FILE_PATH_LENGTH = 512U;

		absFilePath = GetUTF8FullPath( filePath, FILE_PATH_LENGTH );
	}

	void Loader::FetchVertices( size_t meshIndex, const FBX::FbxMesh *pMesh, const std::vector<BoneInfluencesPerControlPoint> &fetchedInfluences )
	{
		const FBX::FbxVector4 *pControlPointsArray = pMesh->GetControlPoints();
		const int mtlCount = pMesh->GetNode()->GetMaterialCount();
		const int polygonCount = pMesh->GetPolygonCount();

		auto &mesh = meshes[meshIndex];

		mesh.subsets.resize( ( !mtlCount ) ? 1 : mtlCount );

		// Calculate subsets start index(not optimized).
		if ( mtlCount )
		{
			// Count the faces each material.
			for ( int i = 0; i < polygonCount; ++i )
			{
				int mtlIndex = pMesh->GetElementMaterial()->GetIndexArray().GetAt( i );
				mesh.subsets[mtlIndex].indexCount += 3;
			}

			// Record the offset (how many vertex)
			int offset = 0;
			for ( auto &subset : mesh.subsets )
			{
				subset.indexStart = offset;
				offset += subset.indexCount;
				// This will be used as counter in the following procedures, reset to zero.
				subset.indexCount = 0;
			}
		}

		size_t vertexCount = 0;

		mesh.indices.resize( polygonCount * 3 );
		for ( int polyIndex = 0; polyIndex < polygonCount; ++polyIndex )
		{
			// The material for current face.
			int mtlIndex = 0;
			if ( mtlCount )
			{
				mtlIndex = pMesh->GetElementMaterial()->GetIndexArray().GetAt( polyIndex );
			}

			// Where should I save the vertex attribute index, according to the material.
			auto &subset = mesh.subsets[mtlIndex];
			int indexOffset = subset.indexStart + subset.indexCount;

			FBX::FbxVector4	fbxNormal;
			Donya::Vector3	normal;
			Donya::Vector3	position;

			size_t size = pMesh->GetPolygonSize( polyIndex );
			for ( size_t v = 0; v < size; ++v )
			{
				pMesh->GetPolygonVertexNormal( polyIndex, v, fbxNormal );
				normal.x = scast<float>( fbxNormal[0] );
				normal.y = scast<float>( fbxNormal[1] );
				normal.z = scast<float>( fbxNormal[2] );

				const int ctrlPointIndex = pMesh->GetPolygonVertex( polyIndex, v );
				position.x = scast<float>( pControlPointsArray[ctrlPointIndex][0] );
				position.y = scast<float>( pControlPointsArray[ctrlPointIndex][1] );
				position.z = scast<float>( pControlPointsArray[ctrlPointIndex][2] );

				mesh.normals.push_back( normal );
				mesh.positions.push_back( position );

				mesh.indices[indexOffset + v] = vertexCount;
				vertexCount++;

				mesh.influences.push_back( fetchedInfluences[ctrlPointIndex] );
			}
			subset.indexCount += size;
		}

		FBX::FbxStringList uvName;
		pMesh->GetUVSetNames( uvName );

		FBX::FbxArray<FBX::FbxVector2> uvs{};
		pMesh->GetPolygonVertexUVs( uvName.GetStringAt( 0 ), uvs );
		for ( int i = 0; i < uvs.GetCount(); ++i )
		{
			float x = scast<float>( uvs[i].mData[0] );
			float y = 1.0f - scast<float>( uvs[i].mData[1] );
			mesh.texCoords.push_back( Donya::Vector2{ x, y } );
		}
	}

	void Loader::FetchMaterial( size_t meshIndex, const FBX::FbxMesh *pMesh )
	{
		FBX::FbxNode *pNode = pMesh->GetNode();
		if ( !pNode ) { return; }
		// else

		int materialCount = pNode->GetMaterialCount();
		if ( materialCount < 1 ) { return; }
		// else

		for ( int i = 0; i < materialCount; ++i )
		{
			FBX::FbxSurfaceMaterial *pMaterial = pNode->GetMaterial( i );
			if ( !pMaterial ) { continue; }
			// else

			AnalyseProperty( meshIndex, i, pMaterial );
		}
	}

	void Loader::AnalyseProperty( size_t meshIndex, int mtlIndex, FBX::FbxSurfaceMaterial *pMaterial )
	{
		enum MATERIAL_TYPE
		{
			NILL = 0,
			LAMBERT,
			PHONG
		};
		MATERIAL_TYPE mtlType = NILL;

		if ( pMaterial->GetClassId().Is( FBX::FbxSurfaceLambert::ClassId ) )
		{
			mtlType = LAMBERT;
		}
		else
		if ( pMaterial->GetClassId().Is( FBX::FbxSurfacePhong::ClassId ) )
		{
			mtlType = PHONG;
		}

		FBX::FbxProperty prop{};
		FBX::FbxProperty factor{};

		auto AssignFbxDouble4 =
		[]( Donya::Vector4 *output, const FBX::FbxDouble3 *input, double factor )
		{
			output->x = scast<float>( input->mData[0] * factor );
			output->y = scast<float>( input->mData[1] * factor );
			output->z = scast<float>( input->mData[2] * factor );
			output->w = 1.0f;
		};
		auto AssignFbxDouble4Process =
		[&]( Donya::Vector4 *output )
		{
			auto entity = prop.Get<FBX::FbxDouble3>();
			double fact = factor.Get<FBX::FbxDouble>();
			AssignFbxDouble4
			(
				output,
				&entity,
				fact
			);
		};
		auto FetchMaterialParam =
		[&]( Loader::Material *pOutMtl, const char *surfaceMtl, const char *surfaceMtlFactor )
		{
			prop	= pMaterial->FindProperty( surfaceMtl );
			factor	= pMaterial->FindProperty( surfaceMtlFactor );

			auto FetchTextures = [&]()->void
			{
				if ( !prop.IsValid() ) { return; }
				// else

				int layerCount = prop.GetSrcObjectCount<FBX::FbxLayeredTexture>();
				if ( layerCount ) { return; }
				// else

				int textureCount = prop.GetSrcObjectCount<FBX::FbxFileTexture>();
				for ( int i = 0; i < textureCount; ++i )
				{
					FBX::FbxFileTexture *texture = prop.GetSrcObject<FBX::FbxFileTexture>( i );
					if ( !texture ) { continue; }
					// else
				
					std::string relativePath = texture->GetRelativeFileName();
					if ( relativePath.empty() )
					{
						std::string fullPath = texture->GetFileName();

						if ( !fullPath.empty() )
						{
							relativePath = fullPath.substr( fileDirectory.size() );
							pOutMtl->relativeTexturePaths.push_back( relativePath );
						}
					}
					else
					{
						pOutMtl->relativeTexturePaths.push_back( relativePath );
					}
				}
			};

			if ( prop.IsValid() )
			{
				FetchTextures();

				if ( factor.IsValid() )
				{
					AssignFbxDouble4Process( &( pOutMtl->color ) );
				}
			}
		};
		
		auto &subset = meshes[meshIndex].subsets[mtlIndex];

		FetchMaterialParam
		(
			&subset.ambient,
			FBX::FbxSurfaceMaterial::sAmbient,
			FBX::FbxSurfaceMaterial::sAmbientFactor
		);
		FetchMaterialParam
		(
			&subset.bump,
			FBX::FbxSurfaceMaterial::sBump,
			FBX::FbxSurfaceMaterial::sBumpFactor
		);
		FetchMaterialParam
		(
			&subset.diffuse,
			FBX::FbxSurfaceMaterial::sDiffuse,
			FBX::FbxSurfaceMaterial::sDiffuseFactor
		);
		FetchMaterialParam
		(
			&subset.emissive,
			FBX::FbxSurfaceMaterial::sEmissive,
			FBX::FbxSurfaceMaterial::sEmissiveFactor
		);
		
		prop = pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sTransparencyFactor );
		if ( prop.IsValid() )
		{
			subset.transparency = scast<float>( prop.Get<FBX::FbxFloat>() );
		}

		if ( mtlType == PHONG )
		{ 
			FetchMaterialParam
			(
				&subset.specular,
				FBX::FbxSurfaceMaterial::sSpecular,
				FBX::FbxSurfaceMaterial::sSpecularFactor
			);

			prop = pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sReflection );
			if ( prop.IsValid() )
			{
				subset.reflection = scast<float>( prop.Get<FBX::FbxFloat>() );
			}

			prop = pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sShininess );
			if ( prop.IsValid() )
			{
				subset.specular.color.w = scast<float>( prop.Get<FBX::FbxFloat>() );
			}
		}
		else
		{
			subset.reflection		= 0.0f;
			subset.transparency		= 0.0f;
			subset.specular.color	= Donya::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f };
		}
	}

	void Loader::FetchGlobalTransform( size_t meshIndex, const fbxsdk::FbxMesh *pMesh )
	{
		FBX::FbxAMatrix globalTransform = pMesh->GetNode()->EvaluateGlobalTransform( 0 );
		ConvertFloat4x4( &meshes[meshIndex].globalTransform, globalTransform );
	}

#endif // USE_FBX_SDK

#if USE_IMGUI
	void Loader::EnumPreservingDataToImGui( const char *ImGuiWindowIdentifier ) const
	{
		ImVec2 childFrameSize( 0.0f, 0.0f );

		size_t meshCount = meshes.size();
		for ( size_t i = 0; i < meshCount; ++i )
		{
			const auto &mesh = meshes[i];
			std::string meshCaption = "Mesh[" + std::to_string( i ) + "]";
			if ( ImGui::TreeNode( meshCaption.c_str() ) )
			{
				size_t verticesCount = mesh.indices.size();
				std::string verticesCaption = "Vertices[Count:" + std::to_string( verticesCount ) + "]";

				if ( ImGui::TreeNode( verticesCaption.c_str() ) )
				{
					if ( ImGui::TreeNode( "Positions" ) )
					{
						auto &ref = mesh.positions;

						ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), childFrameSize );
						size_t end = ref.size();
						for ( size_t i = 0; i < end; ++i )
						{
							ImGui::Text( "[No:%d][X:%6.3f][Y:%6.3f][Z:%6.3f]", i, ref[i].x, ref[i].y, ref[i].z );
						}
						ImGui::EndChild();

						ImGui::TreePop();
					}

					if ( ImGui::TreeNode( "Normals" ) )
					{
						auto &ref = mesh.normals;

						ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), childFrameSize );
						size_t end = ref.size();
						for ( size_t i = 0; i < end; ++i )
						{
							ImGui::Text( "[No:%d][X:%6.3f][Y:%6.3f][Z:%6.3f]", i, ref[i].x, ref[i].y, ref[i].z );
						}
						ImGui::EndChild();

						ImGui::TreePop();
					}

					if ( ImGui::TreeNode( "Indices" ) )
					{
						ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), childFrameSize );
						size_t end = mesh.indices.size();
						for ( size_t i = 0; i < end; ++i )
						{
							ImGui::Text( "[No:%d][%d]", i, mesh.indices[i] );
						}
						ImGui::EndChild();

						ImGui::TreePop();
					}

					if ( ImGui::TreeNode( "TexCoords" ) )
					{
						auto &ref = mesh.texCoords;

						ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), childFrameSize );
						size_t end = ref.size();
						for ( size_t i = 0; i < end; ++i )
						{
							ImGui::Text( "[No:%d][X:%6.3f][Y:%6.3f]", i, ref[i].x, ref[i].y );
					
						}
						ImGui::EndChild();

						ImGui::TreePop();
					}

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( "Materials" ) )
				{
					size_t subsetCount = mesh.subsets.size();
					for ( size_t j = 0; j < subsetCount; ++j )
					{
						const auto &subset = mesh.subsets[j];
						std::string subsetCaption = "Subset[" + std::to_string( j ) + "]";
						if ( ImGui::TreeNode( subsetCaption.c_str() ) )
						{
							auto ShowMaterialContain =
							[this]( const Loader::Material &mtl )
							{
								ImGui::Text
								(
									"Color:[X:%05.3f][Y:%05.3f][Z:%05.3f][W:%05.3f]",
									mtl.color.x, mtl.color.y, mtl.color.z, mtl.color.w
								);

								size_t texCount = mtl.relativeTexturePaths.size();
								if ( !texCount )
								{
									ImGui::Text( "This material don't have texture." );
									return;
								}
								// else
								for ( size_t i = 0; i < texCount; ++i )
								{
									auto &relativeTexturePath = mtl.relativeTexturePaths[i];

									if ( !Donya::IsExistFile( fileDirectory + relativeTexturePath ) )
									{
										ImGui::Text( "!This texture was not found![%s]", relativeTexturePath.c_str() );
										continue;
									}
									// else

									ImGui::Text
									(
										"Texture No.%d:[%s]",
										i, relativeTexturePath.c_str()
									);
								}
							};

							if ( ImGui::TreeNode( "Ambient" ) )
							{
								ShowMaterialContain( subset.ambient );

								ImGui::TreePop();
							}

							if ( ImGui::TreeNode( "Bump" ) )
							{
								ShowMaterialContain( subset.bump );

								ImGui::TreePop();
							}

							if ( ImGui::TreeNode( "Diffuse" ) )
							{
								ShowMaterialContain( subset.diffuse );

								ImGui::TreePop();
							}

							if ( ImGui::TreeNode( "Emissive" ) )
							{
								ShowMaterialContain( subset.emissive );

								ImGui::TreePop();
							}

							if ( ImGui::TreeNode( "Specular" ) )
							{
								ShowMaterialContain( subset.specular );

								ImGui::TreePop();
							}

							ImGui::Text( "Transparency:[%06.3f]", subset.transparency );

							ImGui::Text( "Reflection:[%06.3f]", subset.reflection );

							ImGui::TreePop();
						}
					} // subsets loop.

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( "Bone" ) )
				{
					if ( ImGui::TreeNode( "Influences" ) )
					{
						ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), childFrameSize );
						size_t boneInfluencesCount = mesh.influences.size();
						for ( size_t v = 0; v < boneInfluencesCount; ++v )
						{
							ImGui::Text( "Vertex No[%d]", v );

							auto &data = mesh.influences[v].cluster;
							size_t containCount = data.size();
							for ( size_t c = 0; c < containCount; ++c )
							{
								ImGui::Text
								(
									"\t[Index:%d][Weight[%6.4f]",
									data[c].index,
									data[c].weight
								);
							}
						}
						ImGui::EndChild();

						ImGui::TreePop();
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
		} // meshes loop.
	}
#endif // USE_IMGUI
}