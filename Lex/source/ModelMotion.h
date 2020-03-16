#pragma once

#include <string>
#include <vector>

#include "ModelCommon.h"
#include "ModelSource.h"

namespace Donya
{
	namespace Model
	{
		/// <summary>
		/// The storage of some motions.
		/// </summary>
		class MotionHolder
		{
		private:
			std::vector<Animation::Motion> motions;
		public:
			size_t GetMotionCount() const;
			/// <summary>
			/// return ( motionIndex &lt; 0 || GetMotionCount() &lt;= motionIndex );
			/// </summary>
			bool IsOutOfRange( int motionIndex ) const;
		public:
			/// <summary>
			/// Returns the motion of specified element, or empty if the index is invalid.
			/// </summary>
			Animation::Motion GetMotion( int motionIndex ) const;
			/// <summary>
			/// Returns the specified motion that found first, or empty if the specified name is invalid.
			/// </summary>
			Animation::Motion FindMotion( const std::string &motionName ) const;
		public:
			/// <summary>
			/// Append all motions that the source has. The consistency with internal motion is not considered.
			/// </summary>
			void AppendSource( const ModelSource &source );
			/// <summary>
			/// The consistency with internal motion is not considered.
			/// </summary>
			void AppendMotion( const Animation::Motion &element );
		};

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
			Animation::KeyFrame CalcCurrentPose( const std::vector<Animation::KeyFrame> &motion ) const;
			Animation::KeyFrame CalcCurrentPose( const Animation::Motion &motion ) const;
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
