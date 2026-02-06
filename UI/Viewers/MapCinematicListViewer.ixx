export module GW2Viewer.UI.Viewers.MapCinematicListViewer;
import GW2Viewer.Common;
import GW2Viewer.Common.Time;
import GW2Viewer.Content.MapCinematic;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.Controls;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Manager;
import GW2Viewer.UI.Viewers.MapCinematicViewer;
import GW2Viewer.UI.Viewers.ListViewer;
import GW2Viewer.UI.Viewers.ViewerRegistry;
import GW2Viewer.Utils.Async;
import GW2Viewer.Utils.Encoding;
import GW2Viewer.Utils.Format;
import GW2Viewer.Utils.Scan;
import GW2Viewer.Utils.Sort;
import GW2Viewer.Utils.String;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Viewers
{

struct MapCinematicListViewer : ListViewer<MapCinematicListViewer, { ICON_FA_CROSSHAIRS " MapCinematics", "MapCinematics", Category::ListViewer }>
{
    MapCinematicListViewer(uint32 id, bool newTab) : Base(id, newTab) { UpdateSearch(); }

    std::shared_mutex Lock;
    std::vector<uint64> FilteredList;
    Utils::Async::Scheduler AsyncFilter { true };

    std::string FilterString;
    std::optional<std::pair<int64, int64>> FilterID;
    uint64 FilterRange { };
    enum class MapCinematicSort { Hash, UID, Text, EncounteredTime } Sort { MapCinematicSort::UID };
    bool SortInvert { };

    static auto SortList(Utils::Async::Context context, std::vector<uint64>& data, MapCinematicSort sort, bool invert)
    {
        std::shared_lock _(Content::mapCinematicsLock);
        switch (sort)
        {
            using Utils::Sort::ComplexSort;
            using enum MapCinematicSort;
            case Hash:
                std::ranges::sort(data, [invert](auto a, auto b) { return a < b ^ invert; });
                break;
            case UID:
                ComplexSort(data, invert, [](uint64 id) { return Content::mapCinematics.at(id).UID; });
                break;
            case Text:
                ComplexSort(data, invert, [](uint64 id) { return Content::mapCinematics.at(id).Text(); });
                break;
            case EncounteredTime:
                ComplexSort(data, invert, [](uint64 id) { return Content::mapCinematics.at(id).Encounter.Time; });
                break;
            default: std::terminate();
        }
    }
    void SetResult(Utils::Async::Context context, std::vector<uint64>&& data)
    {
        if (context->Cancelled) return;
        std::unique_lock _(Lock);
        FilteredList = std::move(data);
        context->Finish();
    }
    void UpdateSort() override
    {
        if (std::shared_lock _(Lock); FilteredList.empty())
            return UpdateSearch();

        AsyncFilter.Run([this, sort = Sort, invert = SortInvert](Utils::Async::Context context)
        {
            context->SetIndeterminate();
            std::vector<uint64> data;
            {
                std::shared_lock _(Lock);
                data = FilteredList;
            }
            CHECK_ASYNC;
            SortList(context, data, sort, invert);
            SetResult(context, std::move(data));
        });
    }
    void UpdateSearch() override
    {
        bool textSearch = false;
        FilterID.reset();
        if (FilterString.empty())
            ;
        else if (uint64 id, range; Utils::Scan::Into(FilterString, "{}+{}", id, range))
            FilterID.emplace(id - range, id + range);
        else if (Utils::Scan::Into(FilterString, "{}-{}", id, range))
            FilterID.emplace(id, range);
        else if (Utils::Scan::NumberLiteral(FilterString, id))
            FilterID.emplace(id - FilterRange, id + FilterRange);
        else if (Utils::Scan::NumberLiteral(FilterString, "0x{:x}", id))
            FilterID.emplace(id - FilterRange, id + FilterRange);
        else
            textSearch = true;

        AsyncFilter.Run([this, textSearch, filter = FilterID, string = FilterString, sort = Sort, invert = SortInvert](Utils::Async::Context context) mutable
        {
            context->SetIndeterminate();
            std::vector<uint64> data;
            CHECK_ASYNC;
            if (textSearch)
            {
                std::shared_lock _(Content::mapCinematicsLock);
                data.assign_range(Content::mapCinematics | std::views::keys | std::views::filter([query = Utils::String::Uppercased(Utils::Encoding::FromUTF8(string))](uint64 hash)
                {
                    auto const& mapCinematic = Content::mapCinematics.at(hash);
                    for (auto const& group : mapCinematic.Groups)
                    {
                        if (auto const string = G::Game.Text.GetNormalized(group.TextID).first; string && !string->empty() && string->contains(query))
                            return true;

                        for (auto const& object : group.Objects.GetSpan(mapCinematic.Objects))
                            if (auto const string = G::Game.Text.GetNormalized(object.TextID).first; string && !string->empty() && string->contains(query))
                                return true;
                    }
                    return false;
                }));
            }
            else
            {
                auto limits = filter.value_or(std::pair { std::numeric_limits<int64>::min(), std::numeric_limits<int64>::max() });
                std::shared_lock _(Content::mapCinematicsLock);
                data.assign_range(Content::mapCinematics | std::views::filter([limits](auto const& pair) { return (int64)pair.first >= limits.first && (int64)pair.first <= limits.second || (int64)pair.second.UID >= limits.first && (int64)pair.second.UID <= limits.second; }) | std::views::keys);
            }
            CHECK_ASYNC;
            SortList(context, data, sort, invert);
            SetResult(context, std::move(data));
        });
    }

    void Draw() override
    {
        if (scoped::TableDockRight("Search"))
        {
            I::TableNextColumn();
            if (Controls::SearchInput(FilterString, FilteredList, Lock, &AsyncFilter))
                UpdateSearch();

            I::TableNextColumn();
            if (Controls::SearchFilterRange(FilterID, FilterRange))
                UpdateSearch();
        }

        if (scoped::TableList("Table", 4))
        {
            I::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 50, (ImGuiID)MapCinematicSort::Hash);
            I::TableSetupColumn("~UID", ImGuiTableColumnFlags_WidthFixed, 50, (ImGuiID)MapCinematicSort::UID);
            I::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthStretch, 0, (ImGuiID)MapCinematicSort::Text);
            I::TableSetupColumn("Encountered", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, 80, (ImGuiID)MapCinematicSort::EncounteredTime);
            I::TableSetupScrollFreeze(0, 1);
            I::TableHeadersRow();
            HandleTableSort(Sort, SortInvert);

            std::shared_lock __(Lock);
            ImGuiListClipper clipper;
            clipper.Begin(FilteredList.size());
            while (clipper.Step())
            {
                for (auto mapCinematicHash : std::span(FilteredList.begin() + clipper.DisplayStart, FilteredList.begin() + clipper.DisplayEnd))
                {
                    scoped::WithID(mapCinematicHash);

                    std::shared_lock ___(Content::mapCinematicsLock);
                    auto& mapCinematic = Content::mapCinematics.at(mapCinematicHash);
                    auto const* currentViewer = G::UI.GetCurrentViewer<MapCinematicViewer>();
                    I::TableNextRow();

                    I::TableNextColumn();
                    I::Selectable(std::format("<c=#4>{}</c>", mapCinematicHash).c_str(), currentViewer && currentViewer->MapCinematicHash == mapCinematicHash ? ImGuiTreeNodeFlags_Selected : 0, ImGuiSelectableFlags_SpanAllColumns);

                    if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                        MapCinematicViewer::Open(mapCinematicHash, { .MouseButton = button });

                    I::TableNextColumn();
                    I::Text("<c=#8>%u</c>", mapCinematic.UID);

                    I::TableNextColumn();
                    I::TextUnformatted(mapCinematic.Text().c_str());

                    I::TableNextColumn();
                    Controls::Encounter(mapCinematic.Encounter);
                }
            }
        }
    }
};

}
