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
			void ResetTimer();
			/// <summary>
			/// Update an internal elapsed-timer.
			/// </summary>
			void Update( float elapsedTime );
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
