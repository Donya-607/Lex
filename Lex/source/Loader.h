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
			Donya::Vector4 color;	// w channel is used as shininess by only specular.
			std::vector<std::string> textureNames;
		public:
			Material() : color( 0, 0, 0, 0 ), textureNames()
			{}
			Material( const Material &ref )
			{
				*this = ref;
			}
			Material &operator = ( const Material &ref )
			{
				color = ref.color;
				textureNames = ref.textureNames;
				return *this;
			}
			~Material()
			{}
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
		};

		struct Mesh
		{
			DirectX::XMFLOAT4X4			globalTransform;
			std::vector<Subset>			subsets;
			std::vector<size_t>			indices;
			std::vector<Donya::Vector3>	normals;
			std::vector<Donya::Vector3>	positions;
			std::vector<Donya::Vector2>	texCoords;
		public:
			Mesh() : globalTransform
			(
				{
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
				}
			),
			subsets(), indices(), normals(), positions(), texCoords()
			{}
			Mesh( const Mesh & ) = default;
		};
	private:
		size_t				vertexCount;	// 0 based.
		std::string			fileName;
		std::string			fileDirectory;	// '/' terminated.
		std::vector<Mesh>	meshes;
	public:
		Loader();
		~Loader();
	public:
		/// <summary>
		/// outputErrorString can set nullptr.
		/// </summary>
		bool Load( const std::string &filePath, std::string *outputErrorString );
	public:
		std::string GetFileName() const { return fileName; }
		const std::vector<Mesh> *GetMeshes() const { return &meshes; }
	private:
		void MakeFileName( const std::string &filePath );

		void FetchVertices( size_t meshIndex, const fbxsdk::FbxMesh *pMesh );
		void FetchMaterial( size_t meshIndex, const fbxsdk::FbxMesh *pMesh );
		void AnalyseProperty( size_t meshIndex, int mtlIndex, fbxsdk::FbxSurfaceMaterial *pMaterial );
		void FetchGlobalTransform( size_t meshIndex, const fbxsdk::FbxMesh *pMesh );

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