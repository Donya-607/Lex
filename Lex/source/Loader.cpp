#include "Loader.h"

#include <crtdbg.h>
#include <Windows.h>

#if USE_FBX_SDK
#include <fbxsdk.h>
#endif // USE_FBX_SDK

#include "Donya/Constant.h"	// Use scast macro.
#include "Donya/Useful.h"	// Use OutputDebugStr().

#undef min
#undef max

#if USE_FBX_SDK
namespace FBX = fbxsdk;
#endif // USE_FBX_SDK

void OutputDebugProgress( const std::string &str, bool isAllowOutput )
{
	if ( !isAllowOutput ) { return; }
	// else

	const std::string prefix { "[Donya.LoadProgress]:" };
	const std::string postfix{ "\n" };
	Donya::OutputDebugStr( ( prefix + str + postfix ).c_str() );
}

namespace Donya
{
	Loader::Loader() :
		absFilePath(), fileName(), fileDirectory(),
		meshes(), motions(), collisionFaces(),
		sampleFPS( 0.0f )
	{}
	Loader::~Loader()
	{
		meshes.clear();
		motions.clear();
		collisionFaces.clear();
		meshes.shrink_to_fit();
		motions.shrink_to_fit();
		collisionFaces.shrink_to_fit();
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
	DirectX::XMFLOAT4X4 Convert( const FBX::FbxAMatrix &affineMatrix )
	{
		DirectX::XMFLOAT4X4 out{};
		for ( int r = 0; r < 4; ++r )
		{
			for ( int c = 0; c < 4; ++c )
			{
				out.m[r][c] = scast<float>( affineMatrix[r][c] );
			}
		}
		return out;
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
				data.push_back( { clustersIndex, weight } );
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
	void FetchBoneMatrices( FBX::FbxTime time, const FBX::FbxMesh *pMesh, Loader::Skeletal *pSkeletal )
	{
		auto FetchMatricesFromSkin = []( FBX::FbxTime time, const FBX::FbxMesh *pMesh, const FBX::FbxSkin *pSkin, Loader::Skeletal *pSkeletal )
		{
			const int clusterCount = pSkin->GetClusterCount();
			
		#if 0 // APPEND // If have a probability to run more than once.
			const size_t oldSkeletalCount = pSkeletal->size();
			pSkeletal->resize( oldSkeletalCount + scast<size_t>( clusterCount ) );
		#else
			pSkeletal->skeletal.resize( scast<size_t>( clusterCount ) );
		#endif // APPEND
		
			pSkeletal->boneCount = pSkeletal->skeletal.size();

			for ( int i = 0; i < clusterCount; ++i )
			{
				Loader::Bone &bone = pSkeletal->skeletal.at( i );

				// Each joint.
				const FbxCluster *pCluster = pSkin->GetCluster( i );

				bone.name = pCluster->GetName();

				FBX::FbxAMatrix offsetMatrix{};
				{
					// This matrix transforms coordinates of the initial pose from mesh space to global space.
					FBX::FbxAMatrix referenceGlobalInitPosition{};
					pCluster->GetTransformMatrix( referenceGlobalInitPosition );

					// This matrix transforms coordinates of the initial pose from bone space to global space.
					FBX::FbxAMatrix clusterGlobalInitPosition{};
					pCluster->GetTransformLinkMatrix( clusterGlobalInitPosition );

					offsetMatrix = clusterGlobalInitPosition.Inverse() * referenceGlobalInitPosition;
				}

				FBX::FbxAMatrix poseMatrix{};
				{
					// This matrix transforms coordinates of the current pose from mesh space to global space.
					FBX::FbxAMatrix referenceGlobalCurrentPosition{};
					referenceGlobalCurrentPosition = pMesh->GetNode()->EvaluateGlobalTransform( time );

					// This matrix transforms coordinates of the current pose from bone space to global space.
					FBX::FbxAMatrix clusterGlobalCurrentPosition{};
					FBX::FbxNode *pLinkNode = const_cast<FBX::FbxNode *>( pCluster->GetLink() );
					clusterGlobalCurrentPosition = pLinkNode->EvaluateGlobalTransform( time );

					poseMatrix = referenceGlobalCurrentPosition.Inverse() * clusterGlobalCurrentPosition;
				}

				// "initial : mesh->global->bone", "current : bone->global->mesh"
				// First, transform from initial mesh space to initial bone space.
				// Then,  transfrom from initial bone space to current mesh space(i.e.animated position).
				// FbxAMatrix is column-major, so multiply from right side.

				bone.transform = Convert( poseMatrix * offsetMatrix );
				// bone.transformToBone = Convert( poseMatrix.Inverse() );
			}
		};

		const int deformersCount = pMesh->GetDeformerCount( FBX::FbxDeformer::eSkin );
		for ( int i = 0; i < deformersCount; ++i )
		{
			FBX::FbxSkin *pSkin = scast<FBX::FbxSkin *>( pMesh->GetDeformer( i, FBX::FbxDeformer::eSkin ) );

			FetchMatricesFromSkin( time, pMesh, pSkin, pSkeletal );
		}
	}
	/// <summary>
	/// If specified mesh("pMesh") has not a motion(animation), returns false.
	/// "samplingRate" is sampling times per second. if "samplingRate" zero, use FBX data's frame rate.
	/// </summary>
	bool FetchMotion( float samplingFPS, const FBX::FbxMesh *pMesh, Loader::Motion *pMotion )
	{
		// List of all the animation stack. 
		FBX::FbxArray<FBX::FbxString *> animationStackNames;
		pMesh->GetScene()->FillAnimStackNameArray( animationStackNames );
		const int animationStackCount = animationStackNames.Size();

		auto ReleaseAnimationStackNames = [&animationStackNames, &animationStackCount]()->void
		{
			for ( int i = 0; i < animationStackCount; i++ )
			{
				delete animationStackNames[i];
			}
		};

		if ( animationStackCount <= 0 )
		{
			// FBX::DeleteArray
			ReleaseAnimationStackNames();
			return false;
		}
		// else

		// Get the FbxTime per animation's frame. 
		const FBX::FbxTime::EMode timeMode = pMesh->GetScene()->GetGlobalSettings().GetTimeMode();
		FBX::FbxTime frameTime{};
		frameTime.SetTime( 0, 0, 0, 1, 0, timeMode );

		pMotion->samplingRate = ( samplingFPS <= 0.0f )
		? 1.0f / scast<float>( FBX::FbxTime::GetFrameRate( timeMode ) /* Contain FPS */ )
		: 1.0f / samplingFPS;

		FBX::FbxTime samplingStep;
		samplingStep.SetTime( 0, 0, 1, 0, 0, timeMode );
		samplingStep = scast<FBX::FbxLongLong>( scast<double>( samplingStep.Get() ) * pMotion->samplingRate );

		for ( int i = 0; i < animationStackCount; ++i )
		{
			FBX::FbxString		*pAnimStackName			= animationStackNames.GetAt( i );
			FBX::FbxAnimStack	*pCurrentAnimationStack	= pMesh->GetScene()->FindMember<FBX::FbxAnimStack>( pAnimStackName->Buffer() );
			pMesh->GetScene()->SetCurrentAnimationStack( pCurrentAnimationStack );

			FBX::FbxTakeInfo	*pTakeInfo = pMesh->GetScene()->GetTakeInfo( pAnimStackName->Buffer() );
			if ( !pTakeInfo )	{ continue; }
			// else

			pMotion->names.emplace_back( pAnimStackName->Buffer() );

			const FBX::FbxTime beginTime	= pTakeInfo->mLocalTimeSpan.GetStart();
			const FBX::FbxTime endTime		= pTakeInfo->mLocalTimeSpan.GetStop();
			for ( FBX::FbxTime currentTime	= beginTime; currentTime < endTime; currentTime += samplingStep )
			{
				Loader::Skeletal currentPosture{};
				FetchBoneMatrices( currentTime, pMesh, &currentPosture );

				pMotion->motion.emplace_back( currentPosture );
			}
		}
		
		ReleaseAnimationStackNames();

		return true;
	}

#endif // USE_FBX_SDK

#if USE_FBX_SDK

	void Loader::SetSamplingFPS( float FPS )
	{
		sampleFPS = FPS;
	}

#endif // USE_FBX_SDK

	bool Loader::Load( const std::string &filePath, std::string *outputErrorString, bool outputProgress )
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
			OutputDebugProgress( std::string{ "Start By FBX:" + filePath }, outputProgress );

			bool succeeded = LoadByFBXSDK( fullPath, outputErrorString, outputProgress );

			const std::string resultString = ( succeeded ) ? "Load By FBX Successful:" : "Load By FBX Failed:";
			OutputDebugProgress( resultString + filePath, outputProgress );

			return succeeded;
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
			OutputDebugProgress( std::string{ "Start By Cereal:" + filePath }, outputProgress );

			bool succeeded = LoadByCereal( fullPath, outputErrorString, outputProgress );

			const std::string resultString = ( succeeded ) ? "Load By Cereal Successful:" : "Load By Cereal Failed:";
			OutputDebugProgress( resultString + filePath, outputProgress );

			return succeeded;
		}

		return false;
	}

