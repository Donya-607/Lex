#include "UseImGui.h"

namespace Donya
{
	static bool isAllowShowingImGui = true;
	void SetShowStateOfImGui( bool isAllow )
	{
		isAllowShowingImGui = isAllow;
	}
	bool IsAllowShowImGui()
	{
		return isAllowShowingImGui;
	}
}

namespace ImGui
{
	bool BeginIfAllowed( const char* name, bool* p_open, ImGuiWindowFlags flags )
	{
		if ( !Donya::IsAllowShowImGui() ) { return false; }
		// else

		if ( !ImGui::Begin( name, p_open, flags ) )
		{
			ImGui::End();
			return false;
		}
		// else
		return true;
	}
}