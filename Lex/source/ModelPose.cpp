#include "ModelPose.h"

namespace Donya
{
	namespace Model
	{
		const std::vector<Animation::Node> &Pose::GetCurrentPose() const { return skeletal; }

		bool Pose::HasCompatibleWith( const std::vector<Animation::Node> &validation ) const
		{
			/*
			Requirements:
				1. The bone count is the same.
				2. Each bones name are the same as others.
			*/

			// No.1
			if ( validation.size() != skeletal.size() ) { return false; }
			// else

			// No.2
			const size_t boneCount = skeletal.size();
			for ( size_t i = 0;  i < boneCount; ++i )
			{
				if ( validation[i].bone.name != skeletal[i].bone.name )
				{
					return false;
				}
			}

			return true;
		}
		bool Pose::HasCompatibleWith( const Animation::KeyFrame &validation ) const
		{
			return HasCompatibleWith( validation.keyPose );
		}

		void Pose::AssignSkeletal( const std::vector<Animation::Node> &newPose )
		{
			const size_t newSize = newPose.size();
			skeletal.clear();
			skeletal.resize( newSize );

			for ( size_t i = 0; i < newSize; ++i )
			{
				skeletal[i] = newPose[i];
			}
		}
		void Pose::AssignSkeletal( const Animation::KeyFrame &newKeyFrame )
		{
			AssignSkeletal( newKeyFrame.keyPose );
		}

		void Pose::UpdateTransformMatrices()
		{
			UpdateLocalMatrices();
			UpdateGlobalMatrices();
		}
		void Pose::UpdateLocalMatrices()
		{
			for ( auto &it : skeletal )
			{
				it.local = it.bone.transform.ToWorldMatrix();
			}
		}
		void Pose::UpdateGlobalMatrices()
		{
			for ( auto &it : skeletal )
			{
				if ( it.bone.parentIndex == -1 )
				{
					it.global = it.local;
				}
				else
				{
					const auto &parentBone = skeletal[it.bone.parentIndex];
					it.global = it.local * parentBone.global;
				}
			}
		}
	}
}
