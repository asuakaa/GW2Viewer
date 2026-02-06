export module GW2Viewer.UI.Controls:CompletionMargin;
import GW2Viewer.Common;
import GW2Viewer.UI.ImGui;
import GW2Viewer.Utils.Enum;
import std;
import magic_enum;

export namespace GW2Viewer::UI::Controls
{

struct CompletionMarginOptions
{
    float Width = 4;
    uint32 Color = 0xFF0000FF;
    float AlphaEnumMin = 1.0f;
    float AlphaEnumMax = 0.0f;
};
bool CompletionMargin(std::integral auto current, std::integral auto max, CompletionMarginOptions const& options = { })
{
    float const alpha = std::lerp(options.AlphaEnumMin, options.AlphaEnumMax, current / (float)max);
    if (!alpha)
        return false;

    I::GetWindowDrawList()->AddRectFilled(I::LastRect().Min, { I::LastRect().Min.x + options.Width, I::LastRect().Max.y }, I::GetColorU32(options.Color, alpha));
    return true;
}
template<Enumeration Enum> bool CompletionMargin(Enum current, Enum max, CompletionMarginOptions const& options = { }) { return CompletionMargin(std::to_underlying(current), std::to_underlying(max), options); }
template<Enumeration Enum> bool CompletionMargin(Enum current, CompletionMarginOptions const& options = { }) { return CompletionMargin(current, magic_enum::enum_entries<Enum>().back().first, options); }

}
