module GW2Viewer.UI.Controls;
import GW2Viewer.Data.Archive;
import GW2Viewer.Data.Map.DisplaySet;
import GW2Viewer.UI.Viewers.ContentViewer;
import GW2Viewer.UI.Viewers.ContentListViewer;
import GW2Viewer.UI.Viewers.FileViewer;
import GW2Viewer.UI.Viewers.ListViewer;
import GW2Viewer.UI.Viewers.MapLayoutViewer;

namespace GW2Viewer::UI::Controls
{

void OpenContent(Data::Content::ContentObject const& content, Viewers::OpenViewerOptions const& options)
{
    Viewers::ContentViewer::Open(content, options);
}

void OpenContentNamespace(Data::Content::ContentNamespace const& ns)
{
    G::Viewers::Notify(&Viewers::ContentListViewer::LocateNamespace, ns);
}

void OpenFile(Data::Archive::File const& file, Viewers::OpenViewerOptions const& options)
{
    Viewers::FileViewer::Open(file, options);
}

void OpenMapLayout(Data::Map::DisplaySet const& set, Viewers::OpenViewerOptions const& options)
{
    Viewers::MapLayoutViewer::Open(set, options);
}

}
