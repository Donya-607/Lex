#pragma once

// "ModelCommon.h" provides mainly a shared structures.

#include <array>
#include <d3d11.h>				// Use for implement the method of returns input-element-descs on each vertex struct.
#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"	// Use for impl a serialize method.
#include "Donya/Vector.h"

namespace Donya
{
	namespace Model
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

		/// <summary>
		/// The vertex structures by model type.
		/// </summary>
		namespace Vertex
		{
			struct Pos
			{
				Donya::Vector3	position;
				Donya::Vector3	normal;
			public:
				static constexpr const auto GenerateInputElements( UINT inputSlot )
				{
					return std::array<D3D11_INPUT_ELEMENT_DESC, 2>
					{
						D3D11_INPUT_ELEMENT_DESC{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32_FLOAT,	inputSlot, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
						D3D11_INPUT_ELEMENT_DESC{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32_FLOAT,	inputSlot, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
					};
				}
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					if ( version == 0 )
					{
						archive
						(
							CEREAL_NVP( position ),
							CEREAL_NVP( normal )
						);
					}
				}
			};

			struct Tex
			{
				Donya::Vector2	texCoord; // Origin is left-top.
			public:
				static constexpr const auto GenerateInputElements( UINT inputSlot )
				{
					return std::array<D3D11_INPUT_ELEMENT_DESC, 1>
					{
						D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32_FLOAT,	inputSlot, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
					};
				}
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					if ( version == 0 )
					{
						archive
						(
							CEREAL_NVP( texCoord )
						);
					}
				}
			};

			struct Bone
			{
				Donya::Vector4 	weights; // Each element is used as like array(e.g. x:[0], y:[1], ...).
				Donya::Int4		indices; // Each element is used as like array(e.g. x:[0], y:[1], ...).
			public:
				static constexpr const auto GenerateInputElements( UINT inputSlot )
				{
					return std::array<D3D11_INPUT_ELEMENT_DESC, 2>
					{
						D3D11_INPUT_ELEMENT_DESC{ "WEIGHTS"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	inputSlot, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
						D3D11_INPUT_ELEMENT_DESC{ "BONES"		, 0, DXGI_FORMAT_R32G32B32A32_UINT,		inputSlot, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
					};
				}
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					if ( version == 0 )
					{
						archive
						(
							CEREAL_NVP( weights ),
							CEREAL_NVP( indices )
						);
					}
				}
			};
		}

		/// <summary>
		/// The structures related to animation.
		/// </summary>
		namespace Animation
		{
			struct Transform
			{
				Donya::Vector3		scale{ 1.0f, 1.0f, 1.0f };
				Donya::Quaternion	rotation;
				Donya::Vector3		translation;
			public:
				Donya::Vector4x4 ToWorldMatrix() const;
			public:
				static Transform Identity()
				{
					return Transform{};
				}
				static Transform Interpolate( const Transform &lhs, const Transform &rhs, float time );
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					archive
					(
						CEREAL_NVP( scale ),
						CEREAL_NVP( rotation ),
						CEREAL_NVP( translation )
					);
					
					if ( 1 <= version )
					{
						// archive( CEREAL_NVP(  ) );
					}
				}
			};

			/// <summary>
			/// Represent a transforming data of an associated bone(you may call it Rig or Node), at some timing.
			/// </summary>
			struct Bone
			{
				std::string		name;
				std::string		parentName;			// This will be "" if myself has not parent.
				int				parentIndex = -1;	// This will be -1 if myself has not parent.
				Transform		transform;			// Local transform(bone -> mesh).
				Transform		transformToParent;	// Transform to parent bone space. This will be identity if myself has not parent.
			public:
				/// <summary>
				/// Interpolates the Transform members only. The other members will be "lhs".
				/// </summary>
				static Bone Interpolate( const Bone &lhs, const Bone &rhs, float time );
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					archive
					(
						CEREAL_NVP(	name				),
						CEREAL_NVP(	parentName			),
						CEREAL_NVP(	parentIndex			),
						CEREAL_NVP( transform			),
						CEREAL_NVP( transformToParent	)
					);

