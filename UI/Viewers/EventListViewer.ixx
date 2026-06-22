export module GW2Viewer.UI.Viewers.EventListViewer;
import GW2Viewer.Common;
import GW2Viewer.Content.Event;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.Controls;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Manager;
import GW2Viewer.UI.Viewers.EventViewer;
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

struct EventListViewer : ListViewer<EventListViewer, { ICON_FA_SEAL " Events", "Events", Category::ListViewer }>
{
    std::shared_mutex Lock;
    std::vector<Content::EventID> FilteredList;
    Utils::Async::Scheduler AsyncFilter { true };

    std::string FilterString;
    std::optional<std::pair<int32, int32>> FilterID;
    uint32 FilterRange { };
    struct { bool Normal = true, Group = true, Meta = true, Dungeon = true, NonEvent = true; } Filters;

    struct DrawContext
    {
        Content::EventID EventID;
        std::shared_lock<decltype(Content::eventsLock)> Lock { Content::eventsLock };
        Content::Event const& Event = Content::events.at(EventID);
    };
    Controls::Table<Content::EventID, decltype(FilteredList)&, DrawContext const&> Table { *this };

    EventListViewer(uint32 id, bool newTab) : Base(id, newTab)
    {
        Table.SetShowFilterRow();
        Table.AddColumn("ID",
        {
            .Width = 50,
            .Filter = [](Content::EventID id) { return id.UID; },
            .Sort = [](decltype(FilteredList)& data, bool invert) { std::ranges::sort(data, [invert](auto a, auto b) { return a.UID < b.UID ^ invert; }); },
            .Draw = [](DrawContext const& context)
            {
                auto const* currentViewer = G::UI.GetCurrentViewer<EventViewer>();
                I::SetNextItemAllowOverlap();
                I::Selectable(std::format("{}", context.EventID.UID).c_str(), currentViewer && currentViewer->EventID == context.EventID ? ImGuiTreeNodeFlags_Selected : 0, ImGuiSelectableFlags_SpanAllColumns);
                if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                    EventViewer::Open(context.EventID, { .MouseButton = button });
            },
        });
        Table.AddColumn("Map",
        {
            .Width = -1,
            .Filter = [](Content::EventID id) { return Content::events.at(id).Map(Data::Content::QueryPurpose::Sort); },
            .Draw = [](DrawContext const& context) { I::TextUnformatted(Utils::Encoding::ToUTF8(context.Event.Map(Data::Content::QueryPurpose::Draw)).c_str()); },
        });
        Table.AddColumn("Type",
        {
            .Width = 50,
            .Filter = [](Content::EventID id) { return Content::events.at(id).Type(); },
            .Draw = [](DrawContext const& context) { I::TextUnformatted(context.Event.Type().c_str()); },
        });
        Table.AddColumn("Title",
        {
            .Width = -1,
            .Filter = [](Content::EventID id) { return Content::events.at(id).Title(); },
            .Draw = [](DrawContext const& context) { I::TextUnformatted(Utils::Encoding::ToUTF8(context.Event.Title()).c_str()); },
        });
        Table.AddColumn("Encountered",
        {
            .Width = 80,
            .PreferSortDescending = true,
            .Filter = [](Content::EventID id) { return Content::events.at(id).EncounteredTime(); },
            .Draw = [](DrawContext const& context){ Controls::Encounter({ context.Event.EncounteredTime() }); },
        });

        UpdateSearch();
    }

    void SetResult(Utils::Async::Context context, std::vector<Content::EventID>&& data)
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
            std::vector<Content::EventID> data;
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
        else if (uint32 id, range; Utils::Scan::Into(FilterString, "{}+{}", id, range))
            FilterID.emplace(id - range, id + range);
        else if (Utils::Scan::Into(FilterString, "{}-{}", id, range))
            FilterID.emplace(id, range);
        else if (Utils::Scan::NumberLiteral(FilterString, id))
            FilterID.emplace(id - FilterRange, id + FilterRange);
        else if (Utils::Scan::NumberLiteral(FilterString, "0x{:x}", id))
            FilterID.emplace(id - FilterRange, id + FilterRange);
        else
            textSearch = true;

