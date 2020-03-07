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
			float	FPS					= 1.0f / Animation::Motion::DEFAULT_SAMPLING_RATE;
			bool	enableWrapAround	= true;
		public:
			/// <summary>
			/// Set zero to internal elapsed-timer.
			/// </summary>
			void ResetTimer();
			/// <summary>
			/// Update an internal elapsed-timer.
			/// </summary>
			void Update( float elapsedTime );
		public:
			/// <summary>
			/// Set some motion frame to internal elapsed-timer. That calculated in this way:"frame * ( 1.0f / FPS )".
			/// </summary>
			void AssignFrame( float frame );
			/// <summary>
			/// Calculate current motion frame by internal elapsed-timer. That calculated in this way:"internal-elapsed-timer / ( 1.0f / FPS )".
			/// </summary>
			float CalcCurrentFrame() const;
			/// <summary>
			/// Calculate current motion frame within the range [minFrame ~ maxFrame] by internal-timer.
			/// </summary>
			float CalcCurrentFrame( float minFrame, float maxFrame ) const;
			/// <summary>
			/// Calculate current motion frame within the range [0.0f ~ asFrameRange.size()] by internal-timer.
			/// </summary>
			float CalcCurrentFrame( const std::vector<Animation::KeyFrame> &asFrameRange ) const;
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
			/// The method of frame calculation will use this FPS. The lower limit of FPS is 1.0f.
			/// </summary>
			void SetFPS( float overwrite );
			/// <summary>
			/// Overwrite an internal timer that updating at Update(). This does not represent a current frame.
			/// </summary>
			void SetInternalElapsedTime( float overwrite );
			/// <summary>
			/// Returns an internal timer that updating at Update(). This does not represent a current frame.
			/// </summary>
			float GetInternalElapsedTime() const;
		};
	}
}