	void Loader::SaveByCereal( const std::string &filePath ) const
	{
		Donya::Serializer::Extension bin  = Donya::Serializer::Extension::BINARY;

		std::lock_guard<std::mutex> lock( cerealMutex );
		
		Donya::Serializer seria;
		seria.Save( bin, filePath.c_str(),  SERIAL_ID, *this );
	}
	
	bool Loader::LoadByCereal( const std::string &filePath, std::string *outputErrorString, bool outputProgress )
	{
		Donya::Serializer::Extension ext = Donya::Serializer::Extension::BINARY;

		std::lock_guard<std::mutex> lock( cerealMutex );
		
		Donya::Serializer seria;
		bool succeeded = seria.Load( ext, filePath.c_str(), SERIAL_ID, *this );

		// I should overwrite file-directory after load, because this will overwritten by Serializer::Load().
		fileDirectory = ExtractFileDirectoryFromFullPath( filePath );

		return succeeded;
	}

#if USE_FBX_SDK

#define USE_TRIANGULATE ( true )

	bool Loader::LoadByFBXSDK( const std::string &filePath, std::string *outputErrorString, bool outputProgress )
	{
		fileDirectory	= ExtractFileDirectoryFromFullPath( filePath );
		fileName		= filePath.substr( fileDirectory.size() ); // 0x7598C632 で例外がスローされました (Lex.exe 内): Microsoft C++ の例外: std::out_of_range (メモリの場所 0x0EA4E994)。
		// TODO : Fix this exception that occurred here.

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

		// Import.
		{
			OutputDebugProgress( "Start Import.", outputProgress );

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

			OutputDebugProgress( "Finish Import.", outputProgress );
		}

		pLock.reset( nullptr );

	#if USE_TRIANGULATE
		{
			OutputDebugProgress( "Start Triangulate", outputProgress );

			FBX::FbxGeometryConverter geometryConverter( pManager );
			bool replace = true;
			geometryConverter.Triangulate( pScene, replace );

			OutputDebugProgress( "Finish Triangulate", outputProgress );
		}
	#endif

		std::vector<FBX::FbxNode *> fetchedMeshes{};
		Traverse( pScene->GetRootNode(), &fetchedMeshes );

		std::vector<BoneInfluencesPerControlPoint> influencesPerCtrlPoints{};

		size_t meshCount = fetchedMeshes.size();
		OutputDebugProgress( "Start Meshes load. Meshes count:[" + std::to_string( meshCount ) + "]", outputProgress );

		meshes.resize( meshCount );
		for ( size_t i = 0; i < meshCount; ++i )
		{
			meshes[i].meshNo = scast<int>( i );
			
			FBX::FbxMesh *pMesh = fetchedMeshes[i]->GetMesh();

			influencesPerCtrlPoints.clear();
			FetchBoneInfluences( pMesh, influencesPerCtrlPoints );

			FetchVertices( i, pMesh, influencesPerCtrlPoints );
			FetchMaterial( i, pMesh );
			FetchGlobalTransform( i, pMesh );

			// Convert right-hand space to left-hand space.
			{
				meshes[i].coordinateConversion._11 = -1.0f;
			}

			OutputDebugProgress( "Finish Mesh[" + std::to_string( i ) + "].Polygons.", outputProgress );

			// Fetch the motion.
			{
				// The motion is fetch from FBX's mesh.
				// but I think the mesh is not necessarily link to motion,
				// so separate the motion from mesh, then give mesh's identifier("meshNo") to motion.

				motions.push_back( {} );
				Loader::Motion &currentMotion = motions.back();
				bool hasMotion = FetchMotion( sampleFPS, pMesh, &currentMotion );
				if ( hasMotion )
				{
					currentMotion.meshNo = scast<int>( i );
				}
				else
				{
					// "currentMotion" is disabled here.
					motions.pop_back();
				}
			}

			OutputDebugProgress( "Finish Mesh[" + std::to_string( i ) + "].Motion.", outputProgress );
		}

		OutputDebugProgress( "Finish Meshes load.", outputProgress );

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
			int  mtlIndex = 0;
			if ( mtlCount )
			{
				mtlIndex = pMesh->GetElementMaterial()->GetIndexArray().GetAt( polyIndex );
			}

			// Where should I save the vertex attribute index, according to the material.
			auto &subset	= mesh.subsets[mtlIndex];
			int indexOffset	= subset.indexStart + subset.indexCount;

			FBX::FbxVector4	fbxNormal;
			Donya::Vector3	normal;
			Donya::Vector3	position;

			Face colFace{};
			colFace.materialIndex = mtlIndex;

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

				if ( v < colFace.points.size() )
				{
					colFace.points[v] = position;
				}

				mesh.indices[indexOffset + v] = vertexCount;
				vertexCount++;

				mesh.influences.push_back( fetchedInfluences[ctrlPointIndex] );
			}
			subset.indexCount += size;

