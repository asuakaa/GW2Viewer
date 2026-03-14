export module GW2Viewer.Data.Map.DisplaySet;
import GW2Viewer.Content;
import GW2Viewer.Data.Content;
import GW2Viewer.Data.External.Types;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.ImGui;
import std;

export namespace GW2Viewer::Data::Map
{

struct DisplaySet
{
    struct ContinentFloor
    {
        Content::ContentObject const& MapLayoutContinent;
        Content::ContentObject const& MapLayoutContinentFloor;
    };
    struct ContinentCoords
    {
        ImVec2 Position;
    };
    struct MapCoords
    {
        Content::ContentObject const* MapDef;
        ImVec4 PositionFacing;
        std::variant<std::monostate, External::Encounter> Context;
    };

    using Element = std::variant<ContinentFloor, ContinentCoords, MapCoords>;
    std::vector<Element> Elements;

    DisplaySet& Add(Content::ContentObject const& mapLayoutContinent, Content::ContentObject const& mapLayoutContinentFloor)
    {
        Elements.emplace_back(ContinentFloor { .MapLayoutContinent = mapLayoutContinent, .MapLayoutContinentFloor = mapLayoutContinentFloor });
        return *this;
    }
    DisplaySet& Add(ImVec2 const& continentCoords)
    {
        Elements.emplace_back(ContinentCoords { .Position = continentCoords });
        return *this;
    }
    DisplaySet& Add(ImVec2i const& continentCoords)
    {
        Elements.emplace_back(ContinentCoords { .Position = { (float)continentCoords.x, (float)continentCoords.y } });
        return *this;
    }
    DisplaySet& Add(Content::ContentObject const& mapDef, ImVec2 const& mapCoords)
    {
        Elements.emplace_back(MapCoords { .MapDef = &mapDef, .PositionFacing = { mapCoords.x, mapCoords.y, 0, 0 } });
        return *this;
    }
    DisplaySet& Add(Content::ContentObject const& mapDef, ImVec4 const& mapCoords)
    {
        Elements.emplace_back(MapCoords { .MapDef = &mapDef, .PositionFacing = mapCoords });
        return *this;
    }
    DisplaySet& Add(External::Encounter const& encounter)
    {
        if (encounter.Map && encounter.Position)
            if (auto const mapDef = G::Game.Content.GetByDataID(GW2Viewer::Content::MapDef, *encounter.Map))
                Elements.emplace_back(MapCoords { .MapDef = mapDef, .PositionFacing = *encounter.Position, .Context = encounter });

        return *this;
    }
};

}