					if ( 1 <= version )
					{
						// archive( CEREAL_NVP(  ) );
					}
				}
			};
			/// <summary>
			/// The class used for hold the bone's transform matrix.
			/// </summary>
			struct Node
			{
				Animation::Bone		bone;		// The source.
				Donya::Vector4x4	local;		// Represents local transform only.
				Donya::Vector4x4	global;		// Contain all parent's global transform. If the root bone, this matrix contains the local transform only.
			public:
				/// <summary>
				/// The "local" transform will be made by interpolated bone. The "global" transform will be linear-interpolated.
				/// </summary>
				static Node Interpolate( const Node &lhs, const Node &rhs, float time );
			private:
				friend class cereal::access;
				template<class Archive>
				void serialize( Archive &archive, std::uint32_t version )
				{
					archive
					(
						CEREAL_NVP( bone	),
						CEREAL_NVP( local	),
						CEREAL_NVP( global	)
					);

					if ( 1 <= version )
					{
						// archive( CEREAL_NVP(  ) );
					}
				}
			};

			/// <summary>
			/// A transforming data of an associated skeletal, at some timing.
			/// </summary>
			struct KeyFrame
			{
				float				seconds;	// A begin seconds of some key-frame.
				std::vector<Node>	keyPose;	// A skeletal at some timing. That transform space is bone -> mesh.
			public:
				/// <summary>
				/// Requires the two key-pose counts are the same.
				/// </summary>
				static KeyFrame Interpolate( const KeyFrame &lhs, const KeyFrame &rhs, float time );
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
							CEREAL_NVP( keyPose	)
						);
					}
				}
			};
			/// <summary>
			/// A gathering of an associated key-frame. Store some snap-shots of skeletal per sampling-rate.
			/// </summary>
			struct Motion
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
							CEREAL_NVP(	keyFrames		)
						);
					}
				}
			};
		}

		/// <summary>
		/// The members of constant-buffer by model type.
		/// </summary>
		namespace Constants
		{
			/// <summary>
			/// The constants that update per need. This is not related to the model. Exists for default shading of Model::Renderer.
			/// </summary>
			namespace PerScene
			{
				struct DirectionalLight
				{
					Donya::Vector4 color;		// Will be used as:color.xyz * color.w.
					Donya::Vector4 direction;
				};
				/// <summary>
				/// The everything model types is using this structure's member.
				/// </summary>
				struct Common
				{
					DirectionalLight directionalLight;
					Donya::Vector4   eyePosition;
					Donya::Vector4x4 viewProjMatrix;	// World space -> NDC(actually Clip space)
				};
			}
			/// <summary>
			/// The constants that update per model.
			/// </summary>
			namespace PerModel
			{
				/// <summary>
				/// The everything model types is using this structure's member.
				/// </summary>
				struct Common
				{
					Donya::Vector4   drawColor;
					Donya::Vector4x4 worldMatrix;	// Model space -> World space
				};
			}
			/// <summary>
			/// The constants that update per mesh.
			/// </summary>
			namespace PerMesh
			{
				/// <summary>
				/// The everything model types is using this structure's member.
				/// </summary>
				struct Common
				{
					Donya::Vector4x4 adjustMatrix; // This matrix transforms the space is global of a model -> local of the game. for example, that contain coordinate conversion matrix.
				};

				/// <summary>
				/// The model type of using skinning.
				/// </summary>
				struct Bone
				{
					static constexpr unsigned int MAX_BONE_COUNT = 64U;
				public:
					// This matrix transforms to world space of game from bone space in initial-pose.
					std::array<Donya::Vector4x4, MAX_BONE_COUNT> boneTransforms;
				};
			}
			/// <summary>
			/// The constants that update per subset.
			/// </summary>
			namespace PerSubset
			{
				/// <summary>
				/// The everything model types is using this structure's member.
				/// </summary>
				struct Common
				{
					Donya::Vector4 ambient;
					Donya::Vector4 diffuse;
					Donya::Vector4 specular;
				};
			}
		}

		/// <summary>
		/// The setting description of some shader's slot.
		/// </summary>
		struct RegisterDesc
		{
			unsigned int setSlot = 0;
			bool setVS = true;
			bool setPS = true;
		public:
			static RegisterDesc Make( unsigned int setSlot, bool setVS, bool setPS )
			{
				return RegisterDesc{ setSlot, setVS, setPS };
			}
		};
	}
}

CEREAL_CLASS_VERSION( Donya::Model::Vertex::Pos,			0 )
CEREAL_CLASS_VERSION( Donya::Model::Vertex::Tex,			0 )
CEREAL_CLASS_VERSION( Donya::Model::Vertex::Bone,			0 )

CEREAL_CLASS_VERSION( Donya::Model::Animation::Bone,		0 )
CEREAL_CLASS_VERSION( Donya::Model::Animation::KeyFrame,	0 )
CEREAL_CLASS_VERSION( Donya::Model::Animation::Motion,		0 )
