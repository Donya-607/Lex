#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#undef max
#undef min

#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

#include "Serializer.h"
#include "SkinnedMesh.h"
#include "UseImGui.h"
#include "Vector.h"

#define USE_FBX_SDK ( true )

#if USE_FBX_SDK
namespace fbxsdk
{
	class FbxMesh;
	class FbxSurfaceMaterial;
}
#endif // USE_FBX_SDK

namespace DirectX
{
	template<class Archive>
	void serialize( Archive &archive, XMFLOAT4X4 &f4x4 )
	{
		archive
		(
			cereal::make_nvp( "_11", f4x4._11 ),
			cereal::make_nvp( "_12", f4x4._12 ),
			cereal::make_nvp( "_13", f4x4._13 ),
			cereal::make_nvp( "_14", f4x4._14 ),
			
			cereal::make_nvp( "_21", f4x4._21 ),
			cereal::make_nvp( "_22", f4x4._22 ),
			cereal::make_nvp( "_23", f4x4._23 ),
			cereal::make_nvp( "_24", f4x4._24 ),

			cereal::make_nvp( "_31", f4x4._31 ),
			cereal::make_nvp( "_32", f4x4._32 ),
			cereal::make_nvp( "_33", f4x4._33 ),
			cereal::make_nvp( "_34", f4x4._34 ),
			
			cereal::make_nvp( "_41", f4x4._41 ),
			cereal::make_nvp( "_42", f4x4._42 ),
			cereal::make_nvp( "_43", f4x4._43 ),
			cereal::make_nvp( "_44", f4x4._44 )
		);
	}
}

namespace Donya
{
	/// <summary>
	/// It can copy.
	/// </summary>
	class Loader
	{
		static constexpr unsigned int PROGRAM_VERSION = 1;
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
			Donya::Vector4 color;	// w channel is used as shininess by only specular.
			std::vector<std::string> relativeTexturePaths;
		public:
			Material() : color( 0, 0, 0, 0 ), relativeTexturePaths()
			{}
			Material( const Material &ref )
			{
				*this = ref;
			}
			Material &operator = ( const Material &ref )
			{
				color = ref.color;
				relativeTexturePaths = ref.relativeTexturePaths;
				return *this;
			}
			~Material()
			{}
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
			size_t indexCount;
			size_t indexStart;
			float  reflection;
			float  transparency;
			Material ambient;
			Material bump;
			Material diffuse;
			Material emissive;
			Material specular;
		public:
			Subset() : indexCount( NULL ), indexStart( NULL ), reflection( 0 ), transparency( 0 ), ambient(), bump(), diffuse(), emissive()
			{}
			~Subset()
			{}
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

		struct Bone
		{
			DirectX::XMFLOAT4X4 transform{};
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
			DirectX::XMFLOAT4X4			coordinateConversion;
			DirectX::XMFLOAT4X4			globalTransform;
			std::vector<Subset>			subsets;
			std::vector<size_t>			indices;
			std::vector<Donya::Vector3>	normals;
			std::vector<Donya::Vector3>	positions;
			std::vector<Donya::Vector2>	texCoords;
			std::vector<Bone>			skeletal;
			std::vector<BoneInfluencesPerControlPoint>	influences;
		public:
			Mesh() : coordinateConversion
			(
				{
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
				}
			),
			globalTransform
			(
				{
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
				}
			),
			subsets(), indices(), normals(), positions(), texCoords(),
			skeletal(), influences()
			{}
			Mesh( const Mesh & ) = default;
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
				// archive();
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
		bool Load( const std::string &filePath, std::string *outputErrorString );

		/// <summary>
		/// We expect the "filePath" contain extension also.
		/// </summary>
		void SaveByCereal( const std::string &filePath ) const;
	public:
		std::string GetAbsoluteFilePath()		const { return absFilePath;		}
		std::string GetOnlyFileName()			const { return fileName;		}
		std::string GetFileDirectory()			const { return fileDirectory;	}
		const std::vector<Mesh> *GetMeshes()	const { return &meshes;			}
	private:
		bool LoadByCereal( const std::string &filePath, std::string *outputErrorString );
		
	#if USE_FBX_SDK
		bool LoadByFBXSDK( const std::string &filePath, std::string *outputErrorString );

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
		void EnumPreservingDataToImGui( const char *ImGuiWindowIdentifier ) const;
	#endif // USE_IMGUI

	};

}

CEREAL_CLASS_VERSION( Donya::Loader, 0 )
CEREAL_CLASS_VERSION( Donya::Loader::Material, 0 )
CEREAL_CLASS_VERSION( Donya::Loader::Subset, 0 )
CEREAL_CLASS_VERSION( Donya::Loader::BoneInfluence, 0 )
CEREAL_CLASS_VERSION( Donya::Loader::BoneInfluencesPerControlPoint, 0 )
CEREAL_CLASS_VERSION( Donya::Loader::Mesh, 0 )
