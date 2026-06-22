export module GW2Viewer.UI.Viewers.ConversationListViewer;
import GW2Viewer.Common;
import GW2Viewer.Content.Conversation;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.Controls;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Manager;
import GW2Viewer.UI.Viewers.ConversationViewer;
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

struct ConversationListViewer : ListViewer<ConversationListViewer, { ICON_FA_COMMENT_CHECK " Conversations", "Conversations", Category::ListViewer }>
{
    std::shared_mutex Lock;
    std::vector<uint32> FilteredList;
    Utils::Async::Scheduler AsyncFilter { true };

    std::string FilterString;
    std::optional<std::pair<int32, int32>> FilterID;
    uint32 FilterRange { };

    struct DrawContext
    {
        uint32 ConversationID;
        std::shared_lock<decltype(Content::conversationsLock)> Lock { Content::conversationsLock };
        Content::Conversation const& Conversation = Content::conversations.at(ConversationID);
    };
    Controls::Table<uint32, decltype(FilteredList)&, DrawContext const&> Table { *this };

    ConversationListViewer(uint32 id, bool newTab) : Base(id, newTab)
    {
        Table.SetShowFilterRow();
        Table.AddColumn("#",
        {
            .Width = 50,
            .Filter = [](uint32 id) { return id; },
            .Sort = [](decltype(FilteredList)& data, bool invert) { std::ranges::sort(data, [invert](auto a, auto b) { return a < b ^ invert; }); },
            .Draw = [](DrawContext const& context)
            {
                auto const* currentViewer = G::UI.GetCurrentViewer<ConversationViewer>();
                I::SetNextItemAllowOverlap();
                I::Selectable(std::format("  <c=#4>{}</c>", context.ConversationID).c_str(), currentViewer && currentViewer->ConversationID == context.ConversationID ? ImGuiTreeNodeFlags_Selected : 0, ImGuiSelectableFlags_SpanAllColumns);

                Controls::CompletionMargin(context.Conversation.GetCompleteness());

                if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                    ConversationViewer::Open(context.ConversationID, { .MouseButton = button });
            },
        });
        Table.AddColumn("~UID",
        {
            .Width = 50,
            .Filter = [](uint32 id) { return Content::conversations.at(id).UID; },
            .Draw = [](DrawContext const& context) { I::Text("<c=#8>%u</c>", context.Conversation.UID); },
        });
        Table.AddColumn("Speaker Name",
        {
            .Width = -1,
            .Filter = [](uint32 id) { return Content::conversations.at(id).StartingSpeakerName(); },
            .Draw = [](DrawContext const& context) { I::TextUnformatted(context.Conversation.StartingSpeakerName().c_str()); },
        });
        Table.AddColumn("Start State Text",
        {
            .Width = -1,
            .Filter = [](uint32 id) { return Content::conversations.at(id).StartingStateText(); },
            .Draw = [](DrawContext const& context) { I::TextUnformatted(context.Conversation.StartingStateText().c_str()); },
        });
        Table.AddColumn("Encountered",
        {
            .Width = 80,
            .PreferSortDescending = true,
            .Filter = [](uint32 id) { return Content::conversations.at(id).Encounter.Time; },
            .Draw = [](DrawContext const& context){ Controls::Encounter(context.Conversation.Encounter); },
        });

        UpdateSearch();
    }

    void SetResult(Utils::Async::Context context, std::vector<uint32>&& data)
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
            std::vector<uint32> data;
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

        AsyncFilter.Run([this, textSearch, filter = FilterID, string = FilterString, table = Table.Prepare()](Utils::Async::Context context) mutable
        {
            context->SetIndeterminate();
            auto const limits = filter.value_or(std::pair { std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max() });
            std::vector<uint32> data;
            CHECK_ASYNC;
            {
                std::shared_lock _(Content::conversationsLock);
                data.assign_range(Content::conversations | std::views::filter([limits, &table, textSearch, query = Utils::String::Uppercased(Utils::Encoding::FromUTF8(string))](auto const& pair)
                {
                    if (!((int32)pair.first >= limits.first && (int32)pair.first <= limits.second || (int32)pair.second.UID >= limits.first && (int32)pair.second.UID <= limits.second))
                        return false;

                    if (!table.Filter(pair.first))
                        return false;

                    if (!textSearch)
                        return true;

                    for (auto const& state : pair.second.States)
                    {
                        if (auto const string = G::Game.Text.GetNormalized(state.TextID).first; string && !string->empty() && string->contains(query))
                            return true;
                        if (auto const string = G::Game.Text.GetNormalized(state.SpeakerNameTextID).first; string && !string->empty() && string->contains(query))
                            return true;
                        for (auto const& transition : state.Transitions)
                            if (auto const string = G::Game.Text.GetNormalized(transition.TextID).first; string && !string->empty() && string->contains(query))
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
                for (auto conversationID : std::span(FilteredList.begin() + clipper.DisplayStart, FilteredList.begin() + clipper.DisplayEnd))
                    if (scoped::WithID(conversationID))
                        Table.DrawRow({ .ConversationID = conversationID });
        }
    }
};

}
