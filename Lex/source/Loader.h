#ifndef _INCLUDED_LOADER_H_
#define _INCLUDED_LOADER_H_

#include <string>
#include <vector>

#include "UseImGui.h"
#include "Vector.h"

namespace Donya
{
	/// <summary>
	/// Loader can load a FBX file.<para></para>
	/// also preserve:<para></para>
	/// indices count,<para></para>
	/// normals,<para></para>
	/// positions.
	/// </summary>
	class Loader
	{
	private:
		size_t						vertexCount;	// 0 based.
		std::string					fileName;
		std::vector<size_t>			indices;
		std::vector<Donya::Vector3>	normals;
		std::vector<Donya::Vector3>	positions;
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
		const std::vector<size_t> *GetIndices() const { return &indices; }
		const std::vector<Donya::Vector3> *GetNormals() const { return &normals; }
		const std::vector<Donya::Vector3> *GetPositions() const { return &positions; }
	private:
		void MakeFileName( const std::string &filePath );
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