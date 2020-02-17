#include "ModelAnimator.h"

#include "Donya/Constant.h"	// Use scast macro.
#include "Donya/Useful.h"	// Use ZeroEqual().

namespace
{
	static constexpr float FPSToRate( float FPS )
	{
		return 1.0f / FPS;
	}
	static float ClampFrame		( float frame, float minFrame, float maxFrame )
	{
		return std::max( minFrame, std::min( maxFrame, frame ) );
	}
	static float WrapAroundFrame( float frame, float minFrame, float maxFrame )
	{
		const float diff = maxFrame - minFrame;
		if ( ZeroEqual( diff ) ) { return 0.0f; }
		// else
		return minFrame + fmodf( frame, diff );
	}
	static float HandleFrameImpl( float frame, float minFrame, float maxFrame, bool enableWrapAround )
	{
		return	( enableWrapAround )
				? WrapAroundFrame	( frame, minFrame, maxFrame )
				: ClampFrame		( frame, minFrame, maxFrame );
	}
}

namespace Donya
{
	namespace Model
	{
		void  Animator::Reset()
		{
			elapsedTime = 0.0f;
		}
		void  Animator::Update( float argElapsedTime )
		{
			elapsedTime += argElapsedTime;
		}

		void  Animator::AssignFrame( float frame, float FPS )
		{
			elapsedTime = frame * FPSToRate( FPS );
		}
		float Animator::CalcCurrentFrame( float FPS ) const
		{
			return elapsedTime / FPSToRate( FPS );
		}
		float Animator::CalcCurrentFrame( float FPS, float minFrame, float maxFrame ) const
		{
			const float frame = CalcCurrentFrame( FPS );

			return HandleFrameImpl( frame, minFrame, maxFrame, enableWrapAround );
		}
		float Animator::CalcCurrentFrame( float FPS, const std::vector<Animation::KeyFrame> &asFrameRange ) const
		{
			const size_t frameCount = asFrameRange.size();
			if ( !frameCount || frameCount == 1 )
			{
				return 0.0f;
			}
			// else
			return CalcCurrentFrame( FPS, 0.0f, scast<float>( frameCount ) );
		}

		void  Animator::EnableWrapAround()
		{
			enableWrapAround = true;
		}
		void  Animator::DisableWrapAround()
		{
			enableWrapAround = false;
		}

		void  Animator::SetInternalElapsedTime( float overwrite )
		{
			elapsedTime = overwrite;
		}
		float Animator::GetInternalElapsedTime() const
		{
			return elapsedTime;
		}

		Animation::KeyFrame CalcCurrentPoseImpl( const std::vector<Animation::KeyFrame> &motion, float currentFrame )
		{
			float  integral{};
			float  fractional = modf( currentFrame, &integral );	// Will using as percent of interpolation.
			size_t baseFrame  = scast<size_t>( integral );
			size_t nextFrame  = ( motion.size() <= baseFrame + 1 )
								? ( baseFrame + 1 ) % motion.size()	// Wrap around.
								: baseFrame + 1;

			const Animation::KeyFrame currentPose	= motion[baseFrame];
			const Animation::KeyFrame nextPose		= motion[nextFrame];

			// Calculation is not necessary.
			if ( ZeroEqual( fractional ) ) { return currentPose; }
			// else

			_ASSERT_EXPR( currentPose.keyPose.size() == nextPose.keyPose.size(), L"Error : The bone count did not match! " );

			auto SlerpBone = []( const Animation::Bone &lhs, const Animation::Bone rhs, float percent )
			{
				Animation::Bone rv = lhs;
				rv.scale		= Donya::Lerp( lhs.scale,		rhs.scale,			percent );
				rv.rotation		= Donya::Quaternion::Slerp( lhs.rotation, rhs.rotation, percent );
				rv.translation	= Donya::Lerp( lhs.translation,	rhs.translation,	percent );
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
		Animation::KeyFrame CalcCurrentPose( const std::vector<Animation::KeyFrame> &motion, float currentFrame, bool enableLoop )
		{
			float  computedFrame = HandleFrameImpl( currentFrame, 0.0f, scast<float>( motion.size() ), enableLoop );
			return CalcCurrentPoseImpl( motion, computedFrame );
		}
		Animation::KeyFrame CalcCurrentPose( const std::vector<Animation::KeyFrame> &motion, const Animator &animator, float FPS )
		{
			float  computedFrame = animator.CalcCurrentFrame( FPS, motion );
			return CalcCurrentPoseImpl( motion, computedFrame );
		}
	}
}
