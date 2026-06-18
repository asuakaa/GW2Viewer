module;
#include <cwctype>

module GW2Viewer.Data.Content;
import :ContentFilter;
import :ContentObject;
import GW2Viewer.Data.Content.Mangling;
import GW2Viewer.User.Config;
import std;

namespace GW2Viewer::Data::Content
{

std::wstring* GetCustomName(ContentNamespace const& ns)
{
    auto const itr = G::Config.ContentNamespaceNames.find(ns.GetFullName());
    return itr != G::Config.ContentNamespaceNames.end() && !itr->second.empty() ? &itr->second : nullptr;
}
bool IsCustomNameCorrect(ContentNamespace const& ns)
{
    return MangleFullName(std::format(L"{}.", ns.GetFullDisplayName(QueryPurpose::Mangle))).substr(0, 5) == ns.Name;
}

bool ContentNamespace::HasCustomName() const
{
    return GetCustomName(*this);
}

bool ContentNamespace::HasCorrectCustomName() const
{
    return GetCustomName(*this) && IsCustomNameCorrect(*this);
}

bool ContentNamespace::CustomNameMatchesSiblings() const
{
    if (auto const custom = GetCustomName(*this))
        return CustomNameMatchesSiblings(*custom);

    return false;
}

bool ContentNamespace::CustomNameMatchesSiblings(std::wstring_view custom) const
{
    if (!Parent)
        return true;

    static auto trim = [](std::wstring_view string)
    {
        switch (string[0])
        {
            case L'_':
                return string.substr(1);
            default:
                return string;
        }
    };

    auto thisItr = std::ranges::find(Parent->Namespaces, this);
    for (auto const prev : std::span(Parent->Namespaces.begin(), thisItr) | std::views::reverse)
    {
        if (auto const prevName = GetCustomName(*prev))
        {
            if (custom < *prevName && custom < trim(*prevName) && trim(custom) < *prevName && trim(custom) < trim(*prevName))
                if (!prevName->starts_with(custom) && !custom.starts_with(*prevName))
                    return false;

            break;
        }
    }
    for (auto const next : std::span(++thisItr, Parent->Namespaces.end()))
    {
        if (auto const nextName = GetCustomName(*next))
        {
            if (custom > *nextName && custom > trim(*nextName) && trim(custom) > *nextName && trim(custom) > trim(*nextName))
                if (!nextName->starts_with(custom) && !custom.starts_with(*nextName))
                    return false;

            break;
        }
    }

    return true;
}

std::wstring ContentNamespace::GetDisplayName(QueryPurpose purpose) const
{
    auto const traits = NameQueryTraits::Get(purpose); // Local copy to avoid looking it up in the static array every time
    if (traits.Custom && !(traits.RespectShowOriginalNamesConfig && G::Config.ShowOriginalNames))
    {
        if (auto const custom = GetCustomName(*this))
        {
            if (traits.Color)
            {
                if (!IsCustomNameCorrect(*this))
                    return std::format(L"<c=#FCC>{}</c>", *custom);
                if (!CustomNameMatchesSiblings(*custom))
                    return std::format(L"<c=#FDC>{}</c>", *custom);
            }
            return *custom;
        }
    }

    return traits.Color ? std::format(L"<c=#FFC>{}</c>", Name) : Name;
}

std::wstring ContentNamespace::GetFullDisplayName(QueryPurpose purpose) const
{
    return Parent
        ? std::format(L"{}.{}", Parent->GetFullDisplayName(purpose), GetDisplayName(purpose))
        : GetDisplayName(purpose);
}

std::wstring ContentNamespace::GetFullName() const
{
    return Parent
        ? std::format(L"{}.{}", Parent->Name, Name)
        : Name;
}

ContentNamespace const* ContentNamespace::GetRoot() const
{
    auto* current = this;
    while (auto const* parent = current->Parent)
        current = parent;
    return current;
}

bool ContentNamespace::Contains(ContentObject const& object) const
{
    return object.Namespace && (object.Namespace == this || Contains(*object.Namespace));
}

bool ContentNamespace::MatchesFilter(ContentFilter& filter) const
{
    if (!filter.IsFilteringNamespaces())
        return true;

    auto& result = filter.FilteredNamespaces[Index];
    if (result == ContentFilter::UNCACHED_RESULT)
    {
        std::wstring displayName;
        result =
            !filter ||
            !filter.NameSearch.empty() && std::ranges::search(Name, filter.NameSearch, std::ranges::equal_to(), std::towupper, std::towupper) ||
            !filter.NameSearch.empty() && (displayName = GetDisplayName(QueryPurpose::Search), std::ranges::search(displayName, filter.NameSearch, std::ranges::equal_to(), std::towupper, std::towupper)) ||
            std::ranges::any_of(Namespaces, std::bind_back(&ContentNamespace::MatchesFilter, std::ref(filter))) ||
            std::ranges::any_of(Entries, std::bind_back(&ContentObject::MatchesFilter, std::ref(filter)));
    }
    return result;
}

}
