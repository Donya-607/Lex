#include "ModelMotion.h"

#include <numeric>			// Use std::accumulate.

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

		const Animation::Motion &MotionHolder::GetMotion( int motionIndex ) const
		{
			_ASSERT_EXPR( motionIndex < scast<int>( motions.size() ), L"Error : Passed index out of range!" );
			return motions[motionIndex];
		}
		size_t MotionHolder::FindMotionIndex( const std::string &motionName ) const
		{
			const size_t motionCount = motions.size();
			for ( size_t i = 0; i < motionCount; ++i )
			{
				if ( motions[i].name == motionName )
				{
					return i;
				}
			}

			return motionCount;
		}

		void MotionHolder::EraseMotion( int motionIndex )
		{
			if ( IsOutOfRange( motionIndex ) ) { return; }
			// else
			motions.erase( motions.begin() + motionIndex );
		}
		void MotionHolder::EraseMotion( const std::string &motionName )
		{
			for ( auto &&it = motions.begin(); it != motions.end(); ++it )
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

			auto CalcAverageStep		= []( const std::vector<Animation::KeyFrame> &motion )
			{
				std::vector<float> steps{};
				steps.resize( motion.size() - 1U );

				const size_t motionCount = motion.size();
				for ( size_t i = 0; i < motionCount - 1; ++i )
				{
					steps.emplace_back( motion[i + 1].seconds - motion[i].seconds );
				}

				return std::accumulate( steps.begin(), steps.end(), 0.0f ) / scast<float>( steps.size() );
			};
			const float secondStep		= CalcAverageStep( motion );

			auto CalcWholeSeconds		= []( const std::vector<Animation::KeyFrame> &motion )
			{
				// The "seconds" contain the begin seconds(not playing seconds).
				return motion.back().seconds;

				// float sum = 0.0f;
				// for ( const auto &it : motion )
				// {
				// 	sum += it.seconds;
				// }
				// return sum;
			};
			const float wholeSeconds	= CalcWholeSeconds( motion );

			float currentSeconds = elapsedTime;
			if ( wholeSeconds <= currentSeconds )
			{
				if ( !enableWrapAround ) { return motion.back(); }
				// else

				currentSeconds = fmodf( currentSeconds, wholeSeconds );

				// If you wanna enable the interpolation between last and start.
				// currentSeconds = fmodf( currentSeconds, wholeSeconds + secondStep );
			}

			Animation::KeyFrame keyFrameL{}; // Current.
			Animation::KeyFrame keyFrameR{}; // Next.

			// Find the current key-frame and next key-frame.
			{
				const size_t motionCount = motion.size();
				for ( size_t i = 0; i < motionCount - 1; ++i )
				{
					const auto &L = motion[i];
					const auto &R = motion[i + 1];
					if ( currentSeconds < L.seconds || R.seconds <= currentSeconds ) { continue; }
					// else

					assert( !L.keyPose.empty() && !R.keyPose.empty() );

					keyFrameL = L;
					keyFrameR = R;
					break;
				}

				// When the currentSeconds is greater than wholeSeconds.
				if ( keyFrameL.keyPose.empty() || keyFrameR.keyPose.empty() )
				{
					Animation::KeyFrame nextLoopFirst = motion.front();
					nextLoopFirst.seconds = wholeSeconds + secondStep;

					keyFrameL = motion.back();
					keyFrameR = std::move( nextLoopFirst );
				}
			}

			const float diffL	= currentSeconds    - keyFrameL.seconds;
			const float diffR	= keyFrameR.seconds - keyFrameL.seconds;
			const float percent	= diffL / ( diffR + EPSILON /* Prevent zero-divide */ );

			return Animation::KeyFrame::Interpolate( keyFrameL, keyFrameR, percent );
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
