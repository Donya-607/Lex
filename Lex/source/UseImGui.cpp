#include "UseImGui.h"

namespace Donya
{
	static bool isAllowShowingImGui = true;
	void SetShowStateOfImGui( bool isAllow )
	{
		isAllowShowingImGui = isAllow;
	}
	void TogguleShowStateOfImGui()
	{
		isAllowShowingImGui = !isAllowShowingImGui;
	}

	bool IsAllowShowImGui()
	{
		return isAllowShowingImGui;
	}

	bool IsMouseHoveringImGuiWindow()
	{
	#if !USE_IMGUI
		return false;
	#endif // !USE_IMGUI
		return ImGui::IsMouseHoveringAnyWindow();
	}
}

namespace ImGui
{
	bool BeginIfAllowed( const char* name, bool* p_open, ImGuiWindowFlags flags )
	{
	#if !USE_IMGUI

		return false;

	#endif // !USE_IMGUI

		const char *caption = ( name == nullptr ) ? "Lex" : name;

		if ( !Donya::IsAllowShowImGui() ) { return false; }
		// else

		if ( !ImGui::Begin( caption, p_open, flags ) )
		{
			ImGui::End();
			return false;
		}
		// else
		return true;
	}
}