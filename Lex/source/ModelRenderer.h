#pragma once

#include <array>	// Use std::array for bone-transforms.
#include <d3d11.h>

#include "Donya/CBuffer.h"
#include "Donya/Vector.h"

#include "ModelMaker.h"		// Use for ModelUsage.

namespace Donya
{
	template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	namespace Strategy
	{
		/// <summary>
		/// Use for toggle a constant-buffer by the specification of vertex type.<para></para>
		/// But this must update also a member of constants because the actual constant-buffer type is known only this.<para></para>
		/// So this uses also constant-buffer.
		/// </summary>
		class IConstantsPerMesh
		{
		public:
			virtual bool Create() = 0;
			virtual void Activate() = 0;
			virtual void Deactivate() = 0;
		};

		class SkinnedConstantsPerMesh : public IConstantsPerMesh
		{
		public:
			static constexpr unsigned int MAX_BONE_COUNT = 64U;
		private:
			struct Constants
			{
				using BoneMatricesType = std::array<Donya::Vector4x4, MAX_BONE_COUNT>;

				Donya::Vector4x4 adjustMatrix;		// Model space. This matrix contain a global-transform, and coordinate-conversion matrix.
				BoneMatricesType boneTransforms;	// This matrix transforms to world space of game from bone space in initial-pose.
			};
			Donya::CBuffer<Constants> cbuffer;
		public:
			bool Create() override;
			void Activate() override;
			void Deactivate() override;
		};
		class StaticConstantsPerMesh : public IConstantsPerMesh
		{
		private:
			struct Constants
			{
				Donya::Vector4x4 adjustMatrix; // Model space. This matrix contain a global-transform, and coordinate-conversion matrix.
			};
			Donya::CBuffer<Constants> cbuffer;
		public:
			bool Create() override;
			void Activate() override;
			void Deactivate() override;
		};
	}

	/// <summary>
	/// Render a 3D-model.
	/// </summary>
	class ModelRenderer
	{

	};
}
