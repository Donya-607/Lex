#pragma once

#include "Donya/Vector.h"

namespace Donya
{
	/// <summary>
	/// The specification of a type of vertex. Static or Skinned.
	/// </summary>
	enum class ModelUsage
	{
		Static,		// You can only use the static version.
		Skinned,	// You can only use the skinning version.

		// Will be implemented.
		// Dynamic, // User can choose the usage at use.
	};

	namespace Vertex
	{
		struct Pos
		{
			Donya::Vector3	position;
			Donya::Vector3	normal;
		};
		struct Tex
		{
			Donya::Vector2	texCoord;
		};
		struct Bone
		{
			Donya::Vector4 	weights;
			Donya::Int4		indices;
		};
	}
}
