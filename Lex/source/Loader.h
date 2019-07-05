#ifndef _INCLUDED_LOADER_H_
#define _INCLUDED_LOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "SkinnedMesh.h"
#include "UseImGui.h"
#include "Vector.h"

namespace fbxsdk
{
	class FbxMesh;
	class FbxSurfaceMaterial;
}

namespace Donya
{
	/// <summary>
	/// Loader can load a FBX file.<para></para>
	/// also preserve:<para></para>
	/// indices count,<para></para>
	/// normals,<para></para>
	/// positions,<para></para>
	/// texCoords.
	/// </summary>
	class Loader
	{
	public:
		struct Material
		{
			Donya::Vector3	ambient;
			Donya::Vector3	bump;
			Donya::Vector3	diffuse;
			Donya::Vector3	emissive;

			float			transparency;
			struct Phong
			{
				float			refrectivity;
				float			shininess;
				Donya::Vector3	specular;
			public:
				Phong() : refrectivity( 0 ), shininess( 0 ), specular( 0, 0, 0 )
				{}
				Phong( const Phong & ) = default;
				Phong &operator = ( const Phong & ) = default;
			};
			std::unique_ptr<Phong> pPhong;

			std::vector<std::string> textureNames; // diffuse map, full path.
		public:
			Material() : ambient( 0, 0, 0 ), bump( 0, 0, 0 ), diffuse( 0, 0, 0 ), emissive( 0, 0, 0 ), transparency( 0 ), pPhong( nullptr ), textureNames()
			{}
			Material( const Material & );
			Material &operator = ( const Material & );
		};
	private:
		size_t						vertexCount;	// 0 based.
		std::string					fileName;
		std::string					fileDirectory;	// '/' terminated.
		std::vector<size_t>			indices;
		std::vector<Donya::Vector3>	normals;
		std::vector<Donya::Vector3>	positions;
		std::vector<Donya::Vector2>	texCoords;
		std::vector<Material>		materials;
	public:
		Loader();
		~Loader();
	public:
		/// <summary>
		/// outputErrorString can set nullptr.
		/// </summary>
		bool Load( const std::string &filePath, std::string *outputErrorString );
	public:
		size_t GetMaterialCount() const { return materials.size(); }
		std::string GetFileName() const { return fileName; }
		const std::vector<std::string> *GetTextureNames( size_t materialInex = 0 ) const { return &materials[materialInex].textureNames; }
		const std::vector<size_t> *GetIndices() const { return &indices; }
		const std::vector<Donya::Vector3> *GetNormals() const { return &normals; }
		const std::vector<Donya::Vector3> *GetPositions() const { return &positions; }
		const std::vector<Donya::Vector2> *GetTexCoords() const { return &texCoords; }
	private:
		void MakeFileName( const std::string &filePath );

		void FetchVertices( const fbxsdk::FbxMesh *pMesh );
		void FetchMaterial( const fbxsdk::FbxMesh *pMesh );
		void AnalyseProperty( fbxsdk::FbxSurfaceMaterial *pMaterial );

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

#endif // _INCLUDED_LOADER_H_