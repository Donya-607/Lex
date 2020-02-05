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
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP( position	),
						CEREAL_NVP( normal		),
						CEREAL_NVP( texCoord	),
						CEREAL_NVP( boneWeights	),
						CEREAL_NVP( boneIndices	)
					);
				}
			}
		};

		/// <summary>
		/// The materialIndex is used as an index of an array of whole materials.
		/// </summary>
		struct Subset
		{
			unsigned int	indexCount;
			unsigned int	indexStart;
			unsigned int	materialIndex = -1;	// -1 is invalid. This index link to the vector of ModelSource::materials.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP(	indexCount		),
						CEREAL_NVP(	indexStart		),
						CEREAL_NVP( materialIndex	)
					);
				}
			}
		};

		/// <summary>
		/// The "Node" has many types. The Mesh is one of Node's types, the Bone is also one of the Node's types.
		/// </summary>
		struct Mesh
		{
			int								nodeIndex;		// The index of this mesh's node.
			std::vector<int>				nodeIndices;	// The indices of associated nodes with this mesh and this mesh's node.
			std::vector<Donya::Vector4x4>	boneOffsets;	// The bone-offset(inverse initial-pose) matrices of associated nodes. You can access to that associated nodes with the index of "nodeIndices".

			std::vector<Vertex>				vertices;
			std::vector<int>				indices;		// A index list of vertices.
			std::vector<Subset>				subsets;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP(	nodeIndex	),
						CEREAL_NVP(	nodeIndices	),
						CEREAL_NVP(	boneOffsets	),
						CEREAL_NVP(	vertices	),
						CEREAL_NVP(	indices		),
						CEREAL_NVP(	subsets		)
					);
				}
			}
		};

		struct Material
		{
			Donya::Vector4	color{ 1.0f, 1.0f, 1.0f, 1.0f };	// RGBA.
			std::string		textureName;	// Relative file path. No need to multiple texture.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP(	color		),
						CEREAL_NVP(	textureName	)
					);
				}
			}
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
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP(	parentIndex	),
						CEREAL_NVP(	scale		),
						CEREAL_NVP(	rotation	),
						CEREAL_NVP(	translation	),
						CEREAL_NVP(	name		)
					);
				}
			}
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
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP(	scale		),
						CEREAL_NVP(	rotation	),
						CEREAL_NVP(	translation	)
					);
				}
			}
		};
		/// <summary>
		/// A transforming data of an associated skeletal, at some timing.
		/// </summary>
		struct KeyFrame
		{
			float					seconds;
			std::vector<KeyBone>	keyData;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP(	seconds	),
						CEREAL_NVP(	keyData	)
					);
				}
			}
		};
		/// <summary>
		/// A gathering of an associated key-frame.
		/// </summary>
		struct Animation
		{
			float					animSeconds;
			std::vector<KeyFrame>	keyFrames;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP(	animSeconds	),
						CEREAL_NVP(	keyFrames	)
					);
				}
			}
		};

		std::vector<Mesh>		meshes;
		std::vector<Material>	materials;	// The materials will accessed with Subset::materialIndex.
		std::vector<Bone>		skeletal;	// Represent bones of initial pose(like T-pose).
		std::vector<Animation>	animations;	// Represent animations. The animations contain only animation(i.e. The animation provides a matrix of from mesh space to local(current pose) space).
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP(	meshes		),
						CEREAL_NVP(	materials	),
						CEREAL_NVP(	skeletal	),
						CEREAL_NVP(	animations	)
					);
				}
			}
	};
}

CEREAL_CLASS_VERSION( Donya::ModelSource,				0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Vertex,		0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Subset,		0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Mesh,			0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Material,		0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Bone,			0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::KeyBone,		0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::KeyFrame,		0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Animation,	0 )
