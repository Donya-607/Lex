#pragma once

#include <string>
#include <vector>

#include "ModelCommon.h"
#include "ModelAnimator.h"

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
			void AppendMotion( const Animation::Motion &element );
		};

		/// <summary>
		/// Provides transform matrix of now focusing motion, that transforms: bone space -> current mesh space.		/// Provides transform matrix of now focusing motion that transforms: bone space -> current mesh space.
		/// </summary>
		class FocusMotion
		{
		public:
			struct Node
			{
				Animation::Bone		bone;	// The source.
				Donya::Vector4x4	local;	// Represents local transform only.
				Donya::Vector4x4	global;	// Contain all parent's global transform. If the root bone, this matrix contains the local transform only.
			};
		private:
			std::vector<Node>	skeletal; // Provides the matrices of the current pose. That transforms space is bone -> mesh.
			Animation::Motion	focus;
		public:
			/// <summary>
			/// Validate the motion has compatible of now focus.<para></para>
			/// You should register the focus motion before call this.
			/// </summary>
			bool IsValidMotion( const Animation::Motion &motion ) const;

			void RegisterMotion( const Animation::Motion &targetMotion );
			Animation::Motion GetFocusingMotion() const;

			const std::vector<Node> &GetCurrentSkeletal() const;
		public:
			void UpdateCurrentPose( float currentFrame );
			void UpdateCurrentPose( const Animator &frameCalculator );
		private:
			Animation::KeyFrame CalcCurrentPose( float currentFrame );

			void AssignPose( const Animation::KeyFrame &assignFrame );

			void CalcLocalMatrix();
			void CalcGlobalMatrix();
		};
	}
}
