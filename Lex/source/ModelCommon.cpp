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
			Transform Transform::Interpolate( const Transform &lhs, const Transform &rhs, float percent )
			{
				Transform rv;
				rv.scale		= Donya::Lerp( lhs.scale, rhs.scale, percent );
				rv.rotation		= Donya::Quaternion::Slerp( lhs.rotation, rhs.rotation, percent );
				rv.translation	= Donya::Lerp( lhs.translation, rhs.translation, percent );
				return rv;
			}

			Bone Bone::Interpolate( const Bone &lhs, const Bone &rhs, float percent )
			{
				Bone rv = lhs;
				rv.transform			= Transform::Interpolate( lhs.transform,			rhs.transform,			percent );
				rv.transformToParent	= Transform::Interpolate( lhs.transformToParent,	rhs.transformToParent,	percent );
				return rv;
			}

			KeyFrame KeyFrame::Interpolate( const KeyFrame &lhs, const KeyFrame &rhs, float percent )
			{
				KeyFrame rv;
				rv.seconds = ( lhs.seconds * ( 1.0f - percent ) ) + ( rhs.seconds * percent );
			}
		}
	}
}
