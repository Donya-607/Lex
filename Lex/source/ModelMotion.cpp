#include "ModelMotion.h"

#include "Donya/Constant.h"	// Use scast macro.
#include "Donya/Useful.h"	// Use ZeroEqual().

namespace Donya
{
	namespace Model
	{
		Animation::KeyFrame	EmptyKeyFrame()
		{
			return Animation::KeyFrame{};
		}
		Animation::Motion	EmptyMotion()
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



		bool FocusMotion::IsValidMotion( const Animation::Motion &motion )
		{
			/*
			Requirements:
				1. The motion has something.
				2. The motion's bone count of all key pose of keyframes is the same.
			*/

			const size_t boneCount = motion.keyFrames.size();

			// No.1
			if ( !boneCount ) { return false; };
			// else

			// No.2
			for ( const auto &keyFrame : motion.keyFrames )
			{
				if ( keyFrame.keyPose.size() != boneCount )
				{
					return false;
				}
			}
			
			return true;
		}

		bool FocusMotion::RegisterMotion( const Animation::Motion &target )
		{
			if ( !IsValidMotion( target ) )
			{
				focus = EmptyMotion();

				skeletal.clear();
				return false;
			}
			// else

			focus = target;

			AdaptToFocus();
			return true;
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
			if ( !IsValidMotion( focus ) )
			{
				_ASSERT_EXPR( 0, L"Error : The focusing motion is invalid!" );
				return;
			}
			// else

			const auto currentPose = CalcCurrentPose( currentFrame );
			UpdateSkeletal( currentPose );

			CalcLocalMatrix();

			CalcGlobalMatrix();
		}
		void FocusMotion::UpdateCurrentPose( const Animator &animator )
		{
			float currentFrame = animator.CalcCurrentFrame();
			UpdateCurrentPose( currentFrame );
		}

		void FocusMotion::AdaptToFocus()
		{
			const auto   basicKeyFrame	= CalcCurrentPose( 0.0f );
			const size_t boneCount		= basicKeyFrame.keyPose.size();

			if ( !boneCount )
			{
				skeletal.clear();
				return;
			}
			// else

			skeletal.resize( boneCount );
			UpdateSkeletal( basicKeyFrame );
		}

		Animation::KeyFrame FocusMotion::CalcCurrentPose( float currentFrame )
		{
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

			const Animation::KeyFrame currentPose	= focus.keyFrames[baseFrame];
			const Animation::KeyFrame nextPose		= focus.keyFrames[nextFrame];

			// Interpolation is not necessary.
			if ( ZeroEqual( fractional ) ) { return currentPose; }
			// else

			_ASSERT_EXPR( currentPose.keyPose.size() == nextPose.keyPose.size(), L"Error : The bone count did not match! " );

			auto SlerpBone = [&SlerpTransform]( const Animation::Bone &lhs, const Animation::Bone rhs, float percent )
			{
				Animation::Bone rv		= lhs;
				rv.transform			= SlerpTransform( lhs.transform,			rhs.transform,			percent );
				rv.transformToParent	= SlerpTransform( lhs.transformToParent,	rhs.transformToParent,	percent );
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
		}

		void FocusMotion::UpdateSkeletal( const Animation::KeyFrame &keyFrame )
		{
			_ASSERT_EXPR( keyFrame.keyPose.size() == skeletal.size(), L"Error : The bone count of focusing motion is invalid!" );

			const size_t boneCount = skeletal.size();
			for ( size_t i = 0; i < boneCount; ++i )
			{
				skeletal[i].bone = keyFrame.keyPose[i];
			}
		}

		void FocusMotion::CalcLocalMatrix()
		{
			for ( auto &it : skeletal )
			{
				it.local = it.bone.transform.ToWorldMatrix();
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
