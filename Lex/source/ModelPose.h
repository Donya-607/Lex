#pragma once

#include <vector>

#include "ModelCommon.h"

namespace Donya
{
	namespace Model
	{
		/// <summary>
		/// This class represents a skeletal, and this can update and provide a transform matrices of a skeletal. That matrix transforms space is bone space -> current mesh space.
		/// </summary>
		class Pose
		{
		private:
			std::vector<Animation::Node> skeletal;	// Provides the matrices of the current pose. That transforms space is bone -> mesh.
		public:
			const std::vector<Animation::Node> &GetCurrentPose() const;

			/// <summary>
			/// The "compatible" means the argument is associate with internal skeletal.
			/// e.g. the skeletal belong in the same motion, but another timing.
			/// </summary>
			bool HasCompatibleWith( const std::vector<Animation::Node> &validation ) const;
			/// <summary>
			/// The "compatible" means the argument is associate with internal skeletal.
			/// e.g. the skeletal belong in the same motion, but another timing.
			/// </summary>
			bool HasCompatibleWith( const Animation::KeyFrame &validation ) const;
		public:
			/// <summary>
			/// Assign the skeletal by the argument.
			/// </summary>
			void AssignSkeletal( const std::vector<Animation::Node> &newSkeletal );
			/// <summary>
			/// Assign the skeletal by key-pose of the argument.
			/// </summary>
			void AssignSkeletal( const Animation::KeyFrame &newSkeletal );
		public:
			/// <summary>
			/// Calculate the transform matrix of each node of internal skeletal. So it is heavy,
			/// </summary>
			void UpdateTransformMatrices();
		private:
			void UpdateLocalMatrices();
			void UpdateGlobalMatrices();
		};
	}
}
