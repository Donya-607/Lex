#include "Loader.h"

#include <fbxsdk.h>
// #include <functional>
#include <crtdbg.h>
#include <Windows.h>
#include <memory>

#include "Useful.h"

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

	bool Loader::Load( const std::string &filePath, std::string *outputErrorString )
	{
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

			const FBX::FbxVector4 *pControlPointsArray = pMesh->GetControlPoints();
			const int polygonsCount = pMesh->GetPolygonCount();
			for ( int polyIndex = 0; polyIndex < polygonsCount; ++polyIndex )
			{
				FBX::FbxVector4	fbxNormal;
				Donya::Vector3	normal;
				Donya::Vector3	position;
				for ( size_t v = 0; v < 3; ++v )
				{
					pMesh->GetPolygonVertexNormal( polyIndex, v, fbxNormal );
					normal.x	= scast<float>( fbxNormal[0] );
					normal.y	= scast<float>( fbxNormal[1] );
					normal.z	= scast<float>( fbxNormal[2] );

					const int ctrlPointIndex = pMesh->GetPolygonVertex( polyIndex, v );
					position.x	= scast<float>( pControlPointsArray[ctrlPointIndex][0] );
					position.y	= scast<float>( pControlPointsArray[ctrlPointIndex][1] );
					position.z	= scast<float>( pControlPointsArray[ctrlPointIndex][2] );

					indices.push_back( vertexCount++ );
					normals.push_back( normal );
					positions.push_back( position );
				}
			}
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

	#if USE_IMGUI
	void Loader::EnumPreservingDataToImGui( const char *ImGuiWindowIdentifier ) const
	{
		if ( ImGui::TreeNode( "Positions" ) )
		{
			ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), ImVec2( ImGui::GetWindowWidth(), ImGui::GetWindowHeight() ) );
			size_t end = positions.size();
			for ( size_t i = 0; i < end; ++i )
			{
				ImGui::Text ( "[No:%d][X:%6.3f][Y:%6.3f][Z:%6.3f]", i, positions[i].x, positions[i].y, positions[i].z );
			}
			ImGui::EndChild();

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( "Normals" ) )
		{
			ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), ImVec2( ImGui::GetWindowWidth(), ImGui::GetWindowHeight() ) );
			size_t end = normals.size();
			for ( size_t i = 0; i < end; ++i )
			{
				ImGui::Text( "[No:%d][X:%6.3f][Y:%6.3f][Z:%6.3f]", i, normals[i].x, normals[i].y, normals[i].z );
			}
			ImGui::EndChild();

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( "Indices" ) )
		{
			ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), ImVec2( ImGui::GetWindowWidth(), ImGui::GetWindowHeight() ) );
			size_t end = indices.size();
			for ( size_t i = 0; i < end; ++i )
			{
				ImGui::Text( "[No:%d][%d]", i, indices[i] );
			}
			ImGui::EndChild();

			ImGui::TreePop();
		}
	}
	#endif // USE_IMGUI
}