        AsyncFilter.Run([this, textSearch, filter = FilterID, string = FilterString, filters = Filters, table = Table.Prepare()](Utils::Async::Context context) mutable
        {
            context->SetIndeterminate();
            std::vector<Content::EventID> data;
            if (!textSearch)
            {
                auto limits = filter.value_or(std::pair { std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max() });
                std::shared_lock _(Content::eventsLock);
                data.assign_range(Content::events | std::views::keys | std::views::filter([limits](Content::EventID id) { return (int32)id.UID >= limits.first && (int32)id.UID <= limits.second; }));
            }
            else
            {
                std::shared_lock _(Content::eventsLock);
                data.assign_range(Content::events | std::views::keys);
            }
            CHECK_ASYNC;
            if (!(filters.Normal && filters.Group && filters.Meta && filters.Dungeon && filters.NonEvent))
            {
                std::shared_lock _(Content::eventsLock);
                std::erase_if(data, [&filters](Content::EventID id)
                {
                    auto const& event = Content::events.at(id);
                    return !(
                        !id.UID && filters.NonEvent ||
                        id.UID && std::ranges::any_of(event.States, &Content::Event::State::IsDungeonEvent) && filters.Dungeon ||
                        id.UID && std::ranges::any_of(event.States, &Content::Event::State::IsMetaEvent) && filters.Meta ||
                        id.UID && std::ranges::any_of(event.States, &Content::Event::State::IsGroupEvent) && filters.Group ||
                        id.UID && std::ranges::any_of(event.States, &Content::Event::State::IsNormalEvent) && filters.Normal
                    );
                });
            }
            CHECK_ASYNC;
            if (table.HasFilterTerms() || textSearch)
            {
                std::shared_lock _(Content::eventsLock);
                std::erase_if(data, [&table, textSearch, query = Utils::String::Uppercased(Utils::Encoding::FromUTF8(string))](Content::EventID id)
                {
                    if (!table.Filter(id))
                        return true;

                    if (!textSearch)
                        return false;

                    auto const& event = Content::events.at(id);
                    for (auto const& state : event.States)
                    {
                        if (auto const string = G::Game.Text.GetNormalized(state.TitleTextID).first; string && !string->empty() && string->contains(query))
                            return false;
                        for (auto const& param : state.TitleParameterTextID)
                            if (auto const string = G::Game.Text.GetNormalized(param).first; string && !string->empty() && string->contains(query))
                                return false;
                        if (auto const string = G::Game.Text.GetNormalized(state.DescriptionTextID).first; string && !string->empty() && string->contains(query))
                            return false;
                        if (auto const string = G::Game.Text.GetNormalized(state.MetaTextTextID).first; string && !string->empty() && string->contains(query))
                            return false;
                    }
                    for (auto const& objective : event.Objectives)
                    {
                        if (auto const string = G::Game.Text.GetNormalized(objective.TextID).first; string && !string->empty() && string->contains(query))
                            return false;
                        if (auto const string = G::Game.Text.GetNormalized(objective.AgentNameTextID).first; string && !string->empty() && string->contains(query))
                            return false;
                    }
                    if (Utils::String::Uppercased(event.Map(Data::Content::QueryPurpose::Search)).contains(query))
                        return false;
                    return true;
                });
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

        auto filter = [&, next = false](std::string_view text, bool& filter) mutable
        {
            if (std::exchange(next, true))
                I::SameLine();
            if (I::Button(std::format("<c=#{}><c=#8>{}</c> {}</c>###EventFilter{}", filter ? "F" : "4", ICON_FA_FILTER, text, text).c_str()))
            {
                filter ^= true;
                UpdateFilter();
            }
        };
        filter("Normal", Filters.Normal);
        filter("Group", Filters.Group);
        filter("Meta", Filters.Meta);
        filter("Dungeon", Filters.Dungeon);
        filter("Non-Event", Filters.NonEvent);

        if (scoped::TableList("Table", Table.GetColumnCount()))
        {
            Table.DrawColumnHeaders();

            std::shared_lock __(Lock);
            ImGuiListClipper clipper;
            clipper.Begin(FilteredList.size());
            while (clipper.Step())
                for (auto eventID : std::span(FilteredList.begin() + clipper.DisplayStart, FilteredList.begin() + clipper.DisplayEnd))
                    if (scoped::WithID(eventID.Map << 17 | eventID.UID))
                        Table.DrawRow({ .EventID = eventID });
        }
    }
};

}
