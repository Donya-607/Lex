#pragma once

#include <array>
#include <d3d11.h>				// Use for implement the method of returns input-element-descs on each vertex struct.

#include "Donya/Serializer.h"	// Use for impl a serialize method.
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
			static constexpr const auto InputElements()
			{
				return std::array<D3D11_INPUT_ELEMENT_DESC, 2>
				{
					D3D11_INPUT_ELEMENT_DESC{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
					D3D11_INPUT_ELEMENT_DESC{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
			static constexpr const auto InputElements()
			{
				return std::array<D3D11_INPUT_ELEMENT_DESC, 1>
				{
					D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
			static constexpr const auto InputElements()
			{
				return std::array<D3D11_INPUT_ELEMENT_DESC, 2>
				{
					D3D11_INPUT_ELEMENT_DESC{ "WEIGHTS"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
					D3D11_INPUT_ELEMENT_DESC{ "BONES"		, 0, DXGI_FORMAT_R32G32B32A32_UINT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	/// The members of constant-buffer by model type.
	/// </summary>
	namespace Constants
	{
		/// <summary>
		/// The constants that update per need. This is not related to the model. Exists for default shading of ModelRenderer.
		/// </summary>
		namespace PerNeed
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
				Donya::Vector4x4 adjustMatrix; // Model space. This matrix contain a global-transform, and coordinate-conversion matrix.
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
	/// The usage of constant-buffer.
	/// </summary>
	struct ConstantDesc
	{
		unsigned int setSlot = 0;
		bool setVS = true;
		bool setPS = true;
	};
}

CEREAL_CLASS_VERSION( Donya::Vertex::Pos,  0 )
CEREAL_CLASS_VERSION( Donya::Vertex::Tex,  0 )
CEREAL_CLASS_VERSION( Donya::Vertex::Bone, 0 )