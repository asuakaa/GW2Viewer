export module GW2Viewer.UI.Controls:Table;
import GW2Viewer.Common;
import GW2Viewer.Services.Filter;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Viewers.ListViewer;
import GW2Viewer.Utils.Sort;
import std;
import <boost/any/unique_any.hpp>;
#include "Macros.h"

namespace GW2Viewer::UI::Controls
{

template<typename T> struct FilterTermInput { using type = T; static constexpr bool Range = false; };
template<typename T> requires std::ranges::range<T> && !Services::Filter::String<T> struct FilterTermInput<T> { using type = std::ranges::range_value_t<T>; static constexpr bool Range = true; };

export
{

template<typename FilterContext, typename SortContext, typename DrawContext>
struct Table
{
    struct DefinitionParams
    {
        std::function<void()> UpdateFilter;
        std::function<void()> UpdateSearch;
        std::function<void()> UpdateSort;
    };
    Table(DefinitionParams const& params) : m_definition(params) { }
    Table(Viewers::ListViewerBase& viewer, DefinitionParams const& params = { }) : m_definition(params)
    {
        m_definition.UpdateFilter = [&viewer, param = params.UpdateFilter] { if (param) param(); viewer.UpdateFilter(); };
        m_definition.UpdateSearch = [&viewer, param = params.UpdateSearch] { if (param) param(); viewer.UpdateSearch(); };
        m_definition.UpdateSort   = [&viewer, param = params.UpdateSort  ] { if (param) param(); viewer.UpdateSort  (); };
    }
    Table(Table const&) = delete;
    Table(Table&&) = delete;

    struct FilterHandler;
    struct ColumnParams
    {
        float Width = -1;
        bool NoHeaderWidth = false;
        bool NoResize = false;
        bool Hide = false;
        bool PreferSortDescending = false;

        FilterHandler Filter; // Setting Filter will automatically set Sort to a default implementation using the same getter as provided to Filter. Explicitly set Sort to nullptr if sorting must be disabled
        std::function<void(SortContext context, bool invert)> Sort = Filter.Sort;
        std::function<void(DrawContext context)> Draw;
    };
    void AddColumn(std::string id, ColumnParams const& params)
    {
        auto& column = m_columns.emplace_back(params);
        column.ID = std::move(id);
    }
    uint32 GetColumnCount() const { return m_columns.size(); }

    void SetShowFilterRow(bool show = true) { m_showFilterRow = show; }

    struct SortState
    {
        uint32 ColumnIndex = 0;
        bool Invert = false;
    };
    struct PreparedTable
    {
        bool HasFilterTerms() const { return m_hasColumnFilterTerms; }
        bool Filter(FilterContext context) const
        {
            if (m_hasColumnFilterTerms)
                for (auto const& [index, column] : m_table.m_columns | std::views::enumerate)
                    if (column.Filter)
                        if (auto const& term = m_columnFilterTerms.at(index); term.has_value())
                            if (!column.Filter.Test(term, context))
                                return false;
            return true;
        }
        void Sort(SortContext context) const
        {
            auto const& column = m_table.m_columns.at(m_sort.ColumnIndex);
            if (column.Sort)
                column.Sort(context, m_sort.Invert);
        }

    private:
        Table const& m_table;
        SortState m_sort = m_table.m_sort;
        bool m_hasColumnFilterTerms = false;
        std::vector<boost::anys::unique_any> m_columnFilterTerms;

        PreparedTable(Table& table) : m_table(table)
        {
            m_columnFilterTerms.reserve(table.m_columns.size());
            for (auto& column : table.m_columns)
            {
                if (!column.FilterTermsString.empty())
                {
                    auto term = column.Filter.Parse(column.FilterTermsString);
                    column.FilterHasError = column.Filter.HasError(term);
                    m_hasColumnFilterTerms |= column.Filter.HasTerm(term);
                    m_columnFilterTerms.emplace_back(std::move(term));
                }
                else
                {
                    column.FilterHasError = false;
                    m_columnFilterTerms.emplace_back();
                }
            }
        }

        friend Table;
    };
    PreparedTable Prepare() { return { *this }; }

    void DrawColumnHeaders()
    {
        for (auto const& [index, column] : m_columns | std::views::enumerate)
        {
            ImGuiTableColumnFlags flags = column.Width < 0 ? ImGuiTableColumnFlags_WidthStretch : ImGuiTableColumnFlags_WidthFixed;
            if (column.NoResize)        flags |= ImGuiTableColumnFlags_NoResize;
            if (column.NoHeaderWidth)   flags |= ImGuiTableColumnFlags_NoHeaderWidth;
            if (column.Hide)            flags |= ImGuiTableColumnFlags_DefaultHide;
            if (column.Sort)
            {
                if (column.PreferSortDescending)
                    flags |= ImGuiTableColumnFlags_PreferSortDescending;
            }
            else
                flags |= ImGuiTableColumnFlags_NoSort;

            I::TableSetupColumn(column.ID.c_str(), flags, std::abs(column.Width), index);
        }
        I::TableSetupScrollFreeze(0, m_showFilterRow ? 2 : 1);

        if (scoped::WithStyleVar(ImGuiStyleVar_FramePadding, ImVec2()))
            I::TableUpdateLayout(I::GetCurrentTable());
        I::TableHeadersRow();
        if (auto specs = I::TableGetSortSpecs(); specs && specs->SpecsDirty && specs->SpecsCount > 0)
        {
            m_sort.ColumnIndex = specs->Specs[0].ColumnUserID;
            m_sort.Invert = specs->Specs[0].SortDirection == ImGuiSortDirection_Descending;
            specs->SpecsDirty = false;
            UpdateSort();
        }

        if (m_showFilterRow)
        {
            I::TableNextRow(ImGuiTableRowFlags_Headers);
            for (auto& column : m_columns)
            {
                I::TableNextColumn();
                I::SetNextItemWidth(-FLT_MIN);
                if (column.Filter)
                {
                    if (scoped::WithColorVar(ImGuiCol_Text, column.FilterHasError ? 0xFF0000FF : I::GetColorU32(ImGuiCol_Text)))
                        if (I::InputTextWithHint(std::format("##FilterRow-{}", column.ID.c_str()).c_str(), "<c=#4>" ICON_FA_FILTER "</c>", &column.FilterTermsString))
                            UpdateFilter();
                    if (scoped::ItemTooltip())
                    if (scoped::WithTextWrapPos(450))
                    {
                        auto const help = column.Filter.GetSyntaxHelp();
                        if (!column.FilterTermsString.empty())
                        {
                            if (auto term = column.Filter.Parse(column.FilterTermsString); term.has_value() && column.Filter.HasTerm(term))
                            {
                                I::Text("<c=#8>Include rows where <c=#FF>%s</c> is</c>", column.ID.c_str());
                                column.Filter.DrawTerms(term);
                                if (help)
                                    I::Separator();
                            }
                        }
                        if (scoped::Font(13))
                        if (scoped::WithColorVar(ImGuiCol_Text, 0xFFAAAAAA))
                            I::TextUnformatted(help);
                    }
                }
            }
        }
    }
    void DrawRow(DrawContext& context) const
    {
        I::TableNextRow();
        for (auto const& [index, column] : m_columns | std::views::enumerate)
        {
            I::TableNextColumn();
            column.Draw(context);
        }
    }
    /*
    void DrawTable(SortContext context, std::function<DrawContext(typename std::decay_t<SortContext>::const_reference)> createDrawContext, std::function<ImGuiID(typename std::decay_t<SortContext>::const_reference)> createImGuiID, std::shared_mutex* uiMutex, std::shared_mutex* rowMutex)
    {
        if (scoped::TableList("Table", GetColumnCount()))
        {
            DrawColumnHeaders();

            std::optional<std::shared_lock<std::decay_t<decltype(*uiMutex)>>> uiLock;
            if (uiMutex)
                uiLock.emplace(*uiMutex);
            ImGuiListClipper clipper;
            clipper.Begin(context.size());
            while (clipper.Step())
            {
                for (auto const& element : std::span(context.begin() + clipper.DisplayStart, context.begin() + clipper.DisplayEnd))
                {
                    scoped::WithID(createImGuiID(element));

                    std::optional<std::shared_lock<std::decay_t<decltype(*rowMutex)>>> rowLock;
                    if (rowMutex)
                        rowLock.emplace(*rowMutex);
                    DrawRow(createDrawContext(element));
                }
            }
        }
    }
    */

private:
    struct Definition : DefinitionParams
    {

    };
    Definition m_definition;
    struct ColumnInfo : ColumnParams
    {
        std::string ID;
        std::string FilterTermsString;
        bool FilterHasError = false;
    };
    std::vector<ColumnInfo> m_columns;

    // Provides type-erased wrappers around Services::Filter term types and functions
    struct FilterHandler
    {
        std::function<boost::anys::unique_any(std::string_view input)> Parse;
        std::function<char const* ()> GetSyntaxHelp;
        std::function<bool(boost::anys::unique_any const& term)> HasTerm;
        std::function<bool(boost::anys::unique_any const& term)> HasError;
        std::function<bool(boost::anys::unique_any const& term, FilterContext context)> Test;
        std::function<void(boost::anys::unique_any const& term)> DrawTerms;
        std::function<void(SortContext context, bool invert)> Sort;

        FilterHandler() { }

        template<typename Func>
        FilterHandler(Func func)
        {
            using FilterTermInput = FilterTermInput<std::invoke_result_t<Func, FilterContext>>;
            using Input = typename FilterTermInput::type;
            Parse = [](std::string_view input) -> boost::anys::unique_any { return G::Services::Filter.Parse<Input>(input); };
            GetSyntaxHelp = [] { return G::Services::Filter.GetSyntaxHelp<Input>(); };
            HasTerm = [](boost::anys::unique_any const& term) { return (bool)boost::any_cast<std::unique_ptr<Services::Filter::Term<Input>> const&>(term); };
            HasError = [](boost::anys::unique_any const& term)
            {
                auto const& ptr = boost::any_cast<std::unique_ptr<Services::Filter::Term<Input>> const&>(term);
                return ptr && ptr->HasError();
            };
            Test = [func](boost::anys::unique_any const& term, FilterContext context)
            {
                if (auto const& ptr = boost::any_cast<std::unique_ptr<Services::Filter::Term<Input>> const&>(term))
                {
                    if constexpr (FilterTermInput::Range)
                        return std::ranges::any_of(func(context), [&ptr](auto const& value) { return ptr->Test(value); });
                    else
                        return ptr->Test(func(context));
                }
                return false;
            };
            DrawTerms = [](boost::anys::unique_any const& term)
            {
                if (auto const& ptr = boost::any_cast<std::unique_ptr<Services::Filter::Term<Input>> const&>(term))
                    ptr->Draw();
            };
            Sort = [func](SortContext context, bool invert) { Utils::Sort::ComplexSort(context, invert, func); };
        }

        operator bool() const { return (bool)Test; }
    };
    bool m_showFilterRow = false;

    SortState m_sort;

    void UpdateFilter() const
    {
        if (m_definition.UpdateFilter)
            m_definition.UpdateFilter();
    }
    void UpdateSearch() const
    {
        if (m_definition.UpdateSearch)
            m_definition.UpdateSearch();
    }
    void UpdateSort() const
    {
        if (m_definition.UpdateSort)
            m_definition.UpdateSort();
    }
};

}

}
