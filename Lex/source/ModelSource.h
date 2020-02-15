#pragma once

#include <string>
#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "ModelCommon.h"

namespace Donya
{
	/// <summary>
	/// Store a data of model as it is.
	/// </summary>
	struct ModelSource
	{
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

		struct Subset
		{
			std::string		name;
			unsigned int	indexCount;
			unsigned int	indexStart;
			Material		ambient;
			Material		bump;
			Material		diffuse;
			Material		specular;
			Material		emissive;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP( name		).
						CEREAL_NVP(	indexCount	),
						CEREAL_NVP(	indexStart	),
						CEREAL_NVP( ambient		),
						CEREAL_NVP( bump		),
						CEREAL_NVP( diffuse		),
						CEREAL_NVP( specular	),
						CEREAL_NVP( emissive	)
					);
				}
			}
		};

		/// <summary>
		/// The "Node" has many types. The Mesh is one of Node's types, the Bone is also one of the Node's types.
		/// </summary>
		struct Mesh
		{
			std::string						name;

			Donya::Vector4x4				coordinateConversion;
			Donya::Vector4x4				globalTransform;

			int								nodeIndex;		// The index of this mesh's node.
			std::vector<int>				nodeIndices;	// The indices of associated nodes with this mesh and this mesh's node.
			std::vector<Donya::Vector4x4>	boneOffsets;	// The bone-offset(inverse initial-pose) matrices of associated nodes. You can access to that associated nodes with the index of "nodeIndices".

			std::vector<Vertex::Pos>		positions;
			std::vector<Vertex::Tex>		texCoords;
			std::vector<Vertex::Bone>		boneInfluences;
			std::vector<unsigned int>		indices;		// A index list of vertices.
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
						CEREAL_NVP(	name			),
						CEREAL_NVP(	coordinateConversion	),
						CEREAL_NVP(	globalTransform			),
						CEREAL_NVP(	nodeIndex		),
						CEREAL_NVP(	nodeIndices		),
						CEREAL_NVP(	boneOffsets		),
						CEREAL_NVP(	positions		),
						CEREAL_NVP(	texCoords		),
						CEREAL_NVP(	boneInfluences	),
						CEREAL_NVP(	indices			),
						CEREAL_NVP(	subsets			)
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
			std::string			name;
			int					parentIndex = -1; // -1 is invalid.
			Donya::Vector3		scale{ 1.0f, 1.0f, 1.0f };
			Donya::Quaternion	rotation;
			Donya::Vector3		translation;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				if ( version == 0 )
				{
					archive
					(
						CEREAL_NVP(	name		),
						CEREAL_NVP(	parentIndex	),
						CEREAL_NVP(	scale		),
						CEREAL_NVP(	rotation	),
						CEREAL_NVP(	translation	)
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
			static constexpr float	DEFAULT_SAMPLING_RATE = 1.0f / 24.0f;
		public:
			std::string				name;
			float					samplingRate{ DEFAULT_SAMPLING_RATE };
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
						CEREAL_NVP(	name			),
						CEREAL_NVP(	samplingRate	),
						CEREAL_NVP(	animSeconds		),
						CEREAL_NVP(	animSeconds		),
						CEREAL_NVP(	keyFrames		)
					);
				}
			}
		};

		std::vector<Mesh>		meshes;
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
					CEREAL_NVP(	skeletal	),
					CEREAL_NVP(	animations	)
				);
			}
		}
	};
}

CEREAL_CLASS_VERSION( Donya::ModelSource,				0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Subset,		0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Mesh,			0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Material,		0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Bone,			0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::KeyBone,		0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::KeyFrame,		0 )
CEREAL_CLASS_VERSION( Donya::ModelSource::Animation,	0 )
