#include "ModelMotion.h"

#include "Donya/Constant.h"	// Use scast macro.
#include "Donya/Useful.h"	// Use EPSILON constant.

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

		void MotionHolder::EraseMotion( int motionIndex )
		{
			if ( IsOutOfRange( motionIndex ) ) { return; }
			// else
			motions.erase( motions.begin() + motionIndex );
		}
		void MotionHolder::EraseMotion( const std::string &motionName )
		{
			for ( auto &it = motions.begin(); it != motions.end(); ++it )
			{
				if ( it->name == motionName )
				{
					// Erases only once.
					motions.erase( it );
					return;
				}
			}
		}

		void MotionHolder::AppendSource( const Source &source )
		{
			for ( const auto &it : source.motions )
			{
				AppendMotion( it );
			}
		}
		void MotionHolder::AppendMotion( const Animation::Motion &element )
		{
			motions.emplace_back( element );
		}



		void  Animator::ResetTimer()
		{
			elapsedTime = 0.0f;
		}
		void  Animator::Update( float argElapsedTime )
		{
			elapsedTime += argElapsedTime;
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

			Animation::KeyFrame rv;

			const size_t motionCount = motion.size();
			for ( size_t i = 0; i < motionCount - 1; ++i )
			{
				const auto &keyFrameL = motion[i];
				const auto &keyFrameR = motion[i + 1];
				if ( currentSeconds < keyFrameL.seconds || keyFrameR.seconds <= currentSeconds ) { continue; }
				// else

				const float diffL = currentSeconds    - keyFrameL.seconds;
				const float diffR = keyFrameR.seconds - keyFrameL.seconds;
				const float percent = diffL / ( diffR + EPSILON/* Prevent zero-divide */ );
				
				rv = Animation::KeyFrame::Interpolate( keyFrameL, keyFrameR, percent );

				break;
			}

			return rv;
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
