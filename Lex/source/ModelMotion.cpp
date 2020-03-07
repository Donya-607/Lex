#include "ModelMotion.h"

#include "Donya/Constant.h"	// Use scast macro.
#include "Donya/Useful.h"	// Use ZeroEqual().

namespace Donya
{
	namespace Model
	{
		Animation::Motion EmptyMotion()
		{
			return Animation::Motion{};
		}

		size_t MotionHolder::GetMotionCount() const
		{
			return motions.size();
		}
		bool MotionHolder::IsOutOfRange( int motionIndex ) const
		{
			if ( motionIndex < 0 ) { return true; }
			if ( scast<int>( GetMotionCount() ) <= motionIndex ) { return true; }
			// else

			return false;
		}

		Animation::Motion MotionHolder::GetMotion( int motionIndex ) const
		{
			if ( IsOutOfRange( motionIndex ) ) { return EmptyMotion(); }
			// else
			return motions[motionIndex];
		}
		Animation::Motion MotionHolder::FindMotion( const std::string &motionName ) const
		{
			for ( const auto &it : motions )
			{
				if ( it.name == motionName )
				{
					return it;
				}
			}

			return EmptyMotion();
		}

		void MotionHolder::AppendMotion( const Animation::Motion &element )
		{
			motions.emplace_back( element );
		}



		bool FocusMotion::IsValidMotion( const Animation::Motion &motion ) const
		{
			/*
			Requirements:
			1.	The global transform count is the same as the keyframe count.
			2.	(When the focus is not registered)
					The keyframes is empty.
			3.	(When the focus is registered)
					The bone count of all key pose of keyframes is same as my bone count.
			*/

			// No.1
			if ( motion.globalTransforms.size() != motion.keyFrames.size() ) { return false; }
			// else
			
			// No.2
			if ( skeletal.empty() )
			{
				return ( motion.keyFrames.empty() ) ? true : false;
			}
			// else

			// No.3

			if ( motion.keyFrames.empty() ) { return false; };
			// else
			const size_t boneCount = skeletal.size();
			for ( const auto &keyFrame : motion.keyFrames )
			{
				if ( keyFrame.keyPose.size() != boneCount )
				{
					return false;
				}
			}
			
			return true;
		}

		void FocusMotion::RegisterMotion( const Animation::Motion &target )
		{
			focus = target;
		}
		Animation::Motion FocusMotion::GetFocusingMotion() const
		{
			return focus;
		}

		const std::vector<FocusMotion::Node> &FocusMotion::GetCurrentSkeletal() const
		{
			return skeletal;
		}

		void FocusMotion::UpdateCurrentPose( float currentFrame )
		{
			const auto currentTransform = CalcCurrentPose( currentFrame );
			AssignTransform( currentTransform );

			CalcLocalMatrix();

			CalcGlobalMatrix();
		}
		void FocusMotion::UpdateCurrentPose( const Animator &animator )
		{
			CalcCurrentPose( animator.CalcCurrentFrame() );
		}

		FocusMotion::LerpedTransform FocusMotion::CalcCurrentPose( float currentFrame )
		{
			if ( !IsValidMotion( focus ) )
			{
				_ASSERT_EXPR( 0, L"Error : The focusing motion is invalid!" );
				return LerpedTransform::Empty();
			}
			// else

			const size_t frameCount = focus.keyFrames.size();

			float  integral{};
			float  fractional = modf( currentFrame, &integral );	// Will using as percent of interpolation.
			size_t baseFrame  = scast<size_t>( integral );
			size_t nextFrame  = ( frameCount <= baseFrame + 1 )
								? ( baseFrame + 1 ) % frameCount	// Wrap around.
								: baseFrame + 1;

			auto SlerpTransform	= []( const Animation::Transform &lhs, const Animation::Transform &rhs, float percent )
			{
				Animation::Transform rv = lhs;
				rv.scale		= Donya::Lerp( lhs.scale, rhs.scale, percent );
				rv.rotation		= Donya::Quaternion::Slerp( lhs.rotation, rhs.rotation, percent );
				rv.translation	= Donya::Lerp( lhs.translation, rhs.translation, percent );
				return rv;
			};

			auto CalcLerpedPose = [&]()->Animation::KeyFrame
			{
				const Animation::KeyFrame currentPose	= focus.keyFrames[baseFrame];
				const Animation::KeyFrame nextPose		= focus.keyFrames[nextFrame];

				if ( ZeroEqual( fractional ) ) { return currentPose; }
				// else

				_ASSERT_EXPR( currentPose.keyPose.size() == nextPose.keyPose.size(), L"Error : The bone count did not match! " );
				auto SlerpBone = [&SlerpTransform]( const Animation::Bone &lhs, const Animation::Bone rhs, float percent )
				{
					Animation::Bone rv	= lhs;
					rv.transformOffset	= SlerpTransform( lhs.transformOffset,	rhs.transformOffset,	percent );
					rv.transformPose	= SlerpTransform( lhs.transformPose,	rhs.transformPose,		percent );
					return rv;
				};

				// A member that does not interpolate is using as currentPose.
				Animation::KeyFrame resultPose = currentPose;

				const size_t boneCount = currentPose.keyPose.size();
				for ( size_t i = 0; i < boneCount; ++i )
				{
					resultPose.keyPose[i] = SlerpBone( currentPose.keyPose[i], nextPose.keyPose[i], fractional );
				}

				return resultPose;
			};
			auto CalcLerpedGlobalTransform = [&]()->Animation::Transform
			{
				const Animation::Transform currentTransform	= focus.globalTransforms[baseFrame];
				const Animation::Transform nextTransform	= focus.globalTransforms[nextFrame];

				if ( ZeroEqual( fractional ) ) { return currentTransform; }
				// else

				return SlerpTransform( currentTransform, nextTransform, fractional );
			};

			LerpedTransform result{};
			result.keyFrame = CalcLerpedPose();
			result.globalTransform = CalcLerpedGlobalTransform();
			return result;
		}

		void FocusMotion::AssignTransform( const LerpedTransform &transform )
		{
			_ASSERT_EXPR( transform.keyFrame.keyPose.size() == skeletal.size(), L"Error : The bone count of focusing motion is invalid!" );
			const size_t boneCount = skeletal.size();
			for ( size_t i = 0; i < boneCount; ++i )
			{
				skeletal[i].bone = transform.keyFrame.keyPose[i];
			}

			globalTransform = transform.globalTransform.ToWorldMatrix();
		}

		void FocusMotion::CalcLocalMatrix()
		{
			for ( auto &it : skeletal )
			{
				it.local = it.bone.transformPose.ToWorldMatrix();
			}
		}
		void FocusMotion::CalcGlobalMatrix()
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
