#include "Loader.h"

#include <algorithm>		// Use std::sort.
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
		source(),
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
	Donya::Quaternion ToQuaternion( const FBX::FbxDouble4 &source )
	{
		return Donya::Quaternion
		{
			scast<float>( source.mData[0] ),
			scast<float>( source.mData[1] ),
			scast<float>( source.mData[2] ),
			scast<float>( source.mData[3] )
		};
	}

	Donya::Vector3 ToVec3( const Donya::Vector4 &v )
	{
		return Donya::Vector3{ v.x, v.y, v.z };
	};

	Model::Animation::Transform SeparateSRT( const FBX::FbxAMatrix &source )
	{
		Model::Animation::Transform output;
		output.scale		= ToVec3( Convert( source.GetS() ) );
		output.rotation		= ToQuaternion( source.GetQ() );
		output.translation	= ToVec3( Convert( source.GetT() ) );
		return output;
	}

	constexpr bool HasMesh( FBX::FbxNodeAttribute::EType attr )
	{
		constexpr FBX::FbxNodeAttribute::EType HAS_LIST[]
		{
			FBX::FbxNodeAttribute::EType::eMesh
		};
		for ( const auto &type : HAS_LIST )
		{
			if ( attr == type ) { return true; }
		}
		return false;
	}
	constexpr bool HasSkeletal( FBX::FbxNodeAttribute::EType attr )
	{
		constexpr FBX::FbxNodeAttribute::EType HAS_LIST[]
		{
			FBX::FbxNodeAttribute::EType::eUnknown,
			FBX::FbxNodeAttribute::EType::eNull,
			FBX::FbxNodeAttribute::EType::eMarker,
			FBX::FbxNodeAttribute::EType::eSkeleton,
			FBX::FbxNodeAttribute::EType::eMesh
		};
		for ( const auto &type : HAS_LIST )
		{
			if ( attr == type ) { return true; }
		}
		return false;
	}

	int  FindMaterialIndex( FBX::FbxScene *pScene, const FBX::FbxSurfaceMaterial *pSurfaceMaterial )
	{
		const int mtlCount = pScene->GetMaterialCount();
		for ( int i = 0; i < mtlCount; ++i )
		{
			if ( pScene->GetMaterial( i ) == pSurfaceMaterial )
			{
				return i;
			}
		}

		return -1;
	}
	int  FindBoneIndex( const std::vector<Model::Animation::Bone> &source, const std::string &keyName )
	{
		const size_t boneCount = source.size();
		for ( size_t i = 0; i < boneCount; ++i )
		{
			if ( source[i].name == keyName )
			{
				return scast<int>( i );
			}
		}

		return -1;
	}

	void Traverse( FBX::FbxNode *pNode, std::vector<FBX::FbxNode *> *pMeshNodes, std::vector<FBX::FbxNode *> *pSkeltalNodes )
	{
		if ( !pNode ) { return; }
		// else

		FBX::FbxNodeAttribute *pNodeAttr = pNode->GetNodeAttribute();
		FBX::FbxNodeAttribute::EType attrType = FBX::FbxNodeAttribute::EType::eUnknown;
		if ( pNodeAttr )
		{
			attrType = pNodeAttr->GetAttributeType();
		}

		if ( HasSkeletal( attrType ) )
		{
			pSkeltalNodes->emplace_back( pNode );
		}
		if ( HasMesh( attrType ) )
		{
			pMeshNodes->emplace_back( pNode );
		}

		int end = pNode->GetChildCount();
		for ( int i = 0; i < end; ++i )
		{
			Traverse( pNode->GetChild( i ), pMeshNodes, pSkeltalNodes );
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

				bone.name = pMesh->GetNode()->GetName();

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

	void AdjustCoordinate( Model::Source::Mesh *pMesh )
	{
		// Convert right-hand space to left-hand space.
		//pMesh->coordinateConversion._11 = -1.0f;
	}

	/// <summary>
	/// Fetch the inverse matrix of initial pose(like T-pose), that transforms: mesh space -> bone space.
	/// </summary>
	FBX::FbxAMatrix FetchBoneOffsetMatrix( const FBX::FbxCluster *pCluster )
	{
		// This matrix transforms coordinates of the initial pose from mesh space to global space.
		FBX::FbxAMatrix referenceGlobalInitPosition{};
		pCluster->GetTransformMatrix( referenceGlobalInitPosition );

		// This matrix transforms coordinates of the initial pose from bone space to global space.
		FBX::FbxAMatrix clusterGlobalInitPosition{};
		pCluster->GetTransformLinkMatrix( clusterGlobalInitPosition );

		return clusterGlobalInitPosition.Inverse() * referenceGlobalInitPosition;
	}

	/// <summary>
	/// Build a skeletal from skeletal nodes.
	/// </summary>
	void BuildSkeletal( std::vector<Model::Animation::Bone> *pSkeletal, const std::vector<FBX::FbxNode *> &skeletalNodes, const FBX::FbxTime &currentTime = FBX::FBXSDK_TIME_INFINITE )
	{
		std::vector<Model::Animation::Bone> &data = *pSkeletal;

		const size_t nodeCount = skeletalNodes.size();
		data.resize( nodeCount );
		for ( size_t i = 0; i < nodeCount; ++i )
		{
			Model::Animation::Bone &bone = data[i];
			FBX::FbxNode *pNode = skeletalNodes[i];

			FBX::FbxAMatrix localTransform{};
			localTransform = pNode->EvaluateLocalTransform( currentTime );

			bone.name		= pNode->GetName();
			bone.transform	= SeparateSRT( localTransform );

			FBX::FbxNode *pParent = pNode->GetParent();
			if ( pParent )
			{
				bone.parentName   = pParent->GetName();
			}
			else
			{
				bone.parentName   = "";
			}
		}

		// Assign the parent from built skeletal.
		for ( size_t i = 0; i < nodeCount; ++i )
		{
			Model::Animation::Bone &bone = data[i];
			FBX::FbxNode *pNode = skeletalNodes[i];

			if ( bone.parentName == "" )
			{
				bone.parentIndex = -1;
				bone.transformToParent = Model::Animation::Transform::Identity();
			}
			else
			{
				// This matrix transforms coordinates of the current pose from bone space to global space.
				FBX::FbxAMatrix boneToGlobal{};
				boneToGlobal = pNode->EvaluateGlobalTransform( currentTime );

				// This matrix transforms coordinates of the current pose from parent's bone space to global space.
				FBX::FbxAMatrix parentBoneToGlobal{};
				parentBoneToGlobal = pNode->GetParent()->EvaluateGlobalTransform( currentTime );

				// bone -> global * global -> parentBone
				FBX::FbxAMatrix boneToParent{};
				boneToParent = parentBoneToGlobal.Inverse() * boneToGlobal;

				bone.parentIndex		= FindBoneIndex( data, bone.parentName );
				bone.transformToParent	= SeparateSRT( boneToParent );
			}
		}
	}

	void BuildKeyFrame( Model::Animation::KeyFrame *pKeyFrame, const std::vector<FBX::FbxNode *> &skeletalNodes, const FBX::FbxTime &currentTime, float currentSeconds )
	{
		pKeyFrame->seconds = currentSeconds;
		pKeyFrame->keyPose.clear();

		BuildSkeletal( &pKeyFrame->keyPose, skeletalNodes, currentTime );
	}
	void BuildMotions( std::vector<Model::Animation::Motion> *pMotions, const std::vector<FBX::FbxNode *> &skeletalNodes, FBX::FbxScene *pScene, float samplingFPS )
	{
		// List of all the animation stack. 
		FBX::FbxArray<FBX::FbxString *> animationStackNames;
		pScene->FillAnimStackNameArray( animationStackNames );
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
			return;
		}
		// else

		// Get the FbxTime per animation's frame. 
		const FBX::FbxTime::EMode timeMode = pScene->GetGlobalSettings().GetTimeMode();
		FBX::FbxTime frameTime{};
		frameTime.SetTime( 0, 0, 0, 1, 0, timeMode );

		const float samplingRate = ( samplingFPS <= 0.0f ) ? scast<float>( FBX::FbxTime::GetFrameRate( timeMode ) /* Contain FPS */ ) : samplingFPS;
		const float samplingTime = 1.0f / samplingRate;

		// Sampling criteria is 60fps.
		FBX::FbxTime samplingStep;
		samplingStep.SetTime( 0, 0, 1, 0, 0, timeMode );
		samplingStep = scast<FBX::FbxLongLong>( samplingStep.Get() * samplingTime ); // Please ignore the C26451 warning. If you cast to something, the samplingStep will be increased from the non-cast version.

		for ( int i = 0; i < animationStackCount; ++i )
		{
			FBX::FbxString		*pAnimStackName			= animationStackNames.GetAt( i );
			FBX::FbxAnimStack	*pCurrentAnimationStack	= pScene->FindMember<FBX::FbxAnimStack>( pAnimStackName->Buffer() );
			pScene->SetCurrentAnimationStack( pCurrentAnimationStack );

			FBX::FbxTakeInfo	*pTakeInfo = pScene->GetTakeInfo( pAnimStackName->Buffer() );
			if ( !pTakeInfo )	{ continue; }
			// else

			const FBX::FbxTime beginTime	= pTakeInfo->mLocalTimeSpan.GetStart();
			const FBX::FbxTime endTime		= pTakeInfo->mLocalTimeSpan.GetStop();
			const int startFrame	= scast<int>( beginTime.Get() / samplingStep.Get() );
			const int endFrame		= scast<int>( endTime.Get()   / samplingStep.Get() );
			const int frameCount	= scast<int>( ( endTime.Get() - beginTime.Get() ) / samplingStep.Get() );

			Model::Animation::Motion motion{};
			motion.name				= std::string{ pAnimStackName->Buffer() };	
			motion.samplingRate		= samplingRate;
			motion.animSeconds		= samplingTime * frameCount;
			motion.keyFrames.resize(  scast<size_t>( frameCount + 1 ) );

			float  seconds	= 0.0f;
			size_t index	= 0;
			Donya::Vector4x4 currentGlobalTransform{};
			for (  FBX::FbxTime currentTime = beginTime; currentTime < endTime; currentTime += samplingStep )
			{
				BuildKeyFrame( &motion.keyFrames[index], skeletalNodes, currentTime, seconds );
				
				seconds	+= samplingTime;
				index	+= 1U;
			}

			pMotions->emplace_back( std::move( motion ) );
		}
		
		ReleaseAnimationStackNames();
	}

	void BuildSubsets( std::vector<Model::Source::Subset> *pSubsets, FBX::FbxMesh *pMesh, const std::string &fileDirectory )
	{
		FBX::FbxNode *pNode = pMesh->GetNode();

		const int mtlCount = pNode->GetMaterialCount();
		pSubsets->resize( ( !mtlCount ) ? 1 : mtlCount );

		auto FetchMaterial = [&fileDirectory]( Model::Source::Material *pMaterial, const char *strProperty, const char *strFactor, const FBX::FbxSurfaceMaterial *pSurfaceMaterial )
		{
			const FBX::FbxProperty property	= pSurfaceMaterial->FindProperty( strProperty	);
			const FBX::FbxProperty factor	= pSurfaceMaterial->FindProperty( strFactor		);

			if ( !property.IsValid() ) { return; }
			// else

			auto AssignColor	= [&property, &factor]( Model::Source::Material *pMaterial )
			{
				Donya::Vector3	color	= Convert( property.Get<FBX::FbxDouble3>() );
				float			bias	= scast<float>( factor.Get<FBX::FbxDouble>() );
				pMaterial->color = Donya::Vector4{ color * bias, 1.0f };
			};
			auto FetchTextures	= [&]()->void
			{
				int textureCount = property.GetSrcObjectCount<FBX::FbxFileTexture>();
				for ( int i = 0; i < textureCount; ++i )
				{
					FBX::FbxFileTexture *texture = property.GetSrcObject<FBX::FbxFileTexture>( i );
					if ( !texture ) { continue; }
					// else
				
					std::string relativePath = texture->GetRelativeFileName();
					if ( relativePath.empty() )
					{
						std::string fullPath = texture->GetFileName();
						if ( !fullPath.empty() )
						{
							relativePath = fullPath.substr( fileDirectory.size() );
							pMaterial->textureName = relativePath;
						}
					}
					else
					{
						pMaterial->textureName = relativePath;
					}

					// No support a multiple texture currently.
					break;
				}
			};
			
			FetchTextures();

			if ( factor.IsValid() )
			{
				AssignColor( pMaterial );
			}
		};

		enum class MaterialType
		{
			Nil = 0,
			Lambert,
			Phong
		};
		auto AnalyseMaterialType = []( const FBX::FbxSurfaceMaterial *pMaterial )
		{
			if ( pMaterial->GetClassId().Is( FBX::FbxSurfacePhong::ClassId ) )
			{
				return MaterialType::Phong;
			}
			if ( pMaterial->GetClassId().Is( FBX::FbxSurfaceLambert::ClassId ) )
			{
				return MaterialType::Lambert;
			}
			// else
			return MaterialType::Nil;
		};

		MaterialType mtlType = MaterialType::Nil;
		for ( int i = 0; i < mtlCount; ++i )
		{
			using FbxMtl = FBX::FbxSurfaceMaterial;
			const FbxMtl *pSurfaceMaterial = pNode->GetMaterial( i );

			auto &subset = pSubsets->at( i );
			subset.name  = pSurfaceMaterial->GetName();
			FetchMaterial( &subset.ambient,		FbxMtl::sAmbient,	FbxMtl::sAmbientFactor,		pSurfaceMaterial );
			FetchMaterial( &subset.bump,		FbxMtl::sBump,		FbxMtl::sBumpFactor,		pSurfaceMaterial );
			FetchMaterial( &subset.diffuse,		FbxMtl::sDiffuse,	FbxMtl::sDiffuseFactor,		pSurfaceMaterial );
			FetchMaterial( &subset.specular,	FbxMtl::sSpecular,	FbxMtl::sSpecularFactor,	pSurfaceMaterial );
			FetchMaterial( &subset.emissive,	FbxMtl::sEmissive,	FbxMtl::sEmissiveFactor,	pSurfaceMaterial );

			mtlType = AnalyseMaterialType( pSurfaceMaterial );
			if ( mtlType == MaterialType::Phong )
			{
				const FBX::FbxProperty shininess = pSurfaceMaterial->FindProperty( FbxMtl::sShininess );
				if ( shininess.IsValid() )
				{
					subset.specular.color.w = scast<float>( shininess.Get<FBX::FbxDouble>() );
				}
			}
			else
			{
				subset.specular.color = Donya::Vector4{ 0.0f, 0.0f, 0.0f, 1.0f };
			}
		}

		// Calculate subsets start index(not optimized).
		if ( mtlCount )
		{
			// Count the faces each material.
			const int polygonCount = pMesh->GetPolygonCount();
			for ( int i = 0; i < polygonCount; ++i )
			{
				const int mtlIndex = pMesh->GetElementMaterial()->GetIndexArray().GetAt( i );
				pSubsets->at( mtlIndex ).indexCount += 3;
			}

			// Record the offset (how many vertex)
			int offset = 0;
			for ( auto &subset : *pSubsets )
			{
				subset.indexStart = offset;
				offset += subset.indexCount;
				// This will be used as counter in the following procedures, reset to zero.
				subset.indexCount = 0;
			}
		}
	}
	void BuildMesh( Model::Source::Mesh *pMesh, FBX::FbxNode *pNode, FBX::FbxMesh *pFBXMesh, std::vector<Model::Polygon> *pPolygons, FBX::FbxScene *pScene, const std::string &fileDirectory, const std::vector<Model::Animation::Bone> &modelSkeletal, float animationSamplingFPS )
	{
		AdjustCoordinate( pMesh );

		constexpr	int EXPECT_POLYGON_SIZE	= 3;
		const		int mtlCount			= pNode->GetMaterialCount();
		const		int polygonCount		= pFBXMesh->GetPolygonCount();

		pMesh->indices.resize( polygonCount * EXPECT_POLYGON_SIZE );
		pMesh->name = pNode->GetName();

		// TODO : Should separate the calculation of an indexCount and indexStart from here.
		// TODO : Should separate the calculation of an indices array from here.
		BuildSubsets( &pMesh->subsets, pFBXMesh, fileDirectory );

		//BuildMotions( &pMesh->motions, pFBXMesh, pScene, animationSamplingFPS );

		struct BoneInfluence
		{
			using ElementType = std::pair<float, int>;
			std::vector<ElementType> data; // first:Weight, second:Index.
		public:
			void Append( float weight, int index )
			{
				data.emplace_back( std::make_pair( weight, index ) );
			}
		};
		std::vector<BoneInfluence> boneInfluences{};

		// Fetch skinning data and influences by bone.
		{
			const int controlPointCount = pFBXMesh->GetControlPointsCount();
			boneInfluences.resize( scast<size_t>( controlPointCount ) );

			const FBX::FbxAMatrix geometricTransform
			{
				pNode->GetGeometricTranslation	( FBX::FbxNode::eSourcePivot ),
				pNode->GetGeometricRotation		( FBX::FbxNode::eSourcePivot ),
				pNode->GetGeometricScaling		( FBX::FbxNode::eSourcePivot )
			};

			auto FetchInfluence		= [&boneInfluences]( const FBX::FbxCluster *pCluster, int clusterIndex )
			{
				const int		ctrlPointIndicesCount	= pCluster->GetControlPointIndicesCount();
				const int		*ctrlPointIndices		= pCluster->GetControlPointIndices();
				const double	*ctrlPointWeights		= pCluster->GetControlPointWeights();

				if ( !ctrlPointIndices || !ctrlPointWeights ) { return; }
				// else

				for ( int i = 0; i < ctrlPointIndicesCount; ++i )
				{
					BoneInfluence &data	= boneInfluences[ctrlPointIndices[i]];
					float weight = scast<float>( ctrlPointWeights[i] );
					int   index  = clusterIndex;
					data.Append( weight, index );
				}
			};
			auto FetchBoneOffset	= [&geometricTransform]( const FBX::FbxCluster *pCluster, std::vector<Model::Animation::Bone> *pBoneOffsets )
			{
				const FBX::FbxAMatrix  boneOffset = FetchBoneOffsetMatrix( pCluster );
				
				Model::Animation::Bone bone{};
				bone.name				= pCluster->GetLink()->GetName();
				bone.transform			= SeparateSRT( boneOffset * geometricTransform );
				bone.transformToParent	= Model::Animation::Transform::Identity(); // The bone offset matrix does not use this.
				pBoneOffsets->emplace_back( std::move( bone ) );
			};

			const int deformerCount = pFBXMesh->GetDeformerCount( FBX::FbxDeformer::eSkin );
			for ( int deformerIndex = 0; deformerIndex < deformerCount; ++deformerIndex )
			{
				FBX::FbxSkin *pSkin = scast<FBX::FbxSkin *>( pFBXMesh->GetDeformer( deformerIndex, FBX::FbxDeformer::eSkin ) );
				const int clusterCount = pSkin->GetClusterCount();

				// Fetching members.
				for ( int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex )
				{
					FBX::FbxCluster *pCluster = pSkin->GetCluster( clusterIndex );
					FetchInfluence ( pCluster, clusterIndex );
					FetchBoneOffset( pCluster, &pMesh->boneOffsets );
				}

				// Assign the bone indices. The searching range is whole skeletal, so we should assign after building the skeletal.
				for ( int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex )
				{
					const FBX::FbxCluster *pCluster = pSkin->GetCluster( clusterIndex );
					const int boneIndex =  FindBoneIndex( modelSkeletal, pCluster->GetLink()->GetName() );

					pMesh->boneIndices.emplace_back( boneIndex );
				}
			}

			pMesh->boneIndex = FindBoneIndex( modelSkeletal, pNode->GetName() );
		}

		// Fetch vertex data.
		{
			// Sort by descending-order.
			auto SortInfluences = []( BoneInfluence *pSource )
			{
				auto DescendingCompare = []( const BoneInfluence::ElementType &lhs, const BoneInfluence::ElementType &rhs )
				{
					// lhs.weight > rhs.weight.
					return ( lhs.first > rhs.first ) ? true : false;
				};

				std::sort( pSource->data.begin(), pSource->data.end(), DescendingCompare );
			};
			// Sort and Shrink bone-influences count to up to maxInfluenceCount.
			auto NormalizeBoneInfluence = [&SortInfluences]( BoneInfluence source, size_t maxInfluenceCount )
			{
				const size_t sourceCount = source.data.size();
				if ( sourceCount <= maxInfluenceCount )
				{
					SortInfluences( &source );
					return source;
				}
				// else

				/*
				0,	Prepare the buffer to default-value.
				1,	Sort with weight the influences by descending order.
				2,	Assign the higher data of influences to result as many as maxInfluenceCount.
				3,	Add the remaining influences data to highest weight bone.
				*/

				BoneInfluence result{};
				auto Assign = [&result]( size_t elementIndex, float weight, int index )
				{
					result.data[elementIndex].first  = weight;
					result.data[elementIndex].second = index;
				};

				// No.0, Default-value is all zero but the first weight is one.
				result.Append( 1.0f, 0 );
				for ( size_t i = 1; i < maxInfluenceCount; ++i )
				{
					result.Append( 0.0f, 0 );
				}

				// No.1
				SortInfluences( &source );

				// No.2
				size_t  loopIndex = 0;
				for ( ; loopIndex < maxInfluenceCount; ++loopIndex )
				{
					if ( maxInfluenceCount <= loopIndex ) { continue; }
					// else

					Assign( loopIndex, source.data[loopIndex].first, source.data[loopIndex].second );
				}

				// No.3
				size_t highestBoneIndex{};
				{
					float highestWeight = 0.0f;
					for ( size_t i = 0; i < maxInfluenceCount; ++i )
					{
						float selectWeight = result.data[i].first;
						if ( highestWeight < selectWeight )
						{
							highestBoneIndex = i;
							highestWeight = selectWeight;
						}
					}
				}
				for ( ; loopIndex < sourceCount; ++loopIndex )
				{
					result.data[highestBoneIndex].first += source.data[loopIndex].first;
				}

				return result;
			};

			const FBX::FbxVector4 *pControlPointsArray	= pFBXMesh->GetControlPoints();
			const Donya::Vector4x4 globalTransform		= Convert( pNode->EvaluateGlobalTransform( 0 ) );
			const Donya::Vector4x4 adjustMatrix			= globalTransform * pMesh->coordinateConversion;

			FBX::FbxStringList uvSetName;
			pFBXMesh->GetUVSetNames( uvSetName );

			size_t vertexCount = 0;
			for ( int polyIndex = 0; polyIndex < polygonCount; ++polyIndex )
			{
				// The material for current face.
				int  mtlIndex = 0;
				if ( mtlCount )
				{
					mtlIndex = pFBXMesh->GetElementMaterial()->GetIndexArray().GetAt( polyIndex );
				}

				// Where should I save the vertex attribute index, according to the material.
				auto &subset	= pMesh->subsets[mtlIndex];
				int indexOffset	= scast<int>( subset.indexStart + subset.indexCount );

				FBX::FbxVector4		fbxNormal{};
				Model::Vertex::Pos	pos{};
				Model::Vertex::Tex	tex{};
				Model::Vertex::Bone	infl{};

				Model::Polygon		polygon{};
				polygon.materialIndex = mtlIndex;

				const int polygonSize = pFBXMesh->GetPolygonSize( polyIndex );
				_ASSERT_EXPR( polygonSize == EXPECT_POLYGON_SIZE, L"Error : A mesh did not triangulated!" );

				for ( int v = 0; v < EXPECT_POLYGON_SIZE; ++v )
				{
					const int ctrlPointIndex = pFBXMesh->GetPolygonVertex( polyIndex, v );
					pos.position.x = scast<float>( pControlPointsArray[ctrlPointIndex][0] );
					pos.position.y = scast<float>( pControlPointsArray[ctrlPointIndex][1] );
					pos.position.z = scast<float>( pControlPointsArray[ctrlPointIndex][2] );

					pFBXMesh->GetPolygonVertexNormal( polyIndex, v, fbxNormal );
					pos.normal.x   = scast<float>( fbxNormal[0] );
					pos.normal.y   = scast<float>( fbxNormal[1] );
					pos.normal.z   = scast<float>( fbxNormal[2] );

					const int uvCount = pFBXMesh->GetElementUVCount();
					if ( !uvCount )
					{
						tex.texCoord = Donya::Vector2::Zero();
					}
					else
					{
						bool ummappedUV{};
						FbxVector2 uv{};
						pFBXMesh->GetPolygonVertexUV( polyIndex, v, uvSetName[0], uv, ummappedUV );
					
						tex.texCoord.x = scast<float>( uv[0] );
						tex.texCoord.y = 1.0f - scast<float>( uv[1] ); // For DirectX's uv space(the origin is left-top).
					}

					auto AssignInfluence = [&NormalizeBoneInfluence]( Model::Vertex::Bone *pInfl, const BoneInfluence &source )
					{
						constexpr size_t MAX_INFLUENCE_COUNT = 4U; // Align as float4.
						BoneInfluence infl = NormalizeBoneInfluence( source, MAX_INFLUENCE_COUNT );

						auto At = []( auto &vec4, int index )->auto &
						{
							switch ( index )
							{
							case 0: return vec4.x;
							case 1: return vec4.y;
							case 2: return vec4.z;
							case 3: return vec4.w;
							default: assert( 0 ); break;
							}
							// Safety.
							return vec4.x;
						};

						const size_t sourceCount	= infl.data.size();
						const size_t firstLoopCount	= std::min( sourceCount, MAX_INFLUENCE_COUNT );
						size_t  i = 0;
						for ( ; i <  firstLoopCount; ++i )
						{
							At( pInfl->weights, i ) = infl.data[i].first;
							At( pInfl->indices, i ) = infl.data[i].second;
						}
						for ( ; i <  MAX_INFLUENCE_COUNT; ++i )
						{
							At( pInfl->weights, i ) = ( i == 0 ) ? 1.0f : 0.0f;
							At( pInfl->indices, i ) = NULL;
						}
					};

					if ( v < scast<int>( polygon.points.size() ) )
					{
						Donya::Vector4 transformedPos = globalTransform.Mul( pos.position, 1.0f );
						polygon.points[v] = transformedPos.XYZ();
						// Donya::Vector4 transformedPos = adjustMatrix.Mul( pos.position, 1.0f );
						// polygon.points[polygon.points.size() - 1 - v] = transformedPos.XYZ();
					}

					pMesh->positions.emplace_back( pos );
					pMesh->texCoords.emplace_back( tex );

					AssignInfluence( &infl, boneInfluences[ctrlPointIndex] );
					pMesh->boneInfluences.emplace_back( infl );

					pMesh->indices[indexOffset + v] = vertexCount;
					vertexCount++;
				}
				subset.indexCount += EXPECT_POLYGON_SIZE;

				const Donya::Vector3 edgeAB = polygon.points[1] - polygon.points[0];
				const Donya::Vector3 edgeAC = polygon.points[2] - polygon.points[0];
				polygon.normal = Donya::Cross( edgeAB, edgeAC ).Unit();
				pPolygons->emplace_back( std::move( polygon ) );
			}
		}
	}
	void BuildMeshes( std::vector<Model::Source::Mesh> *pMeshes, const std::vector<FBX::FbxNode *> &meshNodes, std::vector<Model::Polygon> *pPolygons, FBX::FbxScene *pScene, const std::string &fileDirectory, const std::vector<Model::Animation::Bone> &modelSkeletal, float animationSamplingFPS )
	{
		const size_t meshCount = meshNodes.size();
		pMeshes->resize( meshCount );
		for ( size_t i = 0; i < meshCount; ++i )
		{
			FBX::FbxMesh *pFBXMesh = meshNodes[i]->GetMesh();
			_ASSERT_EXPR( pFBXMesh, L"Error : A mesh-node that passed mesh-nodes is not mesh!" );

			BuildMesh( &( *pMeshes )[i], meshNodes[i], pFBXMesh, pPolygons, pScene, fileDirectory, modelSkeletal, animationSamplingFPS );
		}
	}

	void BuildModelSource( Model::Source *pSource, std::vector<Model::Polygon> *pPolygons, FBX::FbxScene *pScene, const std::vector<FBX::FbxNode *> &meshNodes, const std::vector<FBX::FbxNode *> &motionNodes, float animationSamplingFPS, const std::string &fileDirectory )
	{
		BuildSkeletal( &pSource->skeletal, motionNodes );

		// The meshes building function is using the skeletal, so we should build after building of the skeletal.
		BuildMeshes( &pSource->meshes, meshNodes, pPolygons, pScene, fileDirectory, pSource->skeletal, animationSamplingFPS );

		BuildMotions( &pSource->motions, motionNodes, pScene, animationSamplingFPS );
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
		OutputDebugProgress( "Start Separating File-Path.", outputProgress );

		fileDirectory	= ExtractFileDirectoryFromFullPath( filePath );
		fileName		= filePath.substr( fileDirectory.size() ); // 0x7598C632 で例外がスローされました (Lex.exe 内): Microsoft C++ の例外: std::out_of_range (メモリの場所 0x0EA4E994)。
		// TODO : Fix this exception that occurred here.

		MakeAbsoluteFilePath( filePath );

		OutputDebugProgress( "Finish Separating File-Path.", outputProgress );

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
			OutputDebugProgress( "Start Triangulate.", outputProgress );

			FBX::FbxGeometryConverter geometryConverter( pManager );
			bool replace = true;
			geometryConverter.Triangulate( pScene, replace );

			OutputDebugProgress( "Finish Triangulate.", outputProgress );
		}
	#endif

		std::vector<FBX::FbxNode *> fetchedMeshNodes{};
		std::vector<FBX::FbxNode *> fetchedAnimNodes{};
		Traverse( pScene->GetRootNode(), &fetchedMeshNodes, &fetchedAnimNodes );

		OutputDebugProgress( "Start Building model-source.", outputProgress );
		BuildModelSource
		(
			&source, &polygons,
			pScene, fetchedMeshNodes, fetchedAnimNodes,
			sampleFPS, fileDirectory
		);
		OutputDebugProgress( "Finish Building model-source.", outputProgress );

		size_t meshCount = fetchedMeshNodes.size();
		OutputDebugProgress( "Start Meshes load. Meshes count:[" + std::to_string( meshCount ) + "]", outputProgress );
		
		std::vector<BoneInfluencesPerControlPoint> influencesPerCtrlPoints{};

		meshes.resize( meshCount );
		for ( size_t i = 0; i < meshCount; ++i )
		{
			meshes[i].meshNo = scast<int>( i );
			
			FBX::FbxMesh *pMesh = fetchedMeshNodes[i]->GetMesh();

			influencesPerCtrlPoints.clear();
			FetchBoneInfluences( pMesh, influencesPerCtrlPoints );

			FetchGlobalTransform( i, pMesh );
			FetchVertices( i, pMesh, influencesPerCtrlPoints, meshes[i].globalTransform );
			FetchMaterial( i, pMesh );

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

	void Loader::FetchVertices( size_t meshIndex, const FBX::FbxMesh *pMesh, const std::vector<BoneInfluencesPerControlPoint> &fetchedInfluences, const Donya::Vector4x4 &globalTransform )
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
					Donya::Vector4 transformedPos = globalTransform.Mul( position, 1.0f );
					colFace.points[v].x = transformedPos.x;
					colFace.points[v].y = transformedPos.y;
					colFace.points[v].z = transformedPos.z;
				}

				mesh.indices[indexOffset + v] = vertexCount;
				vertexCount++;

				// Sort by descending-order.
				auto SortInfluences = []( BoneInfluencesPerControlPoint *pSource )
				{
					auto DescendingCompare = []( const BoneInfluence &lhs, const BoneInfluence &rhs )
					{
						return ( lhs.weight > rhs.weight ) ? true : false;
					};

					std::sort( pSource->cluster.begin(), pSource->cluster.end(), DescendingCompare );
				};
				// Sort and Shrink bone-influences count to up to maxInfluenceCount.
				auto NormalizeBoneInfluence = [&SortInfluences]( BoneInfluencesPerControlPoint source, size_t maxInfluenceCount )
				{
					if ( source.cluster.size() <= maxInfluenceCount )
					{
						SortInfluences( &source );
						return source;
					}
					// else

					/*
					0,	Prepare the buffer to default-value.
					1,	Sort with weight the influences by descending order.
					2,	Assign the higher data of influences to result as many as maxInfluenceCount.
					3,	Add the remaining influences data to highest weight bone.
					*/

					BoneInfluencesPerControlPoint result{};
					auto Append = [&result]( float weight, int index )
					{
						BoneInfluence addition{};
						addition.weight = weight;
						addition.index  = index;
						result.cluster.emplace_back( std::move( addition ) );
					};
					auto Assign = [&result]( size_t elementIndex, float weight, int index )
					{
						result.cluster[elementIndex].weight = weight;
						result.cluster[elementIndex].index  = index;
					};

					// No.0, Default-value is all zero but the first weight is one.
					Append( 1.0f, 0 );
					for ( size_t i = 1; i < maxInfluenceCount; ++i )
					{
						Append( 0.0f, 0 );
					}

					// No.1
					SortInfluences( &source );

					// No.2
					size_t  loopIndex = 0;
					for ( ; loopIndex < maxInfluenceCount; ++loopIndex )
					{
						if ( maxInfluenceCount <= loopIndex ) { continue; }
						// else

						Assign( loopIndex, source.cluster[loopIndex].weight, source.cluster[loopIndex].index );
					}

					// No.3
					size_t highestBoneIndex{};
					{
						float highestWeight = 0.0f;
						for ( size_t i = 0; i < maxInfluenceCount; ++i )
						{
							float selectWeight = result.cluster[i].weight;
							if ( highestWeight < selectWeight )
							{
								highestBoneIndex = i;
								highestWeight = selectWeight;
							}
						}
					}
					const size_t sourceCount = source.cluster.size();
					for ( ; loopIndex < sourceCount; ++loopIndex )
					{
						result.cluster[highestBoneIndex].weight += source.cluster[loopIndex].weight;
					}

					return result;
				};

				BoneInfluencesPerControlPoint targetInfluence = fetchedInfluences[ctrlPointIndex];
				mesh.influences.push_back( NormalizeBoneInfluence( targetInfluence, 4U ) );
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
				auto tmp = pOutMtl->relativeTexturePaths.size();
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

		// Show Model::Source.
		if ( ImGui::TreeNode( u8"モデルソース" ) )
		{
			const size_t meshCount = source.meshes.size();
			for ( size_t i = 0; i < meshCount; ++i )
			{
				const auto &mesh = source.meshes[i];
				const std::string meshCaption = "Mesh[" + std::to_string( i ) + "]";
				if ( ImGui::TreeNode( meshCaption.c_str() ) )
				{
					const size_t verticesCount = mesh.indices.size();
					std::string verticesCaption = "Vertices[Count:" + std::to_string( verticesCount ) + "]";
					if ( ImGui::TreeNode( verticesCaption.c_str() ) )
					{
						if ( ImGui::TreeNode( "Vertex" ) )
						{
							const auto &pos = mesh.positions;
							const auto &tex = mesh.texCoords;
							if ( pos.size() != tex.size() )
							{
								ImGui::Text( "An error occured. So can't showing parameters." );
							}
							else
							{
								ImGui::BeginChild( ImGui::GetID( scast<void *>( NULL ) ), childFrameSize );
								const size_t end = pos.size();
								for ( size_t i = 0; i < end; ++i )
								{
									ImGui::Text( "Position:[No:%d][X:%6.3f][Y:%6.3f][Z:%6.3f]",	i, pos[i].position.x,	pos[i].position.y,	pos[i].position.z	);
									ImGui::Text( "Normal:[No:%d][X:%6.3f][Y:%6.3f][Z:%6.3f]",	i, pos[i].normal.x,		pos[i].normal.y,	pos[i].normal.z		);
									ImGui::Text( "TexCoord:[No:%d][X:%6.3f][Y:%6.3f]",			i, tex[i].texCoord.x,	tex[i].texCoord.y	);
								}
								ImGui::EndChild();
							}

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
								auto ShowMaterialContain = [this]( const Model::Source::Material &mtl )
								{
									ImGui::Text
									(
										"Color:[X:%05.3f][Y:%05.3f][Z:%05.3f][W:%05.3f]",
										mtl.color.x, mtl.color.y, mtl.color.z, mtl.color.w
									);

									if ( mtl.textureName.empty() )
									{
										ImGui::Text( "This material don't have texture." );
										return;
									}
									// else
									
									if ( !Donya::IsExistFile( fileDirectory + mtl.textureName ) )
									{
										ImGui::Text( "!This texture was not found![%s]", mtl.textureName.c_str() );
									}
									else
									{
										ImGui::Text( "Texture Name:[%s]", mtl.textureName.c_str() );
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
							size_t boneInfluencesCount = mesh.boneInfluences.size();
							for ( size_t v = 0; v < boneInfluencesCount; ++v )
							{
								ImGui::Text( "Vertex No[%d]", v );

								auto &data = mesh.boneInfluences[v];
								ImGui::Text
								(
									"\t[Index:%d][Weight[%6.4f]",
									data.indices.x,
									data.weights.x
								);
								ImGui::Text
								(
									"\t[Index:%d][Weight[%6.4f]",
									data.indices.y,
									data.weights.y
								);
								ImGui::Text
								(
									"\t[Index:%d][Weight[%6.4f]",
									data.indices.z,
									data.weights.z
								);
								ImGui::Text
								(
									"\t[Index:%d][Weight[%6.4f]",
									data.indices.w,
									data.weights.w
								);
							}
							ImGui::EndChild();

							ImGui::TreePop();
						}
						
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}

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