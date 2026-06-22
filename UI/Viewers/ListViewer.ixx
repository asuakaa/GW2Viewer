export module GW2Viewer.UI.Viewers.ListViewer;
import GW2Viewer.Common;
import GW2Viewer.UI.ImGui;
import GW2Viewer.UI.Viewers.Viewer;
import GW2Viewer.UI.Viewers.ViewerRegistry;
import GW2Viewer.Utils.Scan;
import std;

export namespace GW2Viewer::UI::Viewers { struct ListViewerBase; }

export namespace GW2Viewer::G::Viewers
{

template<typename T> requires std::is_base_of_v<UI::Viewers::ListViewerBase, T>
std::list<T*> ListViewers;

template<typename T, typename Func> requires std::is_base_of_v<UI::Viewers::ListViewerBase, T>
void ForEach(Func&& func) { std::ranges::for_each(ListViewers<T>, [&func](T* viewer) { return std::invoke(func, *viewer); }); }

template<typename T, typename Result, typename... Args> requires std::is_base_of_v<UI::Viewers::ListViewerBase, T>
void Notify(Result(T::* method)(Args...), Args&&... args) { ForEach<T>(std::bind_back(method, std::ref(args)...)); }

}

export namespace GW2Viewer::UI::Viewers
{

struct ListViewerBase : Viewer
{
    using Viewer::Viewer;

    virtual void UpdateSort() = 0;
    virtual void UpdateSearch() = 0;
    virtual void UpdateFilter() { UpdateSearch(); }

protected:
    template<typename SortEnum>
    bool HandleTableSort(SortEnum& sort, bool& sortInvert)
    {
        if (auto specs = I::TableGetSortSpecs(); specs && specs->SpecsDirty && specs->SpecsCount > 0)
        {
            sort = (SortEnum)specs->Specs[0].ColumnUserID;
            sortInvert = specs->Specs[0].SortDirection == ImGuiSortDirection_Descending;
            specs->SpecsDirty = false;
            UpdateSort();
            return true;
        }
        return false;
    }
};

template<typename Self, ViewerRegistry::Info Info, auto& Config = ViewerRegistry::EmptyConfig>
struct ListViewer : ListViewerBase, RegisterViewer<Self, Info, Config>
{
    using Base = ListViewer;

    ListViewer(uint32 id, bool newTab) : ListViewerBase(id, newTab)
    {
        G::Viewers::ListViewers<Self>.emplace_back((Self*)this);
    }
    ~ListViewer() override
    {
        G::Viewers::ListViewers<Self>.remove((Self*)this);
    }

    std::string Title() override { return this->ViewerInfo.Title; }
};

}
