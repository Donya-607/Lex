#pragma once

namespace Donya
{
	/// <summary>
	/// The specification of a type of vertex. Static or Skinned.
	/// </summary>
	enum class ModelUsage
	{
		Static,		// Not use skinning.
		Skinned,	// Use skinning.
		// Dynamic, Will be implemented. // User can choose the usage at use.
	};

}
