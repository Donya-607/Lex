#include "Loader.h"

#include <fbxsdk.h>
#include <crtdbg.h>
#include <Shlwapi.h>
#include <Windows.h>
#include <memory>

#include "Benchmark.h"
#include "Useful.h"

#pragma comment( lib, "shlwapi.lib" )

#define scast static_cast

namespace FBX = fbxsdk;

namespace Donya
{
	Loader::Loader() :
		vertexCount( 0 ),
		fileName(),
		indices(),
		normals(),
		positions()
	{

	}
	Loader::~Loader()
	{
		std::vector<size_t>().swap( indices );
		std::vector<Donya::Vector3>().swap( positions );
		std::vector<Donya::Vector3>().swap( normals );
	}

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

	void Traverse( FBX::FbxNode *pNode, std::vector<FBX::FbxNode *> *pFetchedMeshes )
	{
		if ( !pNode ) { return; }
		// else

		FBX::FbxNodeAttribute *pNodeAttr = pNode->GetNodeAttribute();
		if ( pNodeAttr )
		{
			switch ( pNodeAttr->GetAttributeType() )
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

		for ( int i = 0; i < pNode->GetChildCount(); ++i )
		{
			Traverse( pNode->GetChild( i ), pFetchedMeshes );
		}
	}

	std::string AcquireDirectoryFromFullPath( std::string fullPath )
	{
		size_t pathLength = fullPath.size();
		std::unique_ptr<char[]> directory = std::make_unique<char[]>( pathLength );
		for ( size_t i = 0; i < pathLength; ++i )
		{
			directory[i] = fullPath[i];
		}

		PathRemoveFileSpecA( directory.get() );
		PathAddBackslashA( directory.get() );

		return std::string{ directory.get() };
	}

	bool Loader::Load( const std::string &filePath, std::string *outputErrorString )
	{
		fileDirectory = filePath;
		fileDirectory = AcquireDirectoryFromFullPath( fileDirectory );

		MakeFileName( filePath );

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
			if ( !pImporter->Initialize( fileName.c_str(), -1, pManager->GetIOSettings() ) )
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

		FBX::FbxGeometryConverter geometryConverter( pManager );
		geometryConverter.Triangulate( pScene, /* replace = */ true );

		std::vector<FBX::FbxNode *> fetchedMeshes{};
		Traverse( pScene->GetRootNode(), &fetchedMeshes );

		size_t end = 1; // fetchedMeshes.size();
		for ( size_t i = 0; i < end; ++i )
		{
			FBX::FbxMesh *pMesh = fetchedMeshes[i]->GetMesh();

			FetchMaterial( pMesh );
			FetchVertices( pMesh );
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
	void Loader::MakeFileName( const std::string &filePath )
	{
		constexpr size_t FILE_PATH_LENGTH = 512U;

		fileName = GetUTF8FullPath( filePath, FILE_PATH_LENGTH );
	}

	void Loader::FetchVertices( const FBX::FbxMesh *pMesh )
	{
		const FBX::FbxVector4 *pControlPointsArray = pMesh->GetControlPoints();
		const int mtlCount = pMesh->GetNode()->GetMaterialCount();
		const int polygonCount = pMesh->GetPolygonCount();

		// Calculate subsets start index(not optimized).
		{
			// Count the faces each material.
			for ( int i = 0; i < polygonCount; ++i )
			{
				int mtlIndex = pMesh->GetElementMaterial()->GetIndexArray().GetAt( i );
				subsets[mtlIndex].indexCount += 3;
			}

			// Record the offset (how many vertex)
			int offset = 0;
			for ( auto &subset : subsets )
			{
				subset.indexStart = offset;
				offset += subset.indexCount;
				// This will be used as counter in the following procedures, reset to zero.
				subset.indexCount = 0;
			}

			// indices.resize( offset );
		}

		indices.resize( polygonCount * 3 );
		for ( int polyIndex = 0; polyIndex < polygonCount; ++polyIndex )
		{
			// The material for current face.
			int mtlIndex = 0;
			if ( mtlCount )
			{
				mtlIndex = pMesh->GetElementMaterial()->GetIndexArray().GetAt( polyIndex );
			}

			// Where should I save the vertex attribute index, according to the material.
			auto &subset = subsets[mtlIndex];
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

				normals.push_back( normal );
				positions.push_back( position );

				// indices.push_back( vertexCount++ );
				indices[indexOffset + v] = vertexCount++;
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
			texCoords.push_back( Donya::Vector2{ x, y } );
		}
	}

	void Loader::FetchMaterial( const FBX::FbxMesh *pMesh )
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

			AnalyseProperty( pMaterial );
		}
	}

	void Loader::AnalyseProperty( FBX::FbxSurfaceMaterial *pMaterial )
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

			if ( prop.IsValid() )
			{
				int layerCount = prop.GetSrcObjectCount<FBX::FbxLayeredTexture>();
				if ( !layerCount )
				{
					int textureCount = prop.GetSrcObjectCount<FBX::FbxFileTexture>();
					for ( int i = 0; i < textureCount; ++i )
					{
						FBX::FbxFileTexture *texture = prop.GetSrcObject<FBX::FbxFileTexture>( i );
						if ( texture )
						{
							std::string relativePath = texture->GetRelativeFileName();
							pOutMtl->textureNames.push_back( fileDirectory + relativePath );
						}
					}
				}

				if ( factor.IsValid() )
				{
					AssignFbxDouble4Process( &( pOutMtl->color ) );
				}
			}
		};
		{

			Loader::Subset subset{};
			{

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
					subset.reflection   = 0.0f;
					subset.transparency = 0.0f;
					subset.specular.color = Donya::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f };
				}
			}

			subsets.push_back( subset );
		}

		// XXX:Ç±ÇÃä÷êîÇ©ÇÁèoÇΩç€Ç…ÅCó·äOÇ™ìäÇ∞ÇÁÇÍÇÈ
	}

	#if USE_IMGUI
	void Loader::EnumPreservingDataToImGui( const char *ImGuiWindowIdentifier ) const
	{
		ImVec2 childFrameSize( 512.0f, 256.0f );

		if ( ImGui::TreeNode( "Positions" ) )
		{
			auto &ref = positions;

			ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), childFrameSize );
			size_t end = ref.size();
			for ( size_t i = 0; i < end; ++i )
			{
				ImGui::Text ( "[No:%d][X:%6.3f][Y:%6.3f][Z:%6.3f]", i, ref[i].x, ref[i].y, ref[i].z );
			}
			ImGui::EndChild();

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( "Normals" ) )
		{
			auto &ref = normals;

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
			size_t end = indices.size();
			for ( size_t i = 0; i < end; ++i )
			{
				ImGui::Text( "[No:%d][%d]", i, indices[i] );
			}
			ImGui::EndChild();

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( "TexCoords" ) )
		{
			auto &ref = texCoords;

			ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), childFrameSize );
			size_t end = ref.size();
			for ( size_t i = 0; i < end; ++i )
			{
				ImGui::Text( "[No:%d][X:%6.3f][Y:%6.3f]", i, ref[i].x, ref[i].y );
			}
			ImGui::EndChild();

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( "Materials" ) )
		{
			size_t subsetEnd = subsets.size();
			for ( size_t i = 0; i < subsetEnd; ++i )
			{
				auto &subset = subsets[i];
				std::string caption = "Subsets[" + std::to_string( i ) + "]";
				if ( ImGui::TreeNode( caption.c_str() ) )
				{
					auto ShowMaterialContain =
					[]( const Loader::Material &mtl )
					{
						ImGui::Text
						(
							"Color:[X:%5.3f][Y:%5.3f][Z:%5.3f][W:%5.3]",
							mtl.color.x, mtl.color.y, mtl.color.z, mtl.color.w
						);

						size_t end = mtl.textureNames.size();
						for ( size_t i = 0; i < end; ++i )
						{
							ImGui::Text
							(
								"Texture No.%d:[%s]",
								i, mtl.textureNames[i].c_str()
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

					ImGui::Text( "Transparency:[%6.3f]", subset.transparency );

					ImGui::Text( "Reflectivity:[%6.3f]", subset.reflection );

					ImGui::TreePop();
				}
			}
			
			ImGui::TreePop();
		}
	}
	#endif // USE_IMGUI
}