export module GW2Viewer.UI.Viewers.MapCinematicViewer;
import GW2Viewer.Common;
import GW2Viewer.Content;
import GW2Viewer.Content.MapCinematic;
import GW2Viewer.Data.Content;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.Controls;
import GW2Viewer.UI.Viewers.ViewerRegistry;
import GW2Viewer.UI.Viewers.ViewerWithHistory;
import GW2Viewer.UI.ImGui;
import GW2Viewer.Utils.Encoding;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Viewers
{

struct MapCinematicViewer : ViewerWithHistory<MapCinematicViewer, uint64, { ICON_FA_CROSSHAIRS " MapCinematic", "MapCinematic", Category::ObjectViewer }>
{
    TargetType MapCinematicHash;

    MapCinematicViewer(uint32 id, bool newTab, TargetType mapCinematicHash) : Base(id, newTab), MapCinematicHash(mapCinematicHash) { }

    TargetType GetCurrent() const override { return MapCinematicHash; }
    bool IsCurrent(TargetType target) const override { return MapCinematicHash == target; }

    std::string Title() override
    {
        return std::format("<c=#4>{} #</c>{}", Base::Title(), Content::mapCinematics.at(MapCinematicHash).UID);
    }
    void Draw() override
    {
        std::shared_lock _(Content::mapCinematicsLock);
        auto const& mapCinematic = Content::mapCinematics.at(MapCinematicHash);

        if (scoped::Child(I::GetSharedScopeID("MapCinematicViewer"), { }, ImGuiChildFlags_Borders | ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeY))
            DrawHistoryButtons();

        if (scoped::Child("Contents"))
        {
            Controls::SymbolEnum("Flags", mapCinematic.Flags, "MapCinematicFlags");

            I::SeparatorText("Groups");
            if (scoped::WithStyleVarY(ImGuiStyleVar_ItemSpacing, 0))
            for (auto const& [index, group] : mapCinematic.Groups | std::views::enumerate)
            {
                if (auto const string = G::Game.Text.Get(group.TextID).first; scoped::TreeNodeEx(&group, ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_DrawLinesToNodes, "<c=#4>[%u]</c> %s", (uint32)index, string ? Utils::Encoding::ToUTF8(*string).c_str() : ""))
                {
                    Controls::SymbolText("Text", group.TextID);
                    Controls::Symbol("MaleVoiceID", group.MaleVoiceID);
                    Controls::Symbol("FemaleVoiceID", group.FemaleVoiceID);

                    for (auto const& [index, object] : group.Objects.GetSpan(mapCinematic.Objects) | std::views::enumerate)
                    {
                        if (auto const string = G::Game.Text.Get(object.TextID).first; scoped::TreeNodeEx(&object, ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_DrawLinesToNodes, "<c=#4>[%u]</c> %s", (uint32)index, string ? Utils::Encoding::ToUTF8(*string).c_str() : ""))
                        {
                            Controls::SymbolText("Text", object.TextID);
                            Controls::Symbol("Content", object.ContentGUID);
                            Controls::Symbol("ContinentPosition", object.ContinentPosition);
                            Controls::Symbol("Floor", object.Floor);
                        }
                    }
                }
            }
            I::NewLine();
            I::SeparatorText("Sectors");
            if (scoped::WithStyleVarY(ImGuiStyleVar_ItemSpacing, 0))
            for (auto const& sector : mapCinematic.Sectors)
            {
                Controls::ContentButton(G::Game.Content.GetByDataID(Content::SectorDef, sector.DataID), &sector.DataID);
            }
        }
    }
};

}
