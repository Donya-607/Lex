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
		void  Animator::ResetTimer()
		{
			elapsedTime = 0.0f;
		}
		void  Animator::Update( float argElapsedTime )
		{
			elapsedTime += argElapsedTime;
		}

		void  Animator::AssignFrame( float frame )
		{
			elapsedTime = frame * FPSToRate( FPS );
		}
		float Animator::CalcCurrentFrame() const
		{
			return elapsedTime / FPSToRate( FPS );
		}
		float Animator::CalcCurrentFrame( float minFrame, float maxFrame ) const
		{
			const float frame = CalcCurrentFrame();

			return HandleFrameImpl( frame, minFrame, maxFrame, enableWrapAround );
		}
		float Animator::CalcCurrentFrame( const std::vector<Animation::KeyFrame> &asFrameRange ) const
		{
			const size_t frameCount = asFrameRange.size();
			if ( !frameCount || frameCount == 1 )
			{
				return 0.0f;
			}
			// else
			return CalcCurrentFrame( 0.0f, scast<float>( frameCount ) );
		}

		Animation::KeyFrame Animator::CalcCurrentPose( const std::vector<Animation::KeyFrame> &motion ) const
		{
			if ( motion.empty()     ) { return Animation::KeyFrame{}; } // Returns empty.
			if ( motion.size() == 1 ) { return motion.front(); }
			// else

			auto CalcWholeSeconds = []( const std::vector<Animation::KeyFrame> &motion )
			{
				// The "seconds" contain the begin seconds(not playing seconds).
				return motion.back().seconds;
				/*
				float sum = 0.0f;
				for ( const auto &it : motion )
				{
					sum += it.seconds;
				}
				return sum;
				*/
			};
			const float wholeSeconds = CalcWholeSeconds( motion );

			float currentSeconds = elapsedTime;
			if ( wholeSeconds <= currentSeconds )
			{
				if ( !enableWrapAround ) { return motion.back(); }
				// else

				currentSeconds = fmodf( currentSeconds, wholeSeconds );
			}

			const size_t motionCount = motion.size();
			for ( size_t i = 0; i < motionCount - 1; ++i )
			{
				const auto &keyFrameL = motion[i];
				const auto &keyFrameR = motion[i + 1];
				if ( currentSeconds < keyFrameL.seconds || keyFrameR.seconds <= currentSeconds ) { continue; }
				// else

				const float diffL = currentSeconds    - keyFrameL.seconds;
				const float diffR = keyFrameR.seconds - keyFrameL.seconds;
				const float percent = diffL / ( diffR + EPSILON );

			}
		}
		Animation::KeyFrame Animator::CalcCurrentPose( const Animation::Motion &motion ) const
		{
			return CalcCurrentPose( motion.keyFrames );
		}

		void  Animator::EnableWrapAround()
		{
			enableWrapAround = true;
		}
		void  Animator::DisableWrapAround()
		{
			enableWrapAround = false;
		}

		void  Animator::SetFPS( float overwrite )
		{
			FPS = overwrite;
			FPS = std::max( 1.0f, FPS );
		}

		void  Animator::SetInternalElapsedTime( float overwrite )
		{
			elapsedTime = overwrite;
		}
		float Animator::GetInternalElapsedTime() const
		{
			return elapsedTime;
		}
	}
}
