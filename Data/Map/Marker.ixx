export module GW2Viewer.Data.Map.Marker;
import GW2Viewer.Common;
import GW2Viewer.Common.GUID;
import GW2Viewer.Data.External.Types;
import GW2Viewer.UI.ImGui;

export namespace GW2Viewer::Data::Map
{

struct Marker
{
    uint32 MapID;
    uint32 ContentID;
    uint32 MarkerID;
    GUID MarkerDefGUID;
    uint32 MarkerType;
    uint32 Flags;
    uint32 Floor;
    uint32 Color;
    uint32 DescriptionTextID;
    uint32 EventUID;
    ImVec4 Coord;
    uint32 CoordType;
    uint32 AgentID;
    ImVec4 AgentPosition;
    float Rotation;
    uint32 SecondaryContentID;
    uint32 SectorDefDataID;

    External::Encounter Encounter;
};

}
