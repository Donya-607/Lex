#include "ModelMotion.h"

#include <numeric>			// Use std::accumulate.

#include "Donya/Constant.h"	// Use scast macro.
#include "Donya/Useful.h"	// Use EPSILON constant, and ZeroEqual().

namespace Donya
{
	namespace Model
	{
		namespace
		{
			Animation::KeyFrame	EmptyKeyFrame()
			{
				return Animation::KeyFrame{};
			}
			Animation::Motion	EmptyMotion()
		{
			return Animation::Motion{};
		}

			float CalcAverageStep( const std::vector<Animation::KeyFrame> &motion )
			{
				std::vector<float> steps{};
				steps.resize( motion.size() - 1U );

				const size_t motionCount = motion.size();
				for ( size_t i = 0; i < motionCount - 1; ++i )
				{
					steps.emplace_back( motion[i + 1].seconds - motion[i].seconds );
				}

				return std::accumulate( steps.begin(), steps.end(), 0.0f ) / scast<float>( steps.size() );
			}
			float CalcWholeSeconds( const std::vector<Animation::KeyFrame> &motion )
			{
				// The "seconds" contain the begin seconds(not playing seconds).
				return motion.back().seconds;

				// float sum = 0.0f;
				// for ( const auto &it : motion )
				// {
				// 	sum += it.seconds;
				// }
				// return sum;
			}
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
		bool MotionHolder::IsOutOfRange( size_t motionIndex ) const
		{
			if ( GetMotionCount() <= motionIndex ) { return true; }
			// else

			return false;
		}

		const std::vector<Animation::Motion> &MotionHolder::GetAllMotions() const
		{
			return motions;
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

		void MotionHolder::EraseAllMotions()
		{
			motions.clear();
		}
		void MotionHolder::EraseMotion( int motionIndex )
		{
			if ( IsOutOfRange( motionIndex ) ) { return; }
			// else
			motions.erase( motions.begin() + motionIndex );
		}
		void MotionHolder::EraseMotion( size_t motionIndex )
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
			elapsedTime	= 0.0f;
			wasEnded	= false;
		}
		void  Animator::Update( float argElapsedTime )
		{
			if ( wasEnded && !enableLoop ) { return; }
			// else

			elapsedTime	+= argElapsedTime;
			wasEnded	=  false;

			if ( enableRepeat )
			{
				WrapAround( repeatRangeL, repeatRangeR );
			}
		}

		bool  Animator::WasEnded() const { return wasEnded; }
		bool  Animator::IsOverPlaybackTimeOf( const std::vector<Animation::KeyFrame> &motion ) const
		{
			const float lastTime = CalcWholeSeconds( motion );
			return ( lastTime <= elapsedTime );
		}
		bool  Animator::IsOverPlaybackTimeOf( const Animation::Motion &motion ) const
		{
			return IsOverPlaybackTimeOf( motion.keyFrames );
		}

		Animation::KeyFrame Animator::CalcCurrentPose( const std::vector<Animation::KeyFrame> &motion ) const
		{
			if ( motion.empty()     ) { return Animation::KeyFrame{}; } // Returns empty.
			if ( motion.size() == 1 ) { return motion.front(); }
			// else

			const float wholeSeconds = CalcWholeSeconds( motion );

			auto  CalcCurrentSeconds = [&]()
			{
				float sec =  elapsedTime;
				if ( 0.0f <= sec ) { return sec; } // Positive value is ok.
				// else

				if ( enableRepeat )
				{
					// Consider as now playing to reverse.
					// The seconds to be relative time from last time("repeatRangeR").

					const float distance = repeatRangeR - repeatRangeL;
					sec = fmodf( sec, distance );

					sec = repeatRangeR - fabsf( sec );
				}
				else
				{
					// We can not usable the negative value.
					sec = 0.0f;
				}

				return sec;
			};

			float currentSeconds = CalcCurrentSeconds();
			if (  wholeSeconds  <= currentSeconds )
			{
				if ( !enableLoop ) { return motion.back(); }
				// else

				currentSeconds = fmodf( currentSeconds, wholeSeconds );

				// If you wanna enable the interpolation between last and start.
				// currentSeconds = fmodf( currentSeconds, wholeSeconds + CalcAverageStep( motion ) );
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
					nextLoopFirst.seconds = wholeSeconds + CalcAverageStep( motion );

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

		void  Animator::EnableLoop()
		{
			enableLoop = true;
		}
		void  Animator::DisableLoop()
		{
			enableLoop = false;
		}

		void  Animator::SetRepeatRange( float startTime, float endTime )
		{
			// Requirements.
			assert( startTime < endTime );
			assert( 0.0f <= startTime );
			assert( 0.0f <= endTime );
			assert( !ZeroEqual( startTime - endTime ) ); // Means: startTime != endTime

			enableRepeat = true;
			repeatRangeL = startTime;
			repeatRangeR = endTime;

			// It is not desired to be updated by WrapAround().
			const bool oldLoopFlag = wasEnded;
			WrapAround( repeatRangeL, repeatRangeR );
			wasEnded = oldLoopFlag;

		}
		void  Animator::SetRepeatRange( const std::vector<Animation::KeyFrame> &motion )
		{
			SetRepeatRange( 0.0f, CalcWholeSeconds( motion ) );
		}
		void  Animator::SetRepeatRange( const Animation::Motion &motion )
		{
			SetRepeatRange( motion.keyFrames );
		}
		void  Animator::ResetRepeatRange()
		{
			enableRepeat = false;
			repeatRangeL = 0.0f;
			repeatRangeR = 1.0f;
		}

		void  Animator::SetInternalElapsedTime( float overwrite )
		{
			elapsedTime = overwrite;
		}
		float Animator::GetInternalElapsedTime() const
		{
			return elapsedTime;
		}

		void  Animator::WrapAround( float min, float max )
		{
			const float distance = max - min;
			if ( ZeroEqual( distance ) )
			{
				_ASSERT_EXPR( 0, L"Error : We can not wrap-around within same range." );
				return;
			}
			// else

			if ( enableLoop )
			{
				while ( max < elapsedTime ) { elapsedTime -= distance; wasEnded = true; }
				while ( elapsedTime < min ) { elapsedTime += distance; wasEnded = true; }
			}
			else
			{
				if ( max < elapsedTime ) { elapsedTime = max; wasEnded = true; }
				if ( elapsedTime < min ) { elapsedTime = min; wasEnded = true; }
			}
		}
	}
}
