export module GW2Viewer.UI.Viewers.MysticForgeEntryListViewer;
import GW2Viewer.Common;
import GW2Viewer.Content;
import GW2Viewer.Content.MysticForgeEntry;
import GW2Viewer.Data.Content;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.Controls;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Manager;
import GW2Viewer.UI.Viewers.ListViewer;
import GW2Viewer.UI.Viewers.MysticForgeEntryViewer;
import GW2Viewer.UI.Viewers.ViewerRegistry;
import GW2Viewer.Utils.Async;
import GW2Viewer.Utils.Encoding;
import GW2Viewer.Utils.Scan;
import GW2Viewer.Utils.Sort;
import GW2Viewer.Utils.String;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Viewers
{

struct MysticForgeEntryListViewer : ListViewer<MysticForgeEntryListViewer, { ICON_FA_TOILET " MysticForgeEntries", "MysticForgeEntries", Category::ListViewer }>
{
    std::shared_mutex Lock;
    std::vector<Content::MysticForgeEntryID> FilteredList;
    Utils::Async::Scheduler AsyncFilter { true };

    std::string FilterString;
    std::optional<std::pair<int64, int64>> FilterID;
    uint64 FilterRange { };

    struct DrawContext
    {
        Content::MysticForgeEntryID MysticForgeEntryID;
        std::shared_lock<decltype(Content::mysticForgeEntriesLock)> Lock { Content::mysticForgeEntriesLock };
        Content::MysticForgeEntry const& MysticForgeEntry = Content::mysticForgeEntries.at(MysticForgeEntryID);
    };
    Controls::Table<Content::MysticForgeEntryID, decltype(FilteredList)&, DrawContext const&> Table { *this };

    MysticForgeEntryListViewer(uint32 id, bool newTab) : Base(id, newTab)
    {
        Table.SetShowFilterRow();
        Table.AddColumn("Gw2",
        {
            .Width = 20,
            .Filter = [](Content::MysticForgeEntryID id) { return id.GameBuild; },
            .Sort = [](decltype(FilteredList)& data, bool invert) { std::ranges::sort(data, [invert](auto a, auto b) { return a < b ^ invert; }); },
            .Draw = [](DrawContext const& context)
            {
                auto const* currentViewer = G::UI.GetCurrentViewer<MysticForgeEntryViewer>();
                I::SetNextItemAllowOverlap();
                I::Selectable(std::format("  <c=#8>{}</c>", context.MysticForgeEntryID.GameBuild).c_str(), currentViewer && currentViewer->MysticForgeEntryID == context.MysticForgeEntryID ? ImGuiTreeNodeFlags_Selected : 0, ImGuiSelectableFlags_SpanAllColumns);

                Controls::CompletionMargin(context.MysticForgeEntry.GetCompleteness());

                if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                    MysticForgeEntryViewer::Open(context.MysticForgeEntryID, { .MouseButton = button });
            },
        });
        Table.AddColumn("~UID",
        {
            .Width = 50,
            .Filter = [](Content::MysticForgeEntryID id) { return id.UID; },
            .Sort = [](decltype(FilteredList)& data, bool invert) { Utils::Sort::ComplexSort(data, invert, [](Content::MysticForgeEntryID id) { return std::make_tuple(id.UID, id.GameBuild); }); },
            .Draw = [](DrawContext const& context) { I::Text("<c=#8>%u</c>", context.MysticForgeEntryID.UID); },
        });
        Table.AddColumn("GameMode",
        {
            .Width = 20,
            .Filter = [](Content::MysticForgeEntryID id) { return Content::mysticForgeEntries.at(id).GameMode; },
            .Draw = [](DrawContext const& context) { I::Text("<c=#%s>%s</c>", context.MysticForgeEntry.GameMode ? "F" : "4", context.MysticForgeEntry.GameMode == 0 ? "PvE" : context.MysticForgeEntry.GameMode == 1 ? "PvP" : "???"); },
        });
        for (uint32 i = 0; i < std::tuple_size_v<decltype(Content::MysticForgeEntry::Ingredients)>; ++i)
        {
            using enum Data::Content::QueryPurpose;
            std::array<uint32, std::tuple_size_v<decltype(Content::MysticForgeEntry::Ingredients)>> indexes;
            std::ranges::iota(indexes, 0);
            std::rotate(indexes.begin(), indexes.begin() + i, indexes.begin() + i + 1); // Move the i-th element to the front, basically produces (0,1,2,3), (1,0,2,3), (2,0,1,3), (3,0,1,2)
            Table.AddColumn(std::format("Ingredient##{}", i),
            {
                .Width = -1,
                .Filter = [i](Content::MysticForgeEntryID id) { return Content::mysticForgeEntries.at(id).Ingredient(i, Search); },
                .Sort = [indexes](decltype(FilteredList)& data, bool invert)
                {
                    Utils::Sort::ComplexSort(data, invert, [indexes](Content::MysticForgeEntryID id)
                    {
                        auto const& entry = Content::mysticForgeEntries.at(id);
                        return std::make_tuple(
                            I::StripMarkup(entry.Ingredient(indexes[0], Sort)),
                            I::StripMarkup(entry.Ingredient(indexes[1], Sort)),
                            I::StripMarkup(entry.Ingredient(indexes[2], Sort)),
                            I::StripMarkup(entry.Ingredient(indexes[3], Sort)));
                    });
                },
                .Draw = [i](DrawContext const& context) { I::TextUnformatted(context.MysticForgeEntry.Ingredient(i, Draw).c_str()); },
            });
        }
        Table.AddColumn("Encountered",
        {
            .Width = 80,
            .PreferSortDescending = true,
            .Filter = [](Content::MysticForgeEntryID id) { return Content::mysticForgeEntries.at(id).Encounter.Time; },
            .Draw = [](DrawContext const& context){ Controls::Encounter(context.MysticForgeEntry.Encounter); },
        });

        UpdateSearch();
    }

    void SetResult(Utils::Async::Context context, std::vector<Content::MysticForgeEntryID>&& data)
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
            std::vector<Content::MysticForgeEntryID> data;
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
            std::vector<Content::MysticForgeEntryID> data;
            CHECK_ASYNC;
            {
                std::shared_lock _(Content::mysticForgeEntriesLock);
                data.assign_range(Content::mysticForgeEntries | std::views::filter([limits, &table, textSearch, query = Utils::String::Uppercased(Utils::Encoding::FromUTF8(string))](auto const& pair)
                {
                    if (!((int64)pair.first.UID >= limits.first && (int64)pair.first.UID <= limits.second))
                        return false;

                    if (!table.Filter(pair.first))
                        return false;

                    if (!textSearch)
                        return true;

                    for (uint32 i = 0; i < pair.second.Ingredients.size(); ++i)
                        if (Utils::String::Uppercased(Utils::Encoding::FromUTF8(pair.second.Ingredient(i, Data::Content::QueryPurpose::Search))).contains(query))
                            return true;

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

            if (!G::Game.Content.AreObjectsLoaded())
                return;

            std::shared_lock __(Lock);
            ImGuiListClipper clipper;
            clipper.Begin(FilteredList.size());
            while (clipper.Step())
                for (auto const& mysticForgeEntryID : std::span(FilteredList.begin() + clipper.DisplayStart, FilteredList.begin() + clipper.DisplayEnd))
                    if (scoped::WithID(&mysticForgeEntryID))
                        Table.DrawRow({ .MysticForgeEntryID = mysticForgeEntryID });
        }
    }
};

}
