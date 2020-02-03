#pragma once

#include <string>
#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

namespace Donya
{
	/// <summary>
	/// Store a data of model as it is.
	/// </summary>
	struct ModelSource
	{
		/// <summary>
		/// The type to support skinning.
		/// </summary>
		struct Vertex
		{
			Donya::Vector3	position;
			Donya::Vector3	normal;
			Donya::Vector2	texCoord;
			Donya::Vector4	boneWeights; // Each element is used as like array(e.g. x:[0], y:[1], ...).
			Donya::Int4		boneIndices; // Each element is used as like array(e.g. x:[0], y:[1], ...).
		};

		/// <summary>
		/// The materialIndex is used as an index of an array of whole materials.
		/// </summary>
		struct Subset
		{
			unsigned int	indexCount;
			unsigned int	indexStart;
			unsigned int	materialIndex = -1;	// -1 is invalid. This index link to the vector of ModelSource::materials.
		};

		/// <summary>
		/// The "Node" has many types. The Mesh is one of Node's types, the Bone is also one of the Node's types.
		/// </summary>
		struct Mesh
		{
			int						nodeIndex;		// The index of this mesh's node.
			std::vector<int>		nodeIndices;	// The indices of associated nodes with this mesh and this mesh's node.

			std::vector<Vertex>		vertices;
			std::vector<int>		indices;		// A index list of vertices.
			std::vector<Subset>		subsets;
		};

		struct Material
		{
			Donya::Vector4	color{ 1.0f, 1.0f, 1.0f, 1.0f };	// RGBA.
			std::string		texturePath;	// Relative. No need to multiple texture.
		};

		/// <summary>
		/// Represent a Bone(you may call it Rig or Node).<para></para>
		/// The stored transforming data are local space(Calculated in: ParentGlobal.Inverse * Global).
		/// </summary>
		struct Bone
		{
			int					parentIndex = -1; // -1 is invalid.
			Donya::Vector3		scale{ 1.0f, 1.0f, 1.0f };
			Donya::Quaternion	rotation;
			Donya::Vector3		translation;
			std::string			name;
		};

		/// <summary>
		/// A transforming data of an associated bone, at some timing.<para></para>
		/// The stored transforming data are local space(Calculated in: ParentGlobal.Inverse * Global).
		/// </summary>
		struct KeyBone
		{
			Donya::Vector3			scale{ 1.0f, 1.0f, 1.0f };
			Donya::Quaternion		rotation;
			Donya::Vector3			translation;
		};
		/// <summary>
		/// A transforming data of an associated skeletal, at some timing.
		/// </summary>
		struct KeyFrame
		{
			float					seconds;
			std::vector<KeyBone>	keyData;
		};
		/// <summary>
		/// A gathering of an associated key-frame.
		/// </summary>
		struct Animation
		{
			float					animSeconds;
			std::vector<KeyFrame>	keyFrames;
		};

		std::vector<Mesh>		meshes;
		std::vector<Material>	materials;	// The materials will accessed with Subset::materialIndex.
		std::vector<Bone>		skeletal;	// Represent bones of initial pose(like T-pose).
		std::vector<Animation>	animations;	// Represent animations. The animations contain only animation(i.e. The animation provides a matrix of from mesh space to local(current pose) space).
	};
}
