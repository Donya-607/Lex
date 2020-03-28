#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// The cereal's source code also will use max, min.
#undef max
#undef min

#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "ModelSource.h"
#include "ModelPolygon.h"

#define USE_FBX_SDK ( true )

#if USE_FBX_SDK
namespace fbxsdk
{
	class FbxMesh;
	class FbxSurfaceMaterial;
}
#endif // USE_FBX_SDK

// Program version : 9

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
	private: // Loading parameters.
		Model::Source		source;
		Model::PolygonGroup	polyGroup;
		
		std::string			fileDirectory;	// The '/' terminated file directory.
		std::string			fileName;		// The file name that contains the extension.
	private:
		float				sampleFPS;		// Use to sampling-rate of an all motions. If set value of lower-equal than zero, use a model's sampling-rate.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( source		),
				CEREAL_NVP( polyGroup	)
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_FBX_SDK
		/// <summary>
		/// This param is use for calculate a sampling-rate at loading a model by FBX(only if FBX).<para></para>
		/// If set value of lower-equal than zero(default is zero) to this, use a model's samling-rate.
		/// </summary>
		void SetSamplingFPS( float samplingFPS ) { sampleFPS = samplingFPS; }
	#endif // USE_FBX_SDK
	public:
		/// <summary>
		/// Clear the read source.
		/// </summary>
		void ClearData();
		/// <summary>
		/// We can those load file extensions:<para></para>
		/// .fbx, .FBX(If the flag of use fbx-sdk is on),<para></para>
		/// .obj, .OBJ(If the flag of use fbx-sdk is on),<para></para>
		/// .bin.
		/// </summary>
		bool Load( const std::string &filePath, bool outputDebugProgress = true );
	public:
		/// <summary>
		/// We expect the "filePath" contain extension also.
		/// </summary>
		void SaveByCereal( const std::string &filePath ) const;
	public:
		const Model::Source			&GetModelSource()	const { return source; }
		void SetModelSource( const Model::Source &newSource ) { source = newSource; }
		const Model::PolygonGroup	&GetPolygonGroup()	const { return polyGroup; }
		void SetPolygonGroup( const Model::PolygonGroup &newSource ) { polyGroup = newSource; }
	public:
		/// <summary>
		/// Ex. returns "Bar.fbx" from ["C:/Foo/Bar.fbx"].
		/// </summary>
		std::string GetFileName()						const { return fileName;		}
		/// <summary>
		/// Ex. returns "C:/Foo/" from ["C:/Foo/Bar.fbx"].
		/// </summary>
		std::string GetFileDirectory()					const { return fileDirectory;	}
	private:
		bool LoadByCereal( const std::string &filePath, bool outputDebugProgress );
	#if USE_FBX_SDK
		bool LoadByFBXSDK( const std::string &filePath, bool outputDebugProgress );
	#endif // USE_FBX_SDK
	public:
	#if USE_IMGUI
		void ShowEnumNode( const std::string &nodeCaption ) const;
	#endif // USE_IMGUI

	};

}
CEREAL_CLASS_VERSION( Donya::Loader, 0 )
