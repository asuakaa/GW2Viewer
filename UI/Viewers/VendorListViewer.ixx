export module GW2Viewer.UI.Viewers.VendorListViewer;
import GW2Viewer.Common;
import GW2Viewer.Content;
import GW2Viewer.Content.Vendor;
import GW2Viewer.Data.Content;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.Controls;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Manager;
import GW2Viewer.UI.Viewers.ListViewer;
import GW2Viewer.UI.Viewers.VendorViewer;
import GW2Viewer.UI.Viewers.ViewerRegistry;
import GW2Viewer.Utils.Async;
import GW2Viewer.Utils.Encoding;
import GW2Viewer.Utils.Scan;
import GW2Viewer.Utils.String;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Viewers
{

struct VendorListViewer : ListViewer<VendorListViewer, { ICON_FA_SACK " Vendors", "Vendors", Category::ListViewer }>
{
    std::shared_mutex Lock;
    std::vector<uint64> FilteredList;
    Utils::Async::Scheduler AsyncFilter { true };

    std::string FilterString;
    std::optional<std::pair<int64, int64>> FilterID;
    uint64 FilterRange { };

    struct DrawContext
    {
        uint64 VendorHash;
        std::shared_lock<decltype(Content::vendorsLock)> Lock { Content::vendorsLock };
        Content::Vendor const& Vendor = Content::vendors.at(VendorHash);
    };
    Controls::Table<uint64, decltype(FilteredList)&, DrawContext const&> Table { *this };

    VendorListViewer(uint32 id, bool newTab) : Base(id, newTab)
    {
        Table.SetShowFilterRow();
        Table.AddColumn("#",
        {
            .Width = 50,
            .Filter = [](uint64 id) { return id; },
            .Sort = [](decltype(FilteredList)& data, bool invert) { std::ranges::sort(data, [invert](auto a, auto b) { return a < b ^ invert; }); },
            .Draw = [](DrawContext const& context)
            {
                auto const* currentViewer = G::UI.GetCurrentViewer<VendorViewer>();
                I::SetNextItemAllowOverlap();
                I::Selectable(std::format("  <c=#4>{}</c>", context.VendorHash).c_str(), currentViewer && currentViewer->VendorHash == context.VendorHash ? ImGuiTreeNodeFlags_Selected : 0, ImGuiSelectableFlags_SpanAllColumns);

                Controls::CompletionMargin(context.Vendor.GetCompleteness());

                if (auto const button = I::IsItemMouseClickedWith(ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle))
                    VendorViewer::Open(context.VendorHash, { .MouseButton = button });
            },
        });
        Table.AddColumn("Name",
        {
            .Width = -1,
            .Filter = [](uint64 id) { return Content::vendors.at(id).Name(); },
            .Draw = [](DrawContext const& context) { I::TextUnformatted(context.Vendor.Name().c_str()); },
        });
        Table.AddColumn("Service Tabs",
        {
            .Width = -3,
            .Filter = [](uint64 id) { return Content::vendors.at(id).TabNames(); },
            .Draw = [](DrawContext const& context) { I::TextUnformatted(context.Vendor.TabNames().c_str()); },
        });
        Table.AddColumn("Encountered",
        {
            .Width = 80,
            .PreferSortDescending = true,
            .Filter = [](uint64 id) { return Content::vendors.at(id).Encounter.Time; },
            .Draw = [](DrawContext const& context) { Controls::Encounter(context.Vendor.Encounter); },
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
                std::shared_lock _(Content::vendorsLock);
                data.assign_range(Content::vendors | std::views::filter([limits, &table, textSearch, query = Utils::String::Uppercased(Utils::Encoding::FromUTF8(string))](auto const& pair)
                {
                    if ((int64)pair.first < limits.first || (int64)pair.first > limits.second)
                        return false;

                    if (!table.Filter(pair.first))
                        return false;

                    if (!textSearch)
                        return true;

                    if (auto const string = G::Game.Text.GetNormalized(pair.second.NameTextID).first; string && !string->empty() && string->contains(query))
                        return true;
                    for (auto const& tab : pair.second.ServiceTabs)
                    {
                        if (auto const string = G::Game.Text.GetNormalized(tab.NameTextID).first; string && !string->empty() && string->contains(query))
                            return true;
                        for (auto const& item : tab.InventoryItems)
                        {
                            if (auto const itemDef = G::Game.Content.GetByDataID(Content::ItemDef, item.ItemDefDataID))
                                if (Utils::String::Uppercased(itemDef->GetDisplayName(Data::Content::QueryPurpose::Search)).contains(query))
                                    return true;
                            if (auto const string = G::Game.Text.GetNormalized(item.UnlockTextID).first; string && !string->empty() && string->contains(query))
                                return true;
                        }
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
        if (Controls::SearchInput(FilterString, FilteredList, Lock, &AsyncFilter))
            UpdateSearch();

        if (scoped::TableList("Table", Table.GetColumnCount()))
        {
            Table.DrawColumnHeaders();

            std::shared_lock __(Lock);
            ImGuiListClipper clipper;
            clipper.Begin(FilteredList.size());
            while (clipper.Step())
                for (auto vendorHash : std::span(FilteredList.begin() + clipper.DisplayStart, FilteredList.begin() + clipper.DisplayEnd))
                    if (scoped::WithID(vendorHash))
                        Table.DrawRow({ .VendorHash = vendorHash });
        }
    }
};

}
