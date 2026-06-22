export module GW2Viewer.UI.Viewers.MapCinematicListViewer;
import GW2Viewer.Common;
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
import GW2Viewer.Utils.Scan;
import GW2Viewer.Utils.String;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Viewers
{

struct MapCinematicListViewer : ListViewer<MapCinematicListViewer, { ICON_FA_CROSSHAIRS " MapCinematics", "MapCinematics", Category::ListViewer }>
{
    std::shared_mutex Lock;
    std::vector<uint64> FilteredList;
    Utils::Async::Scheduler AsyncFilter { true };

    std::string FilterString;
    std::optional<std::pair<int64, int64>> FilterID;
    uint64 FilterRange { };

    struct DrawContext
    {
        uint64 MapCinematicHash;
        std::shared_lock<decltype(Content::mapCinematicsLock)> Lock { Content::mapCinematicsLock };
        Content::MapCinematic const& MapCinematic = Content::mapCinematics.at(MapCinematicHash);
    };
    Controls::Table<uint64, decltype(FilteredList)&, DrawContext const&> Table { *this };

    MapCinematicListViewer(uint32 id, bool newTab) : Base(id, newTab)
    {
        Table.SetShowFilterRow();
        Table.AddColumn("#",
        {
            .Width = 50,
            .Filter = [](uint64 id) { return id; },
            .Sort = [](decltype(FilteredList)& data, bool invert) { std::ranges::sort(data, [invert](auto a, auto b) { return a < b ^ invert; }); },
            .Draw = [](DrawContext const& context)
            {
                auto const* currentViewer = G::UI.GetCurrentViewer<MapCinematicViewer>();
                I::SetNextItemAllowOverlap();
                I::Selectable(std::format("<c=#4>{}</c>", context.MapCinematicHash).c_str(), currentViewer && currentViewer->MapCinematicHash == context.MapCinematicHash ? ImGuiTreeNodeFlags_Selected : 0, ImGuiSelectableFlags_SpanAllColumns);

                if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                    MapCinematicViewer::Open(context.MapCinematicHash, { .MouseButton = button });
            },
        });
        Table.AddColumn("~UID",
        {
            .Width = 50,
            .Filter = [](uint64 id) { return Content::mapCinematics.at(id).UID; },
            .Draw = [](DrawContext const& context) { I::Text("<c=#8>%u</c>", context.MapCinematic.UID); },
        });
        Table.AddColumn("Text",
        {
            .Width = -1,
            .Filter = [](uint64 id) { return Content::mapCinematics.at(id).Text(); },
            .Draw = [](DrawContext const& context) { I::TextUnformatted(context.MapCinematic.Text().c_str()); },
        });
        Table.AddColumn("Encountered",
        {
            .Width = 80,
            .PreferSortDescending = true,
            .Filter = [](uint64 id) { return Content::mapCinematics.at(id).Encounter.Time; },
            .Draw = [](DrawContext const& context){ Controls::Encounter(context.MapCinematic.Encounter); },
        });

        UpdateSearch();
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

        AsyncFilter.Run([this, table = Table.Prepare()](Utils::Async::Context context)
        {
            context->SetIndeterminate();
            std::vector<uint64> data;
            {
                std::shared_lock _(Lock);
                data = FilteredList;
            }
            CHECK_ASYNC;
            table.Sort(data);
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

        AsyncFilter.Run([this, textSearch, filter = FilterID, string = FilterString, table = Table.Prepare()](Utils::Async::Context context) mutable
        {
            context->SetIndeterminate();
            auto const limits = filter.value_or(std::pair { std::numeric_limits<int64>::min(), std::numeric_limits<int64>::max() });
            std::vector<uint64> data;
            CHECK_ASYNC;
            {
                std::shared_lock _(Content::mapCinematicsLock);
                data.assign_range(Content::mapCinematics | std::views::filter([limits, &table, textSearch, query = Utils::String::Uppercased(Utils::Encoding::FromUTF8(string))](auto const& pair)
                {
                    if (!((int64)pair.first >= limits.first && (int64)pair.first <= limits.second || (int64)pair.second.UID >= limits.first && (int64)pair.second.UID <= limits.second))
                        return false;

                    if (!table.Filter(pair.first))
                        return false;

                    if (!textSearch)
                        return true;

                    for (auto const& group : pair.second.Groups)
                    {
                        if (auto const string = G::Game.Text.GetNormalized(group.TextID).first; string && !string->empty() && string->contains(query))
                            return true;

                        for (auto const& object : group.Objects.GetSpan(pair.second.Objects))
                            if (auto const string = G::Game.Text.GetNormalized(object.TextID).first; string && !string->empty() && string->contains(query))
                                return true;
                    }
                    return false;
                }) | std::views::keys);
            }
            CHECK_ASYNC;
            table.Sort(data);
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

        if (scoped::TableList("Table", Table.GetColumnCount()))
        {
            Table.DrawColumnHeaders();

            std::shared_lock __(Lock);
            ImGuiListClipper clipper;
            clipper.Begin(FilteredList.size());
            while (clipper.Step())
                for (auto mapCinematicHash : std::span(FilteredList.begin() + clipper.DisplayStart, FilteredList.begin() + clipper.DisplayEnd))
                    if (scoped::WithID(mapCinematicHash))
                        Table.DrawRow({ .MapCinematicHash = mapCinematicHash });
        }
    }
};

}
