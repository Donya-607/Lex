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
