export module GW2Viewer.UI.Viewers.MysticForgeEntryListViewer;
import GW2Viewer.Common;
import GW2Viewer.Common.Time;
import GW2Viewer.Content;
import GW2Viewer.Content.MysticForgeEntry;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.Controls;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Manager;
import GW2Viewer.UI.Viewers.ListViewer;
import GW2Viewer.UI.Viewers.MysticForgeEntryViewer;
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

struct MysticForgeEntryListViewer : ListViewer<MysticForgeEntryListViewer, { ICON_FA_TOILET " MysticForgeEntries", "MysticForgeEntries", Category::ListViewer }>
{
    MysticForgeEntryListViewer(uint32 id, bool newTab) : Base(id, newTab) { UpdateSearch(); }

    std::shared_mutex Lock;
    std::vector<Content::MysticForgeEntryID> FilteredList;
    Utils::Async::Scheduler AsyncFilter { true };

    std::string FilterString;
    std::optional<std::pair<int64, int64>> FilterID;
    uint64 FilterRange { };
    enum class MysticForgeEntrySort { GameBuild, UID, GameMode, Ingredient0, Ingredient1, Ingredient2, Ingredient3, Encounter } Sort { MysticForgeEntrySort::UID };
    bool SortInvert { };

    static auto SortList(Utils::Async::Context context, std::vector<Content::MysticForgeEntryID>& data, MysticForgeEntrySort sort, bool invert)
    {
        std::shared_lock _(Content::mysticForgeEntriesLock);
        switch (sort)
        {
            using Utils::Sort::ComplexSort;
            using enum MysticForgeEntrySort;
            case GameBuild:     std::ranges::sort(data, [invert](auto a, auto b) { return a < b ^ invert; }); break;
            case UID:           ComplexSort(data, invert, [](Content::MysticForgeEntryID id) { return std::make_tuple(id.UID, id.GameBuild); }); break;
            case Ingredient0:   ComplexSort(data, invert, [](Content::MysticForgeEntryID id) { auto const& e = Content::mysticForgeEntries.at(id); return std::make_tuple(I::StripMarkup(e.Ingredient(0)), I::StripMarkup(e.Ingredient(1)), I::StripMarkup(e.Ingredient(2)), I::StripMarkup(e.Ingredient(3))); }); break;
            case Ingredient1:   ComplexSort(data, invert, [](Content::MysticForgeEntryID id) { auto const& e = Content::mysticForgeEntries.at(id); return std::make_tuple(I::StripMarkup(e.Ingredient(1)), I::StripMarkup(e.Ingredient(0)), I::StripMarkup(e.Ingredient(2)), I::StripMarkup(e.Ingredient(3))); }); break;
            case Ingredient2:   ComplexSort(data, invert, [](Content::MysticForgeEntryID id) { auto const& e = Content::mysticForgeEntries.at(id); return std::make_tuple(I::StripMarkup(e.Ingredient(2)), I::StripMarkup(e.Ingredient(0)), I::StripMarkup(e.Ingredient(1)), I::StripMarkup(e.Ingredient(3))); }); break;
            case Ingredient3:   ComplexSort(data, invert, [](Content::MysticForgeEntryID id) { auto const& e = Content::mysticForgeEntries.at(id); return std::make_tuple(I::StripMarkup(e.Ingredient(3)), I::StripMarkup(e.Ingredient(0)), I::StripMarkup(e.Ingredient(1)), I::StripMarkup(e.Ingredient(2))); }); break;
            case Encounter:     ComplexSort(data, invert, [](Content::MysticForgeEntryID id) { return Content::mysticForgeEntries.at(id).Encounter.Time; }); break;
            default: std::terminate();
        }
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

        AsyncFilter.Run([this, sort = Sort, invert = SortInvert](Utils::Async::Context context)
        {
            context->SetIndeterminate();
            std::vector<Content::MysticForgeEntryID> data;
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
            std::vector<Content::MysticForgeEntryID> data;
            CHECK_ASYNC;
            if (textSearch)
            {
                std::shared_lock _(Content::mysticForgeEntriesLock);
                data.assign_range(Content::mysticForgeEntries | std::views::keys | std::views::filter([query = Utils::String::Uppercased(Utils::Encoding::FromUTF8(string))](Content::MysticForgeEntryID id)
                {
                    auto const& mysticForgeEntry = Content::mysticForgeEntries.at(id);
                    for (uint32 i = 0; i < mysticForgeEntry.Ingredients.size(); ++i)
                        if (Utils::String::Uppercased(Utils::Encoding::FromUTF8(mysticForgeEntry.Ingredient(i))).contains(query))
                            return true;

                    return false;
                }));
            }
            else
            {
                auto limits = filter.value_or(std::pair { std::numeric_limits<int64>::min(), std::numeric_limits<int64>::max() });
                std::shared_lock _(Content::mysticForgeEntriesLock);
                data.assign_range(Content::mysticForgeEntries | std::views::filter([limits](auto const& pair) { return (int64)pair.first.UID >= limits.first && (int64)pair.first.UID <= limits.second; }) | std::views::keys);
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

        if (scoped::TableList("Table", 8))
        {
            I::TableSetupColumn("Gw2", ImGuiTableColumnFlags_WidthFixed, 20, (ImGuiID)MysticForgeEntrySort::GameBuild);
            I::TableSetupColumn("~UID", ImGuiTableColumnFlags_WidthFixed, 50, (ImGuiID)MysticForgeEntrySort::UID);
            I::TableSetupColumn("GameMode", ImGuiTableColumnFlags_WidthFixed, 20, (ImGuiID)MysticForgeEntrySort::GameMode);
            I::TableSetupColumn("Ingredient##0", ImGuiTableColumnFlags_WidthStretch, 0, (ImGuiID)MysticForgeEntrySort::Ingredient0);
            I::TableSetupColumn("Ingredient##1", ImGuiTableColumnFlags_WidthStretch, 0, (ImGuiID)MysticForgeEntrySort::Ingredient1);
            I::TableSetupColumn("Ingredient##2", ImGuiTableColumnFlags_WidthStretch, 0, (ImGuiID)MysticForgeEntrySort::Ingredient2);
            I::TableSetupColumn("Ingredient##3", ImGuiTableColumnFlags_WidthStretch, 0, (ImGuiID)MysticForgeEntrySort::Ingredient3);
            I::TableSetupColumn("Encounter", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, 80, (ImGuiID)MysticForgeEntrySort::Encounter);
            I::TableSetupScrollFreeze(0, 1);
            I::TableHeadersRow();
            HandleTableSort(Sort, SortInvert);

            if (!G::Game.Content.AreObjectsLoaded())
                return;

            std::shared_lock __(Lock);
            ImGuiListClipper clipper;
            clipper.Begin(FilteredList.size());
            while (clipper.Step())
            {
                for (auto const& id : std::span(FilteredList.begin() + clipper.DisplayStart, FilteredList.begin() + clipper.DisplayEnd))
                {
                    scoped::WithID(&id);

                    std::shared_lock ___(Content::mysticForgeEntriesLock);
                    auto& mysticForgeEntry = Content::mysticForgeEntries.at(id);
                    auto const* currentViewer = G::UI.GetCurrentViewer<MysticForgeEntryViewer>();
                    I::TableNextRow();

                    I::TableNextColumn();
                    I::SetNextItemAllowOverlap();
                    I::Selectable(std::format("  <c=#8>{}</c>", id.GameBuild).c_str(), currentViewer && currentViewer->MysticForgeEntryID == id ? ImGuiTreeNodeFlags_Selected : 0, ImGuiSelectableFlags_SpanAllColumns);

                    Controls::CompletionMargin(mysticForgeEntry.GetCompleteness());

                    if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                        MysticForgeEntryViewer::Open(id, { .MouseButton = button });

                    I::TableNextColumn();
                    I::Text("<c=#8>%u</c>", id.UID);

                    I::TableNextColumn();
                    I::Text("<c=#%s>%s</c>", mysticForgeEntry.GameMode ? "F" : "4", mysticForgeEntry.GameMode == 0 ? "PvE" : mysticForgeEntry.GameMode == 1 ? "PvP" : "???");

                    I::TableNextColumn();
                    I::TextUnformatted(mysticForgeEntry.Ingredient(0).c_str());

                    I::TableNextColumn();
                    I::TextUnformatted(mysticForgeEntry.Ingredient(1).c_str());

                    I::TableNextColumn();
                    I::TextUnformatted(mysticForgeEntry.Ingredient(2).c_str());

                    I::TableNextColumn();
                    I::TextUnformatted(mysticForgeEntry.Ingredient(3).c_str());

                    I::TableNextColumn();
                    Controls::Encounter(mysticForgeEntry.Encounter);
                }
            }
        }
    }
};

}
