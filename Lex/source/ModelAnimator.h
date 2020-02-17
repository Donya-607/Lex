#pragma once

#include <vector>

#include "ModelCommon.h"
#include "ModelSource.h"

namespace Donya
{
	namespace Model
	{
		/// <summary>
		/// This class's role is calculation a motion frame.
		/// </summary>
		class Animator
		{
		private:
			float	elapsedTime			= 0.0f;
			bool	enableWrapAround	= true;
		public:
			/// <summary>
			/// Set zero to internal elapsed-timer.
			/// </summary>
			void Reset();
			/// <summary>
			/// Update an internal elapsed-timer.
			/// </summary>
			void Update( float elapsedTime );
		public:
			/// <summary>
			/// Set some motion frame to internal elapsed-timer. That calculated in this way:"frame * ( 1.0f / FPS )".
			/// </summary>
			void AssignFrame( float frame, float FPS = 1.0f / Animation::Motion::DEFAULT_SAMPLING_RATE );
			/// <summary>
			/// Calculate current motion frame by internal elapsed-timer. That calculated in this way:"internal-elapsed-timer / ( 1.0f / FPS )".
			/// </summary>
			float CalcCurrentFrame( float FPS = 1.0f / Animation::Motion::DEFAULT_SAMPLING_RATE ) const;
			/// <summary>
			/// Calculate current motion frame within the range [minFrame ~ maxFrame] by internal-timer.
			/// </summary>
			float CalcCurrentFrame( float FPS, float minFrame, float maxFrame ) const;
			/// <summary>
			/// Calculate current motion frame within the range [0.0f ~ asFrameRange.size()] by internal-timer.
			/// </summary>
			float CalcCurrentFrame( float FPS, const std::vector<Animation::KeyFrame> &asFrameRange ) const;
		public:
			/// <summary>
			/// The calculate method returns frame will be wrap-around values within some range.
			/// </summary>
			void EnableWrapAround();
			/// <summary>
			/// The calculate method returns frame will be clamped within some range.
			/// </summary>
			void DisableWrapAround();
		public:
			/// <summary>
			/// Overwrite an internal timer that updating at Update(). This does not represent a current frame.
			/// </summary>
			void SetInternalElapsedTime( float overwrite );
			/// <summary>
			/// Returns an internal timer that updating at Update(). This does not represent a current frame.
			/// </summary>
			float GetInternalElapsedTime() const;
		};

		/// <summary>
		/// Returns bone-matrix(interpolated by slerp) at current frame.
		/// </summary>
		Animation::KeyFrame CalcCurrentPose( const std::vector<Animation::KeyFrame> &motion, float currentFrame, bool enableLoop = true );
		/// <summary>
		/// Returns bone-matrix(interpolated by slerp) at current frame.
		/// </summary>
		Animation::KeyFrame CalcCurrentPose( const std::vector<Animation::KeyFrame> &motion, const Animator &currentFrameCalculator, float FPS = 1.0f / Animation::Motion::DEFAULT_SAMPLING_RATE );
	}
}
