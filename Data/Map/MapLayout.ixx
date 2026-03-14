export module GW2Viewer.Data.Map.MapLayout;
import GW2Viewer.Content;
import GW2Viewer.Data.Content;
import GW2Viewer.Data.External.Types;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.ImGui;
import std;

namespace GW2Viewer::Data::Map::MapLayout
{

using Results = std::vector<std::reference_wrapper<Content::ContentObject const>>;

export
{

Results GetMapLayoutMaps(Content::ContentObject const& object)
{
    Results results;
    switch (object.Type->GetTypeInfo().ContentType)
    {
        using enum GW2Viewer::Content::EContentTypes;
        case MapDef:
            for (auto const mapDetailsMap : object.IncomingReferences)
                if (mapDetailsMap.Object->Type->GetTypeInfo().ContentType == MapDetailsMap)
                    for (auto const mapLayoutMap : mapDetailsMap.Object->IncomingReferences)
                        if (mapLayoutMap.Object->Type->GetTypeInfo().ContentType == MapLayoutMap)
                            results.emplace_back(*mapLayoutMap.Object);
            break;
        case MapLayoutMap:
            results.emplace_back(object);
            break;
        case MapLayoutRegion:
            for (Content::ContentObject const& map : object["Map->Name"])
                results.emplace_back(map);
            break;
        case MapLayoutContinentFloor:
            for (Content::ContentObject const& map : object["Region->Name->Map->Name"])
                results.emplace_back(map);
            break;
        case MapLayoutContinent:
            for (Content::ContentObject const& map : object["Floor->Name->Region->Name->Map->Name"])
                results.emplace_back(map);
            break;
        default:
            if (auto const map = object.GetMap())
                return GetMapLayoutMaps(*map);
            break;
    }
    return results;
}

Results GetMapLayoutRegions(Content::ContentObject const& object)
{
    Results results;
    switch (object.Type->GetTypeInfo().ContentType)
    {
        using enum GW2Viewer::Content::EContentTypes;
        case MapDef:
            for (Content::ContentObject const& mapLayoutMap : GetMapLayoutMaps(object))
                for (auto const ref : mapLayoutMap.IncomingReferences)
                    if (ref.Object->Type->GetTypeInfo().ContentType == MapLayoutRegion)
                        results.emplace_back(*ref.Object);
            break;
        case MapLayoutMap:
            for (auto const ref : object.IncomingReferences)
                if (ref.Object->Type->GetTypeInfo().ContentType == MapLayoutRegion)
                    results.emplace_back(*ref.Object);
            break;
        case MapLayoutRegion:
            results.emplace_back(object);
            break;
        case MapLayoutContinentFloor:
            for (Content::ContentObject const& map : object["Region->Name"])
                results.emplace_back(map);
            break;
        case MapLayoutContinent:
            for (Content::ContentObject const& map : object["Floor->Name->Region->Name"])
                results.emplace_back(map);
            break;
        default:
            if (auto const map = object.GetMap())
                return GetMapLayoutRegions(*map);
            break;
    }
    return results;
}

Results GetMapLayoutContinentFloors(Content::ContentObject const& object)
{
    Results results;
    switch (object.Type->GetTypeInfo().ContentType)
    {
        using enum GW2Viewer::Content::EContentTypes;
        case MapDef:
        case MapLayoutMap:
            for (Content::ContentObject const& mapLayoutMap : GetMapLayoutRegions(object))
                for (auto const ref : mapLayoutMap.IncomingReferences)
                    if (ref.Object->Type->GetTypeInfo().ContentType == MapLayoutContinentFloor)
                        results.emplace_back(*ref.Object);
            break;
        case MapLayoutRegion:
            for (auto const ref : object.IncomingReferences)
                if (ref.Object->Type->GetTypeInfo().ContentType == MapLayoutContinentFloor)
                    results.emplace_back(*ref.Object);
            break;
        case MapLayoutContinentFloor:
            results.emplace_back(object);
            break;
        case MapLayoutContinent:
            for (Content::ContentObject const& map : object["Floor->Name"])
                results.emplace_back(map);
            break;
        default:
            if (auto const map = object.GetMap())
                return GetMapLayoutContinentFloors(*map);
            break;
    }
    return results;
}

Results GetMapLayoutContinents(Content::ContentObject const& object)
{
    Results results;
    switch (object.Type->GetTypeInfo().ContentType)
    {
        using enum GW2Viewer::Content::EContentTypes;
        case MapDef:
        case MapLayoutMap:
        case MapLayoutRegion:
            for (Content::ContentObject const& mapLayoutContinentFloor : GetMapLayoutContinentFloors(object))
                for (auto const ref : mapLayoutContinentFloor.IncomingReferences)
                    if (ref.Object->Type->GetTypeInfo().ContentType == MapLayoutContinent)
                        results.emplace_back(*ref.Object);
            break;
        case MapLayoutContinentFloor:
            for (auto const ref : object.IncomingReferences)
                if (ref.Object->Type->GetTypeInfo().ContentType == MapLayoutContinent)
                    results.emplace_back(*ref.Object);
            break;
        case MapLayoutContinent:
            results.emplace_back(object);
            break;
        default:
            if (auto const map = object.GetMap())
                return GetMapLayoutContinents(*map);
            break;
    }
    return results;
}

Results GetMapLayoutContinents()
{
    Results results;
    for (auto const mapLayoutContinent : G::Game.Content.GetType(GW2Viewer::Content::MapLayoutContinent)->Objects)
        results.emplace_back(*mapLayoutContinent);
    return results;
}

}

}
