#pragma once

#include <vector>

#include "Vector.h"

namespace Donya
{
	/// <summary>
	/// Rig. Controller.
	/// </summary>
	struct Bone
	{
		Donya::Vector4x4 transform{};
	};
	/// <summary>
	/// Gathering of bones(call "skeletal"). This represents a pose.
	/// </summary>
	struct Skeletal
	{
		std::vector<Bone> skeletal{};
	};
	/// <summary>
	/// Gathering of skeletals(call "Motion"). This represents a motion(animation).
	/// </summary>
	class Motion
	{
		static constexpr float DEFAULT_SAMPLING_RATE = 1.0f / 24.0f;
	public:
		int		meshNo{};	// 0-based.
		float	samplingRate{ DEFAULT_SAMPLING_RATE };
		std::vector<Skeletal> motion{};
	};
}
