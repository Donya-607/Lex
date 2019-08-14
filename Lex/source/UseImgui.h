#ifndef _INCLUDED_USE_IMGUI_H_
#define _INCLUDED_USE_IMGUI_H_

#define USE_IMGUI ( true )

#if USE_IMGUI

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>

#endif // USE_IMGUI

namespace Donya
{
	void SetShowStateOfImGui( bool isAllow );
	void TogguleShowStateOfImGui();

	/// <summary>
	/// In release build, returns false.
	/// </summary>
	bool IsAllowShowImGui();

	bool IsMouseHoveringImGuiWindow();
}

namespace ImGui
{
	/// <summary>
	/// ! This is My Wrapper Function !<para></para>
	/// This function doing Donya::IsAllowShowImGui() before ImGui::Begin().<para></para>
	/// This function's return value is same as ImGui::Begin().<para></para>
	/// You must be evaluate this in if-statement, then If returns false, you must not do something of ImGui related.<para></para>
	/// If returns true, you must be call ImGui::End().
	/// </summary>
	bool BeginIfAllowed( const char* name = nullptr, bool* p_open = NULL, ImGuiWindowFlags flags = 0 );
}

#endif // !_INCLUDED_USE_IMGUI_H_
