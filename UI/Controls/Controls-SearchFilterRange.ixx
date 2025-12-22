export module GW2Viewer.UI.Controls:SearchFilterRange;
import GW2Viewer.UI.ImGui;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Controls
{

template<typename FilterID, typename FilterRange>
bool SearchFilterRange(std::optional<FilterID>& filterID, FilterRange& filterRange)
{
    scoped::Disabled(!filterID);

    I::SetNextItemWidth(50);
    bool const result = I::DragInt("##SearchRange", (int*)&filterRange, 0.1f, 0, 10000, "<c=#8>" ICON_FA_PLUS_MINUS "</c> %d");
    if (I::IsItemHovered())
        I::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    return result;
}

}
