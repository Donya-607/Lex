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

namespace
{
	void OutputDebugProgress( const std::string &str, bool isAllowOutput )
{
	if ( !isAllowOutput ) { return; }
	// else

	const std::string prefix { "[Donya.LoadProgress]:" };
	const std::string postfix{ "\n" };
	Donya::OutputDebugStr( ( prefix + str + postfix ).c_str() );
}
}

namespace Donya
{
	std::mutex Loader::cerealMutex{};

#if USE_FBX_SDK
	std::mutex Loader::fbxMutex{};

	Donya::Vector2		Convert( const FBX::FbxDouble2 &source )
	{
		return Donya::Vector2
		{
			scast<float>( source.mData[0] ),
			scast<float>( source.mData[1] )
		};
	}
	Donya::Vector3		Convert( const FBX::FbxDouble3 &source )
	{
		return Donya::Vector3
		{
			scast<float>( source.mData[0] ),
			scast<float>( source.mData[1] ),
			scast<float>( source.mData[2] )
		};
	}
	Donya::Vector4		Convert( const FBX::FbxDouble4 &source )
	{
		return Donya::Vector4
		{
			scast<float>( source.mData[0] ),
			scast<float>( source.mData[1] ),
			scast<float>( source.mData[2] ),
			scast<float>( source.mData[3] )
		};
	}
	Donya::Quaternion	ToQuaternion( const FBX::FbxDouble4 &source )
	{
		return Donya::Quaternion
		{
			scast<float>( source.mData[0] ),
			scast<float>( source.mData[1] ),
			scast<float>( source.mData[2] ),
			scast<float>( source.mData[3] )
		};
	}
	DirectX::XMFLOAT4X4	Convert( const FBX::FbxAMatrix &affineMatrix )
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

	Model::Animation::Transform SeparateSRT( const FBX::FbxAMatrix &source )
	{
		Model::Animation::Transform output;
		output.scale		= Convert( source.GetS() ).XYZ();
		output.rotation		= ToQuaternion( source.GetQ() );
		output.translation	= Convert( source.GetT() ).XYZ();
		return output;
	}

	constexpr bool HasMesh		( FBX::FbxNodeAttribute::EType attr )
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
	constexpr bool HasSkeletal	( FBX::FbxNodeAttribute::EType attr )
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
	int  FindBoneIndex( const std::vector<Model::Animation::Node> &source, const std::string &keyName )
	{
		const size_t boneCount = source.size();
		for ( size_t i = 0; i < boneCount; ++i )
		{
			if ( source[i].bone.name == keyName )
			{
				return scast<int>( i );
			}
		}

		return -1;
	}

	void AdjustCoordinate( Model::Source *pSource, Model::PolygonGroup *pPolyGroup )
	{
		// Convert right-hand space to left-hand space.
		pSource->coordinateConversion._11 = -1.0f;
		pPolyGroup->SetCullMode( Model::PolygonGroup::CullMode::Back );
	}

