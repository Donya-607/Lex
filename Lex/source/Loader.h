#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#undef max
#undef min

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "SkinnedMesh.h"

#define USE_FBX_SDK ( true )

#if USE_FBX_SDK
namespace fbxsdk
{
	class FbxMesh;
	class FbxSurfaceMaterial;
}
#endif // USE_FBX_SDK

// Program version : 3

namespace Donya
{
	/// <summary>
	/// It can copy.
	/// </summary>
	class Loader
	{
	private:
		static constexpr const char *SERIAL_ID = "Loader";
		static std::mutex cerealMutex;

	#if USE_FBX_SDK
		static std::mutex fbxMutex;
	#endif // USE_FBX_SDK
	public:
	#pragma region Structs

		struct Material
		{
			Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };	// w channel is used as shininess by only specular.
			std::vector<std::string> relativeTexturePaths{};
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( color ),
					CEREAL_NVP( relativeTexturePaths )
				);
				if ( 1 <= version )
				{
					// archive();
				}
			}
		};

		struct Subset
		{
			size_t		indexCount{};
			size_t		indexStart{};
			float		reflection{};
			float		transparency{};
			Material	ambient{};
			Material	bump{};
			Material	diffuse{};
			Material	emissive{};
			Material	specular{};
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( indexCount ), CEREAL_NVP( indexStart ),
					CEREAL_NVP( reflection ), CEREAL_NVP( transparency ),
					CEREAL_NVP( ambient ), CEREAL_NVP( bump ),
					CEREAL_NVP( diffuse ), CEREAL_NVP( emissive ),
					CEREAL_NVP( specular )
				);
				if ( 1 <= version )
				{
					// archive();
				}
			}
		};

		/// <summary>
		/// Rig. Controller.
		/// </summary>
		struct Bone
		{
			std::string			name{};
			Donya::Vector4x4	transform{}; // From initial model space to posed model space.
			// DirectX::XMFLOAT4X4 transformToBone{}; // From model space to bone space.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( name ),
					CEREAL_NVP( transform )
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		/// <summary>
		/// Gathering of bones(I call "skeletal"). This represents a posture at that time.
		/// </summary>
		struct Skeletal
		{
			size_t				boneCount{};
			std::vector<Bone>	skeletal{};
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( boneCount ),
					CEREAL_NVP( skeletal )
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		/// <summary>
		/// Gathering of skeletals(I call "Motion"). This represents a motion(animation).
		/// </summary>
		struct Motion
		{
			static constexpr float DEFAULT_SAMPLING_RATE = 1.0f / 24.0f;
		public:
			int							meshNo{};	// 0-based.
			float						samplingRate{ DEFAULT_SAMPLING_RATE };
			std::vector<std::string>	names{};
			std::vector<Skeletal>		motion{};	// Store consecutive skeletals according to a time.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( meshNo ),
					CEREAL_NVP( samplingRate ),
					CEREAL_NVP( names ),
					CEREAL_NVP( motion )
				);
				
				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		struct BoneInfluence
		{
			int		index{};
			float	weight{};
		public:
			BoneInfluence() : index(), weight() {}
			BoneInfluence( int index, float weight ) : index( index ), weight( weight ) {}
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( index ),
					CEREAL_NVP( weight )
				);
				if ( 1 <= version )
				{
					// archive();
				}
			}
		};
		
		struct BoneInfluencesPerControlPoint
		{
			std::vector<BoneInfluence> cluster{};
		public:
			BoneInfluencesPerControlPoint() : cluster() {}
			BoneInfluencesPerControlPoint( const BoneInfluencesPerControlPoint &ref ) : cluster( ref.cluster ) {}
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( cluster )
				);
				if ( 1 <= version )
				{
					// archive();
				}
			}
		};

		struct Mesh
		{
			int							meshNo{};	// 0-based.
			Donya::Vector4x4			coordinateConversion{};
			Donya::Vector4x4			globalTransform{};
			std::vector<Subset>			subsets{};
			std::vector<size_t>			indices{};
			std::vector<Donya::Vector3>	normals{};
			std::vector<Donya::Vector3>	positions{};
			std::vector<Donya::Vector2>	texCoords{};
			std::vector<BoneInfluencesPerControlPoint>	influences{};
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( coordinateConversion ),
					CEREAL_NVP( globalTransform ),
					CEREAL_NVP( subsets ),
					CEREAL_NVP( indices ), CEREAL_NVP( normals ),
					CEREAL_NVP( positions ), CEREAL_NVP( texCoords ),
					CEREAL_NVP( influences )
				);
				if ( 1 <= version )
				{
					archive( CEREAL_NVP( meshNo ) );
				}
				if ( 2 <= version )
				{
					// archive();
				}
			}
		};

		// region Structs
	#pragma endregion
	private:
		std::string			absFilePath;
		std::string			fileName;		// only file-name, the directory is not contain.
		std::string			fileDirectory;	// '/' terminated.
		std::vector<Mesh>	meshes;
		std::vector<Motion> motions;
	public:
		Loader();
		~Loader();
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( absFilePath ),
				CEREAL_NVP( fileName ),
				CEREAL_NVP( fileDirectory ),
				CEREAL_NVP( meshes )
			);
			if ( 1 <= version )
			{
				archive( CEREAL_NVP( motions ) );
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		/// <summary>
		/// We can those load file extensions:<para></para>
		/// .fbx, .FBX(If the flag of use fbx-sdk is on),<para></para>
		/// .obj, .OBJ(If the flag of use fbx-sdk is on),<para></para>
		/// .bin, .json(Expect, only file of saved by this Loader class).<para></para>
		/// The "outputErrorString" can set nullptr.
		/// </summary>
		bool Load( const std::string &filePath, std::string *outputErrorString, bool outputDebugProgress = true );

		/// <summary>
		/// We expect the "filePath" contain extension also.
		/// </summary>
		void SaveByCereal( const std::string &filePath ) const;
	public:
		std::string GetAbsoluteFilePath()		const { return absFilePath;		}
		std::string GetOnlyFileName()			const { return fileName;		}
		std::string GetFileDirectory()			const { return fileDirectory;	}
		const std::vector<Mesh>   *GetMeshes()	const { return &meshes;			}
		const std::vector<Motion> *GetMotions()	const { return &motions;		}
	private:
		bool LoadByCereal( const std::string &filePath, std::string *outputErrorString, bool outputDebugProgress );
		
	#if USE_FBX_SDK
		bool LoadByFBXSDK( const std::string &filePath, std::string *outputErrorString, bool outputDebugProgress );

		void MakeAbsoluteFilePath( const std::string &filePath );

		void FetchVertices( size_t meshIndex, const fbxsdk::FbxMesh *pMesh, const std::vector<BoneInfluencesPerControlPoint> &fetchedInfluencesPerControlPoints );
		void FetchMaterial( size_t meshIndex, const fbxsdk::FbxMesh *pMesh );
		void AnalyseProperty( size_t meshIndex, int mtlIndex, fbxsdk::FbxSurfaceMaterial *pMaterial );
		void FetchGlobalTransform( size_t meshIndex, const fbxsdk::FbxMesh *pMesh );
	#endif // USE_FBX_SDK
		
	#if USE_IMGUI
	public:
		/// <summary>
		/// This function don't ImGui::Begin() and Begin::End().<para></para>
		/// please call between ImGui::Begin() to ImGui::End().
		/// </summary>
		void AdjustParameterByImGuiNode();
		/// <summary>
		/// This function don't ImGui::Begin() and Begin::End().<para></para>
		/// please call between ImGui::Begin() to ImGui::End().
		/// </summary>
		void EnumPreservingDataToImGui() const;
	#endif // USE_IMGUI

	};

}

CEREAL_CLASS_VERSION( Donya::Loader,				1 )
CEREAL_CLASS_VERSION( Donya::Loader::Material,		0 )
CEREAL_CLASS_VERSION( Donya::Loader::Subset,		0 )
CEREAL_CLASS_VERSION( Donya::Loader::Bone,			0 )
CEREAL_CLASS_VERSION( Donya::Loader::Skeletal,		0 )
CEREAL_CLASS_VERSION( Donya::Loader::Motion,		0 )
CEREAL_CLASS_VERSION( Donya::Loader::BoneInfluence, 0 )
CEREAL_CLASS_VERSION( Donya::Loader::BoneInfluencesPerControlPoint, 0 )
CEREAL_CLASS_VERSION( Donya::Loader::Mesh,			1 )
