export module GW2Viewer.UI.Viewers.MysticForgeEntryViewer;
import GW2Viewer.Content;
import GW2Viewer.Content.MysticForgeEntry;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.Controls;
import GW2Viewer.UI.Viewers.ViewerRegistry;
import GW2Viewer.UI.Viewers.ViewerWithHistory;
import GW2Viewer.UI.ImGui;
import std;
#include "Macros.h"

export namespace GW2Viewer::UI::Viewers
{

struct MysticForgeEntryViewer : ViewerWithHistory<MysticForgeEntryViewer, Content::MysticForgeEntryID, { ICON_FA_TOILET " MysticForgeEntry", "MysticForgeEntry", Category::ObjectViewer }>
{
    TargetType MysticForgeEntryID;

    MysticForgeEntryViewer(uint32 id, bool newTab, TargetType mysticForgeEntryID) : Base(id, newTab), MysticForgeEntryID(mysticForgeEntryID) { }

    TargetType GetCurrent() const override { return MysticForgeEntryID; }
    bool IsCurrent(TargetType target) const override { return MysticForgeEntryID == target; }

    std::string Title() override
    {
        return std::format("<c=#4>{} #</c>{}", Base::Title(), MysticForgeEntryID.UID);
    }
    void Draw() override
    {
        std::shared_lock _(Content::mysticForgeEntriesLock);
        auto const& mysticForgeEntry = Content::mysticForgeEntries.at(MysticForgeEntryID);

        if (scoped::Child(I::GetSharedScopeID("MysticForgeEntryViewer"), { }, ImGuiChildFlags_Borders | ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeY))
            DrawHistoryButtons();

        if (scoped::Child("Contents"))
        {
            Controls::Symbol("GameMode", mysticForgeEntry.GameMode);

            if (scoped::WithStyleVar(ImGuiStyleVar_CellPadding, ImVec2()))
            if (scoped::WithStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2()))
            //if (scoped::WithStyleVar(ImGuiStyleVar_CellPadding, I::GetStyle().ItemSpacing / 2))
            if (scoped::Table("Inventory", 3, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_Hideable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_RowBg))
            {
                I::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30);
                I::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, 50);
                I::TableSetupColumn("Ingredients", ImGuiTableColumnFlags_WidthStretch);
                I::TableHeadersRow();

                for (auto const& [index, ingredient] : mysticForgeEntry.Ingredients | std::views::enumerate)
                {
                    I::TableNextRow();

                    I::TableNextColumn();
                    I::AlignTextToFramePadding();
                    I::Text("  <c=#%s>%u</c>", ingredient.IsUnknown() ? "F00" : "4", index);
                    Controls::CompletionMargin(ingredient.GetCompleteness());

                    I::TableNextColumn();
                    if (!ingredient.IsUnknown())
                    {
                        I::AlignTextToFramePadding();
                        I::RightAlignedText("{} <c=#4>x</c>", ingredient.Count);
                    }

                    I::TableNextColumn();
                    switch (ingredient.IngredientType)
                    {
                        case Content::MYSTIC_FORGE_INGREDIENT_TYPE_ATTRIBUTES:
                        {
                            I::AlignTextToFramePadding();
                            I::TextUnformatted("<c=#8>Any</c>");
                            I::SameLine(0, 0); Controls::ValueEnum(ingredient.Attributes.ItemRarity, "ItemRarity");
                            I::SameLine(0, 0); Controls::ValueEnum(ingredient.Attributes.ItemType, "ItemType");

                            auto [armor, consumable, trinket, upgradeComponent, weapon] = Data::Content::TypeInfo::FindEnumValues("ItemType", "Armor", "Consumable", "Trinket", "UpgradeComponent", "Weapon");
                            if (ingredient.Attributes.ItemType == armor)
                            {
                                I::SameLine(0, 0); Controls::ValueEnum(ingredient.Attributes.Armor.Type, "ItemArmorType");
                                I::SameLine(0, 0); Controls::ValueEnum(ingredient.Attributes.Armor.WeightClass, "ItemArmorWeightClass");
                            }
                            else if (ingredient.Attributes.ItemType == consumable)
                            {
                                I::SameLine(0, 0); Controls::ValueEnum(ingredient.Attributes.Consumable.Type, "ItemConsumableType");
                                auto [unlock] = Data::Content::TypeInfo::FindEnumValues("ItemConsumableType", "Unlock");
                                if (ingredient.Attributes.Consumable.Type == unlock)
                                {
                                    I::SameLine(0, 0); Controls::ValueEnum(ingredient.Attributes.Consumable.Unlock.Type, "ItemConsumableUnlockType");
                                }
                            }
                            else if (ingredient.Attributes.ItemType == trinket)
                            {
                                I::SameLine(0, 0); Controls::ValueEnum(ingredient.Attributes.Trinket.Type, "ItemTrinketType");
                            }
                            else if (ingredient.Attributes.ItemType == upgradeComponent)
                            {
                                I::SameLine(0, 0); Controls::ValueEnum(ingredient.Attributes.UpgradeComponent.Type, "ItemUpgradeComponentType");
                            }
                            else if (ingredient.Attributes.ItemType == weapon)
                            {
                                I::SameLine(0, 0); Controls::ValueEnum(ingredient.Attributes.Weapon.Type, "ItemWeaponType");
                            }
                            break;
                        }
                        case Content::MYSTIC_FORGE_INGREDIENT_TYPE_ITEM:
                        {
                            auto const itemDef = G::Game.Content.GetByDataID(Content::ItemDef, ingredient.Item.DataID);
                            Controls::ContentButton(itemDef, &ingredient.Item.DataID);
                            break;
                        }
                        case Content::MYSTIC_FORGE_INGREDIENT_TYPES:
                        {
                            I::AlignTextToFramePadding();
                            I::TextUnformatted("<c=#F004>Unknown Ingredient</c>");
                            break;
                        }
                        default:
                            std::terminate();
                    }
                }
            }
        }
    }
};

}