	/// <summary>
	/// Assign the "local" from that "bone", then calcuate the "global" matrices..
	/// </summary>
	void UpdateTransformMatrices( std::vector<Model::Animation::Node> *pSkeletal )
	{
		// Assign each "local".
		for ( auto &it : *pSkeletal )
		{
			it.local = it.bone.transform.ToWorldMatrix();
		}

		// Calc each "global".
		for ( auto &it : *pSkeletal )
		{
			if ( it.bone.parentIndex == -1 )
			{
				it.global = it.local;
			}
			else
			{
				const auto &parentBone = pSkeletal->at( it.bone.parentIndex );
				it.global = it.local * parentBone.global;
			}
		}
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
	void BuildSkeletal( std::vector<Model::Animation::Node> *pSkeletal, const std::vector<FBX::FbxNode *> &skeletalNodes, const FBX::FbxTime &currentTime = FBX::FBXSDK_TIME_INFINITE )
	{
		std::vector<Model::Animation::Node> &data = *pSkeletal;

		const size_t nodeCount = skeletalNodes.size();
		data.resize( nodeCount );
		for ( size_t i = 0; i < nodeCount; ++i )
		{
			Model::Animation::Bone &bone = data[i].bone;
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
			Model::Animation::Bone &bone = data[i].bone;
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

		UpdateTransformMatrices( &data );
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
	void BuildMesh( Model::Source::Mesh *pMesh, FBX::FbxNode *pNode, FBX::FbxMesh *pFBXMesh, std::vector<Model::Polygon> *pPolygons, FBX::FbxScene *pScene, const std::string &fileDirectory, const std::vector<Model::Animation::Node> &modelSkeletal, float animationSamplingFPS )
	{
		constexpr	int EXPECT_POLYGON_SIZE	= 3;
		const		int mtlCount			= pNode->GetMaterialCount();
		const		int polygonCount		= pFBXMesh->GetPolygonCount();

		pMesh->indices.resize( polygonCount * EXPECT_POLYGON_SIZE );
		pMesh->name = pNode->GetName();

		// TODO : Should separate the calculation of an indexCount and indexStart from here.
		// TODO : Should separate the calculation of an indices array from here.
		BuildSubsets( &pMesh->subsets, pFBXMesh, fileDirectory );

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
			auto FetchBoneOffset	= [&geometricTransform]( const FBX::FbxCluster *pCluster, std::vector<Model::Animation::Node> *pBoneOffsets )
			{
				const FBX::FbxAMatrix  boneOffset = FetchBoneOffsetMatrix( pCluster );
				
				Model::Animation::Node node{};
				node.bone.name				= pCluster->GetLink()->GetName();
				node.bone.transform			= SeparateSRT( boneOffset * geometricTransform );
				node.local	= node.bone.transform.ToWorldMatrix();

				// The bone offset matrix does not use something related to parent related.
				node.bone.parentName		= "";
				node.bone.parentIndex		= -1;
				node.bone.transformToParent	= Model::Animation::Transform::Identity();
				node.global	= node.local;

				pBoneOffsets->emplace_back( std::move( node ) );
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

			UpdateTransformMatrices( &pMesh->boneOffsets );

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
	void BuildMeshes( std::vector<Model::Source::Mesh> *pMeshes, const std::vector<FBX::FbxNode *> &meshNodes, Model::PolygonGroup *pPolyGroup, FBX::FbxScene *pScene, const std::string &fileDirectory, const std::vector<Model::Animation::Node> &modelSkeletal, float animationSamplingFPS )
	{
		std::vector<Model::Polygon> polygons;

		const size_t meshCount = meshNodes.size();
		pMeshes->resize( meshCount );
		for ( size_t i = 0; i < meshCount; ++i )
		{
			FBX::FbxMesh *pFBXMesh = meshNodes[i]->GetMesh();
			_ASSERT_EXPR( pFBXMesh, L"Error : A mesh-node that passed mesh-nodes is not mesh!" );

			BuildMesh( &( *pMeshes )[i], meshNodes[i], pFBXMesh, &polygons, pScene, fileDirectory, modelSkeletal, animationSamplingFPS );
		}

		pPolyGroup->Assign( std::move( polygons ) );
	}

	void BuildModelSource( Model::Source *pSource, Model::PolygonGroup *pPolyGroup, FBX::FbxScene *pScene, const std::vector<FBX::FbxNode *> &meshNodes, const std::vector<FBX::FbxNode *> &motionNodes, float animationSamplingFPS, const std::string &fileDirectory )
	{
		AdjustCoordinate( pSource, pPolyGroup );

		BuildSkeletal( &pSource->skeletal, motionNodes );

		// The meshes building function is using the skeletal, so we should build after building of the skeletal.
		BuildMeshes( &pSource->meshes, meshNodes, pPolyGroup, pScene, fileDirectory, pSource->skeletal, animationSamplingFPS );

		BuildMotions( &pSource->motions, motionNodes, pScene, animationSamplingFPS );
	}

#endif // USE_FBX_SDK

	bool Loader::Load( const std::string &filePath, bool outputProgress )
	{
		const std::string fullPath = ToFullPath( filePath );

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

			bool succeeded = LoadByFBXSDK( fullPath, outputProgress );

			const std::string resultString = ( succeeded ) ? "Load By FBX Successful:" : "Load By FBX Failed:";
			OutputDebugProgress( resultString + filePath, outputProgress );

			return succeeded;
		}
		// else

	#endif // USE_FBX_SDK

		auto ShouldLoadByCereal = []( const std::string &filePath )
		{
			constexpr std::array<const char *, 1> EXTENSIONS
			{
				".bin"
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
		if ( ShouldLoadByCereal( fullPath ) )
		{
			OutputDebugProgress( std::string{ "Start By Cereal:" + filePath }, outputProgress );

			bool succeeded = LoadByCereal( fullPath, outputProgress );

			const std::string resultString = ( succeeded ) ? "Load By Cereal Successful:" : "Load By Cereal Failed:";
			OutputDebugProgress( resultString + filePath, outputProgress );

			return succeeded;
		}
		// else

		OutputDebugProgress( "Failed : The file has not valid extension:" + filePath, outputProgress );

		return false;
	}

	void Loader::SaveByCereal( const std::string &filePath ) const
	{
		Donya::Serializer::Extension bin  = Donya::Serializer::Extension::BINARY;

		std::lock_guard<std::mutex> lock( cerealMutex );
		
		Donya::Serializer seria;
		seria.Save( bin, filePath.c_str(),  SERIAL_ID, *this );
	}
	
	bool Loader::LoadByCereal( const std::string &filePath, bool outputProgress )
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

	std::string GetUTF8FullPath( const std::string &inputFilePath, size_t filePathLengthMax = 512U )
	{
		// reference to http://blog.livedoor.jp/tek_nishi/archives/9446152.html

		std::unique_ptr<char[]> fullPath = std::make_unique<char[]>( filePathLengthMax );
		auto writeLength = GetFullPathNameA( inputFilePath.c_str(),  filePathLengthMax, fullPath.get(), nullptr );

		char *convertedPath = nullptr;
		FBX::FbxAnsiToUTF8( fullPath.get(), convertedPath );

		std::string convertedStr( convertedPath );

		FBX::FbxFree( convertedPath );

		return convertedStr;
	}

	bool Loader::LoadByFBXSDK( const std::string &sourceFilePath, bool outputProgress )
	{
		OutputDebugProgress( "Start Separating File-Path.", outputProgress );

		filePath		= GetUTF8FullPath( sourceFilePath );
		fileDirectory	= ExtractFileDirectoryFromFullPath( sourceFilePath );
		fileName		= sourceFilePath.substr( fileDirectory.size() );

		OutputDebugProgress( "Finish Separating File-Path.", outputProgress );

		std::unique_ptr<std::lock_guard<std::mutex>> pLock{}; // Use scoped-lock without code-bracket.
		pLock = std::make_unique<std::lock_guard<std::mutex>>( fbxMutex );

		FBX::FbxManager		*pManager		= FBX::FbxManager::Create();
		FBX::FbxIOSettings	*pIOSettings	= FBX::FbxIOSettings::Create( pManager, IOSROOT );
		pManager->SetIOSettings( pIOSettings );

		auto Uninitialize = [&]
		{
			pManager->Destroy();
		};

		FBX::FbxScene *pScene = FBX::FbxScene::Create( pManager, "" );

		// Import.
		{
			OutputDebugProgress( "Start Import.", outputProgress );

			FBX::FbxImporter *pImporter = FBX::FbxImporter::Create( pManager, "" );
			if ( !pImporter->Initialize( filePath.c_str(), -1, pManager->GetIOSettings() ) )
			{
				std::string errMsg = "Failed Initialize. What: ";
				errMsg += pImporter->GetStatus().GetErrorString();

				Uninitialize();
				return false;
			}

			if ( !pImporter->Import( pScene ) )
			{
				std::string errMsg = "Failed Import. What: ";
				errMsg += pImporter->GetStatus().GetErrorString();

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

		OutputDebugProgress( "Start Extract fbx nodes.", outputProgress );
		std::vector<FBX::FbxNode *> fetchedMeshNodes{};
		std::vector<FBX::FbxNode *> fetchedBoneNodes{};
		Traverse( pScene->GetRootNode(), &fetchedMeshNodes, &fetchedBoneNodes );
		OutputDebugProgress( "Finish Extract fbx nodes.", outputProgress );

		OutputDebugProgress( "Start Building model-source.", outputProgress );
		BuildModelSource
		(
			&source, &polyGroup,
			pScene, fetchedMeshNodes, fetchedBoneNodes,
			sampleFPS, fileDirectory
		);
		OutputDebugProgress( "Finish Building model-source.", outputProgress );

		Uninitialize();
		return true;
	}

#endif // USE_FBX_SDK

#if USE_IMGUI
	void Loader::ShowEnumNode( const std::string &nodeCaption ) const
	{
		const ImVec2 childFrameSize( 0.0f, 0.0f );

		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

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

					if ( ImGui::TreeNode( "Index" ) )
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
#endif // USE_IMGUI
}
