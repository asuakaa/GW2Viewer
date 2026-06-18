export module GW2Viewer.Content.MysticForgeEntry;
import GW2Viewer.Common;
import GW2Viewer.Common.Time;
import GW2Viewer.Content;
import GW2Viewer.Data.Content;
import GW2Viewer.Data.External.Types;
import GW2Viewer.Data.Game;
import GW2Viewer.Data.Text.Format;
import GW2Viewer.Utils.Encoding;
import std;

export namespace GW2Viewer::Content
{

enum
{
    MYSTIC_FORGE_INGREDIENTS_REQUIRED = 4,
};
enum MysticForgeIngredientType : uint32
{
    MYSTIC_FORGE_INGREDIENT_TYPE_ATTRIBUTES,
    MYSTIC_FORGE_INGREDIENT_TYPE_ITEM,
    MYSTIC_FORGE_INGREDIENT_TYPES
};
struct MysticForgeEntryID
{
    uint32 GameBuild;
    uint32 UID;

    auto operator<=>(MysticForgeEntryID const& other) const = default;
};
struct MysticForgeEntry
{
    enum Completeness
    {
        COMPLETENESS_UNKNOWN_INGREDIENT,
        COMPLETENESS_COMPLETE,
    };

    #pragma pack(push, 1)
    struct Ingredient
    {
        byte IngredientType = MYSTIC_FORGE_INGREDIENT_TYPES;
        byte Count = 0;
        union
        {
            uint32 Raw = 0;
            struct
            {
                byte ItemType;
                byte ItemRarity;
                union
                {
                    struct
                    {
                        byte Type;
                        byte WeightClass;
                    } Armor;
                    struct
                    {
                        byte Type;
                        union
                        {
                            struct
                            {
                                byte Type;
                            } Unlock;
                        };
                    } Consumable;
                    struct
                    {
                        byte Type;
                    } Trinket;
                    struct
                    {
                        byte Type;
                    } UpgradeComponent;
                    struct
                    {
                        byte Type;
                    } Weapon;
                };
            } Attributes;
            struct
            {
                uint32 DataID;
            } Item;
        };

        bool IsUnknown() const { return IngredientType == MYSTIC_FORGE_INGREDIENT_TYPES; }
        Completeness GetCompleteness() const
        {
            if (IsUnknown())
                return COMPLETENESS_UNKNOWN_INGREDIENT;
            return COMPLETENESS_COMPLETE;
        }
    };
    static_assert(sizeof(Ingredient) == 6);
    #pragma pack(pop)

    uint32 GameMode;
    std::array<Ingredient, MYSTIC_FORGE_INGREDIENTS_REQUIRED> Ingredients { };

    mutable Data::External::Encounter Encounter;

    Completeness GetCompleteness() const { return std::ranges::min(Ingredients | std::views::transform(&Ingredient::GetCompleteness)); }

    std::string Ingredient(uint32 index, Data::Content::QueryPurpose purpose) const
    {
        std::string result;
        result.reserve(100);
        switch (auto const& ingredient = Ingredients.at(index); ingredient.IngredientType)
        {
            case MYSTIC_FORGE_INGREDIENT_TYPE_ATTRIBUTES:
            {
                std::array rarityColors { 0xA9A9A9, 0xFFFFFF, 0x4F9DFE, 0x2DC50E, 0xFFE51F, 0xFDA500, 0xFB3E8D, 0xA02EF7 };
                result += std::format("<c=#{:06X}>", rarityColors.at(ingredient.Attributes.ItemRarity));

                auto [armor, consumable, trinket, upgradeComponent, weapon] = Data::Content::TypeInfo::FindEnumValues("ItemType", "Armor", "Consumable", "Trinket", "UpgradeComponent", "Weapon");
                if (ingredient.Attributes.ItemType == armor)
                {
                    bool written = false;
                    if (ingredient.Attributes.Armor.WeightClass != 4)
                        result += (std::exchange(written, true), Data::Content::TypeInfo::FindEnumName("ItemArmorWeightClass", ingredient.Attributes.Armor.WeightClass).value_or("ItemArmorWeightClass?"));
                    if (ingredient.Attributes.Armor.Type != 8)
                        result += (std::exchange(written, true) && (result += " ", true), Data::Content::TypeInfo::FindEnumName("ItemArmorType", ingredient.Attributes.Armor.Type).value_or("ItemArmorType?"));
                    if (!written)
                        result += Data::Content::TypeInfo::FindEnumName("ItemType", ingredient.Attributes.ItemType).value_or("ItemType?");
                }
                else if (ingredient.Attributes.ItemType == consumable)
                {
                    auto [unlock] = Data::Content::TypeInfo::FindEnumValues("ItemConsumableType", "Unlock");
                    if (ingredient.Attributes.Consumable.Type == unlock)
                        result += Data::Content::TypeInfo::FindEnumName("ItemConsumableUnlockType", ingredient.Attributes.Consumable.Unlock.Type).value_or("ItemConsumableUnlockType?");
                    else
                        result += Data::Content::TypeInfo::FindEnumName("ItemConsumableType", ingredient.Attributes.Consumable.Type).value_or("ItemConsumableType?");
                }
                else if (ingredient.Attributes.ItemType == trinket && ingredient.Attributes.Trinket.Type != 3)
                    result += Data::Content::TypeInfo::FindEnumName("ItemTrinketType", ingredient.Attributes.Trinket.Type).value_or("ItemTrinketType?");
                else if (ingredient.Attributes.ItemType == upgradeComponent && ingredient.Attributes.UpgradeComponent.Type != 4)
                    result += Data::Content::TypeInfo::FindEnumName("ItemUpgradeComponentType", ingredient.Attributes.UpgradeComponent.Type).value_or("ItemUpgradeComponentType?");
                else if (ingredient.Attributes.ItemType == weapon && ingredient.Attributes.Weapon.Type != 24)
                    result += Data::Content::TypeInfo::FindEnumName("ItemWeaponType", ingredient.Attributes.Weapon.Type).value_or("ItemWeaponType?");
                else
                    result += Data::Content::TypeInfo::FindEnumName("ItemType", ingredient.Attributes.ItemType).value_or("ItemType?");

                result += "</c>";
                if (ingredient.Count != 1)
                    result += std::format(" <c=#4>x</c> <c=#8>{}</c>", ingredient.Count);
                break;
            }
            case MYSTIC_FORGE_INGREDIENT_TYPE_ITEM:
            {
                if (auto const itemDef = G::Game.Content.GetByDataID(ItemDef, ingredient.Item.DataID))
                {
                    if (auto const icon = itemDef->GetIcon())
                        result += std::format("<img={}/>", icon);

                    result += std::format("{}", Utils::Encoding::ToUTF8(Data::Text::FormatStringEmpty(itemDef->GetDisplayName(purpose))));
                }
                if (ingredient.Count != 1)
                    result += std::format(" <c=#4>x</c> <c=#8>{}</c>", ingredient.Count);
                break;
            }
            case MYSTIC_FORGE_INGREDIENT_TYPES:
            {
                result += "<c=#F00>???</c>";
                break;
            }
            default:
                std::terminate();
        }
        return result;
    }
    std::string Text(Data::Content::QueryPurpose purpose) const
    {
        std::string result;
        result.reserve(400);
        for (uint32 i = 0; i < Ingredients.size(); ++i)
            result += std::format("{}{}", i ? " <c=#8>|</c> " : "", Ingredient(i, purpose));
        return result;
    }
};
std::map<MysticForgeEntryID, MysticForgeEntry> mysticForgeEntries;
std::shared_mutex mysticForgeEntriesLock;

}
