#include "ModelCommon.h"

namespace Donya
{
	namespace Model
	{
		namespace Animation
		{
			Donya::Vector4x4 Transform::ToWorldMatrix() const
			{
				Donya::Vector4x4 m{};
				m._11 = scale.x;
				m._22 = scale.y;
				m._33 = scale.z;
				m *= rotation.RequireRotationMatrix();
				m._41 = translation.x;
				m._42 = translation.y;
				m._43 = translation.z;
				return m;
			}
			Transform Transform::Interpolate( const Transform &lhs, const Transform &rhs, float time )
			{
				Transform rv;
				rv.scale		= Donya::Lerp( lhs.scale, rhs.scale, time );
				rv.rotation		= Donya::Quaternion::Slerp( lhs.rotation, rhs.rotation, time );
				rv.translation	= Donya::Lerp( lhs.translation, rhs.translation, time );
				return rv;
			}

			Bone Bone::Interpolate( const Bone &lhs, const Bone &rhs, float time )
			{
				Bone rv = lhs;
				rv.transform			= Transform::Interpolate( lhs.transform,			rhs.transform,			time );
				rv.transformToParent	= Transform::Interpolate( lhs.transformToParent,	rhs.transformToParent,	time );
				return rv;
			}

			Node Node::Interpolate( const Node &lhs, const Node &rhs, float time )
			{
				Node rv;
				rv.bone		= Bone::Interpolate( lhs.bone, rhs.bone, time );
				rv.local	= rv.bone.transform.ToWorldMatrix();
				rv.global	= Donya::Lerp( lhs.global, rhs.global, time );
				return rv;
			}

			KeyFrame KeyFrame::Interpolate( const KeyFrame &lhs, const KeyFrame &rhs, float time )
			{
				_ASSERT_EXPR( lhs.keyPose.size() == rhs.keyPose.size(), L"Error : We can not interpolate between the vectors that difference size!" );
				const size_t boneCount = lhs.keyPose.size();

				KeyFrame rv;
				rv.seconds = lhs.seconds + ( time * ( rhs.seconds - lhs.seconds ) );
				rv.keyPose.resize( boneCount );
				for ( size_t i = 0; i < boneCount; ++i )
				{
					rv.keyPose[i] = Node::Interpolate( lhs.keyPose[i], rhs.keyPose[i], time );
				}

				return rv;
			}
		}
	}
}
