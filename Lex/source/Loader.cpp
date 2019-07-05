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
	Loader::Material::Material( const Loader::Material &ref )
	{
		// use implement of operator =.
		*this = ref;
	}
	Loader::Material &Loader::Material::operator = ( const Loader::Material &ref )
	{
		ambient			= ref.ambient;
		bump			= ref.bump;
		diffuse			= ref.diffuse;
		emissive		= ref.emissive;
		transparency	= ref.transparency;
		if ( ref.pPhong )
		{
			pPhong = std::move( std::make_unique<Loader::Material::Phong>( *ref.pPhong ) );
		}

		textureNames = ref.textureNames;

		return *this;
	}

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
		// TODO:スタックオーバーフローが発生する？

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

			FetchVertices( pMesh );
			FetchMaterial( pMesh );
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
		const int polygonsCount = pMesh->GetPolygonCount();
		for ( int polyIndex = 0; polyIndex < polygonsCount; ++polyIndex )
		{
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

				indices.push_back( vertexCount++ );
				normals.push_back( normal );
				positions.push_back( position );
			}
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

		Loader::Material mtl;

		/*
		materials.push_back( {} );
		auto &mtl = materials.back();
		*/

		FBX::FbxProperty prop{};
		FBX::FbxProperty factor{};

		auto AssignFbxDouble3 =
		[]( Donya::Vector3 *output, const FBX::FbxDouble3 *input, double factor )
		{
			output->x = scast<float>( input->mData[0] * factor );
			output->y = scast<float>( input->mData[1] * factor );
			output->z = scast<float>( input->mData[2] * factor );
		};
		auto AssignFbxDouble3Process =
		[&]( Donya::Vector3 *output )
		{
			auto entity = prop.Get<FBX::FbxDouble3>();
			double fact = factor.Get<FBX::FbxDouble>();
			AssignFbxDouble3
			(
				output,
				&entity,
				fact
			);
		};
		
		prop	= pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sAmbient );
		factor	= pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sAmbientFactor );
		if ( prop.IsValid() && factor.IsValid() )
		{
			AssignFbxDouble3Process( &mtl.ambient );
		}

		prop	= pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sBump );
		factor	= pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sBumpFactor );
		if ( prop.IsValid() && factor.IsValid() )
		{
			AssignFbxDouble3Process( &mtl.bump );
		}

		prop	= pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sDiffuse );
		factor	= pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sDiffuseFactor );
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
						mtl.textureNames.push_back( fileDirectory + relativePath );
					}
				}
			}

			if ( factor.IsValid() )
			{
				AssignFbxDouble3Process( &mtl.diffuse );
			}
		}

		prop	= pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sEmissive );
		factor	= pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sEmissiveFactor );
		if ( prop.IsValid() && factor.IsValid() )
		{
			AssignFbxDouble3Process( &mtl.emissive );
		}

		prop = pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sTransparencyFactor );
		if ( prop.IsValid() )
		{
			mtl.transparency = scast<float>( prop.Get<FBX::FbxFloat>() );
		}

		if ( mtlType == PHONG )
		{ 
			mtl.pPhong = std::make_unique<Material::Phong>();

			prop = pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sReflection );
			if ( prop.IsValid() )
			{
				mtl.pPhong->refrectivity = scast<float>( prop.Get<FBX::FbxFloat>() );
			}

			prop = pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sShininess );
			if ( prop.IsValid() )
			{
				mtl.pPhong->shininess = scast<float>( prop.Get<FBX::FbxFloat>() );
			}

			prop	= pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sSpecular );
			factor	= pMaterial->FindProperty( FBX::FbxSurfaceMaterial::sSpecularFactor );
			if ( prop.IsValid() && factor.IsValid() )
			{
				AssignFbxDouble3Process( &mtl.pPhong->specular );
			}
		}

		materials.push_back( mtl );
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
			ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), childFrameSize );
			size_t mtlEnd = materials.size();
			for ( size_t i = 0; i < mtlEnd; ++i )
			{
				auto &mtl = materials[i];
				std::string caption = "Material[" + std::to_string( i ) + "]";
				if ( ImGui::TreeNode( caption.c_str() ) )
				{
					size_t texSize = mtl.textureNames.size();
					for ( size_t i = 0; i < texSize; ++i )
					{
						ImGui::Text( "Texture Name No.%d:[%s]", i, mtl.textureNames[i].c_str() );
					}

					ImGui::Text( "Ambient:[X:%6.3f][Y:%6.3f][Z:%6.3f]", mtl.ambient.x, mtl.ambient.y, mtl.ambient.z );

					ImGui::Text( "Bump:[X:%6.3f][Y:%6.3f][Z:%6.3f]", mtl.bump.x, mtl.bump.y, mtl.bump.z );

					ImGui::Text( "Diffuse:[X:%6.3f][Y:%6.3f][Z:%6.3f]", mtl.diffuse.x, mtl.diffuse.y, mtl.diffuse.z );

					ImGui::Text( "Emissive:[X:%6.3f][Y:%6.3f][Z:%6.3f]", mtl.emissive.x, mtl.emissive.y, mtl.emissive.z );

					ImGui::Text( "Transparency:[%6.3f]", mtl.transparency );

					if ( mtl.pPhong != nullptr )
					{
						ImGui::Text( "Refrectivity:[%6.3f]", mtl.pPhong->refrectivity );

						ImGui::Text( "Shininess:[%6.3f]", mtl.pPhong->shininess );

						ImGui::Text( "Specular:[X:%6.3f][Y:%6.3f][Z:%6.3f]", mtl.pPhong->specular.x, mtl.pPhong->specular.y, mtl.pPhong->specular.z );
					}

					ImGui::TreePop();
				}
			}
			ImGui::EndChild();
			
			ImGui::TreePop();
		}
	}
	#endif // USE_IMGUI
}