			collisionFaces.emplace_back( colFace );
		}

		FBX::FbxStringList uvName;
		pMesh->GetUVSetNames( uvName );

		FBX::FbxArray<FBX::FbxVector2> uvs{};
		pMesh->GetPolygonVertexUVs( uvName.GetStringAt( 0 ), uvs );
		for ( int i = 0; i < uvs.GetCount(); ++i )
		{
			float x = scast<float>( uvs[i].mData[0] );
			float y = 1.0f - scast<float>( uvs[i].mData[1] ); // For DirectX's uv space(the origin is left-top).
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
			subset.specular.color	= Donya::Vector4{ 0.0f, 0.0f, 0.0f, 1.0f };
		}
	}

	void Loader::FetchGlobalTransform( size_t meshIndex, const fbxsdk::FbxMesh *pMesh )
	{
		FBX::FbxAMatrix globalTransform = pMesh->GetNode()->EvaluateGlobalTransform( 0 );
		meshes[meshIndex].globalTransform = Convert( globalTransform );
	}

#endif // USE_FBX_SDK

#if USE_IMGUI
	void Loader::AdjustParameterByImGuiNode()
	{
		if ( ImGui::TreeNode( u8"パラメータの変更" ) )
		{
			ImGui::Text( u8"ここで変更したパラメータは保存できますが，即時反映はされません" );

			auto ShowFloat4x4 = []( const std::string &caption, Donya::Vector4x4 *p4x4 )
			{
				if ( ImGui::TreeNode( caption.c_str() ) )
				{
					ImGui::SliderFloat4( "11, 12, 13, 14", &p4x4->_11, -1.0f, 1.0f );
					ImGui::SliderFloat4( "21, 22, 23, 24", &p4x4->_21, -1.0f, 1.0f );
					ImGui::SliderFloat4( "31, 32, 33, 34", &p4x4->_31, -1.0f, 1.0f );
					ImGui::SliderFloat4( "41, 42, 43, 44", &p4x4->_41, -1.0f, 1.0f );

					ImGui::TreePop();
				}
			};

			const size_t meshCount = meshes.size();
			for ( size_t i = 0; i < meshCount; ++i )
			{
				auto &mesh = meshes[i];
				const std::string meshCaption = "Mesh[" + std::to_string( i ) + "]";
				if ( ImGui::TreeNode( meshCaption.c_str() ) )
				{
					ShowFloat4x4( "CoordinateConversion", &mesh.coordinateConversion );

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}
	}
	void Loader::EnumPreservingDataToImGui() const
	{
		ImVec2 childFrameSize( 0.0f, 0.0f );

		const size_t meshCount = meshes.size();
		for ( size_t i = 0; i < meshCount; ++i )
		{
			const auto &mesh = meshes[i];
			const std::string meshCaption = "Mesh[" + std::to_string( i ) + "]";
			if ( ImGui::TreeNode( meshCaption.c_str() ) )
			{
				const size_t verticesCount = mesh.indices.size();
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
		}

		const size_t motionCount = motions.size();
		for ( size_t i = 0; i < motionCount; ++i )
		{
			const auto &motion = motions[i];
			const std::string motionCaption = "Motion[" + std::to_string( i ) + "]";
			if ( ImGui::TreeNode( motionCaption.c_str() ) )
			{
				ImGui::Text( "Mesh.No:%d", motion.meshNo );
				ImGui::Text( "Mesh.SamplingRate:%5.3f", motion.samplingRate );

				if ( ImGui::TreeNode( "Names" ) )
				{
					const size_t nameCount = motion.names.size();
					for ( size_t n = 0; n < nameCount; ++n )
					{
						ImGui::Text( motion.names[n].c_str() );
					}

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( "Skeletals" ) )
				{
					std::string  subscriptCaption{};
					std::string  skeletalCaption{};
					const size_t skeletalCount = motion.motion.size();
					for ( size_t s = 0; s < skeletalCount; ++s )
					{
						auto &skeletal   = motion.motion[s];
						subscriptCaption = "[" + std::to_string( s ) + "]";
						skeletalCaption  = ".BoneCount : " + std::to_string( skeletal.boneCount );
						if ( ImGui::TreeNode( ( subscriptCaption + skeletalCaption ).c_str() ) )
						{
							std::string  boneName{};
							for ( size_t b = 0; b < skeletal.boneCount; ++b )
							{
								const auto &bone = skeletal.skeletal[b];
								subscriptCaption = "[" + std::to_string( b ) + "]";
								boneName = ".Name:[" + bone.name + "]";
								ImGui::Text( ( subscriptCaption + boneName ).c_str() );

								constexpr const char *FLOAT4_FORMAT = "%+05.2f, %+05.2f, %+05.2f, %+05.2f";
								ImGui::Text
								(
									FLOAT4_FORMAT,
									bone.transform._11, bone.transform._12, bone.transform._13, bone.transform._14
								);
								ImGui::Text
								(
									FLOAT4_FORMAT,
									bone.transform._21, bone.transform._22, bone.transform._23, bone.transform._24
								);
								ImGui::Text
								(
									FLOAT4_FORMAT,
									bone.transform._31, bone.transform._32, bone.transform._33, bone.transform._34
								);
								ImGui::Text
								(
									FLOAT4_FORMAT,
									bone.transform._41, bone.transform._42, bone.transform._43, bone.transform._44
								);
								ImGui::Text( "" );
							}

							ImGui::TreePop();
						}
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
		}
	}
#endif // USE_IMGUI
}