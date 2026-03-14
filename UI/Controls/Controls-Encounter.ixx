export module GW2Viewer.UI.Controls:Encounter;
import GW2Viewer.Common.Time;
import GW2Viewer.Data.External.Types;
import GW2Viewer.Data.Map.DisplaySet;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Viewers.Viewer;
import GW2Viewer.Utils.Format;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Controls
{
void OpenMapLayout(Data::Map::DisplaySet const& set, Viewers::OpenViewerOptions const& options);

struct EncounterOptions
{
    bool Button = false;
    std::string_view TimeText = "Encountered on";
};
bool Encounter(Data::External::Encounter const& encounter, EncounterOptions const& options = { })
{
    if (!encounter.Time.time_since_epoch().count())
        return false;

    scoped::WithID(&encounter);

    auto const text = std::format("<c=#{}>{}</c> {}###Encounter",
        encounter.Map ? "F" : "2",
        ICON_FA_GLOBE,
        Utils::Format::DurationShortColored("{} ago", Time::UntilNowSecs(encounter.Time)));

    bool result;
    if (options.Button)
        result = I::Button(text.c_str());
    else
        result = I::Selectable(text.c_str());

    if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
    {
        if (encounter.Map)
        {
            OpenMapLayout(Data::Map::DisplaySet().Add(encounter), { .MouseButton = button });
            result = true;
        }
    }

    if (scoped::ItemTooltip())
        I::TextUnformatted(std::format("{}: {}", options.TimeText, Utils::Format::DateTimeFullLocal(encounter.Time)).c_str());

    return result;
}

}
