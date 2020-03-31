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
			/// Returns the motion of specified element.
			/// </summary>
			const Animation::Motion &GetMotion( int motionIndex ) const;
			/// <summary>
			/// Returns the specified motion that found first, or end(== GetMotionCount()) if the specified name is invalid.
			/// </summary>
			size_t FindMotionIndex( const std::string &motionName ) const;
		public:
			/// <summary>
			/// Erase a motion by index of array.
			/// </summary>
			void EraseMotion( int motionIndex );
			/// <summary>
			/// Erase a motion that found first by name. This method erases only once even if I contain multiple names.
			/// </summary>
			void EraseMotion( const std::string &motionName );
		public:
			/// <summary>
			/// Append all motions that the source has. The consistency with internal motion is not considered.
			/// </summary>
			void AppendSource( const Source &source );
			/// <summary>
			/// The consistency with internal motion is not considered.
			/// </summary>
			void AppendMotion( const Animation::Motion &element );
		};

		/// <summary>
		/// This class's role is calculation a motion frame.<para></para>
		/// This class does not linking to some motion, so if you wanna know the end timing of playback, you should set the play seconds range of a motion.
		/// </summary>
		class Animator
		{
		private:
			float	elapsedTime		= 0.0f;
			float	repeatRangeL	= 0.0f;
			float	repeatRangeR	= 1.0f;
			bool	enableRepeat	= false;
			bool	enableLoop		= true;
			bool	wasEnded		= false;
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
			/// Returns true if the current time was over the repeat range.<para></para>
			/// Returns false when the repeat is not enable.
			/// </summary>
			bool WasEnded() const;
			/// <summary>
			/// Returns true if the current time is greater equal than the motion's last time(the repeat range will be ignored).
			/// </summary>
			bool IsOverPlaybackTimeOf( const std::vector<Animation::KeyFrame> &motion ) const;
			/// <summary>
			/// Returns true if the current time is greater equal than the motion's last time(the repeat range will be ignored).
			/// </summary>
			bool IsOverPlaybackTimeOf( const Animation::Motion &motion ) const;
		public:
			Animation::KeyFrame CalcCurrentPose( const std::vector<Animation::KeyFrame> &motion ) const;
			Animation::KeyFrame CalcCurrentPose( const Animation::Motion &motion ) const;
		public:
			/// <summary>
			/// If the current time is over some range, the current time will back to a start of some range.
			/// </summary>
			void EnableLoop();
			/// <summary>
			/// If the current time is over some range, the current time will be a last of some range.
			/// </summary>
			void DisableLoop();
		public:
			/// <summary>
			/// Requirements:<para></para>
			/// 1: startTime &lt; endTime	<para></para>
			/// 2: 0.0f &lt;= startTime		<para></para>
			/// 3: 0.0f &lt;= endTime		<para></para>
			/// 4: startTime != endTime
			/// </summary>
			void SetRepeatRange( float startTime, float endTime );
			/// <summary>
			/// Set the motion's frame range to repeat range.
			/// </summary>
			void SetRepeatRange( const std::vector<Animation::KeyFrame> &motion );
			/// <summary>
			/// Set the motion's frame range to repeat range.
			/// </summary>
			void SetRepeatRange( const Animation::Motion &motion );
			/// <summary>
			/// Disable the repeat range.
			/// </summary>
			void ResetRepeatRange();
		public:
			/// <summary>
			/// Overwrite an internal timer that updating at Update(). This does not represent a current frame.
			/// </summary>
			void SetInternalElapsedTime( float overwrite );
			/// <summary>
			/// Returns an internal timer that updating at Update(). This does not represent a current frame.
			/// </summary>
			float GetInternalElapsedTime() const;
		private:
			void WrapAround( float minimum, float maximum );
		};
	}
}
