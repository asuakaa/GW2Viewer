export module GW2Viewer.Data.Encryption.Text;
import GW2Viewer.Common;
import GW2Viewer.Common.Time;
import GW2Viewer.Data.External.Types;
import GW2Viewer.UI.ImGui;
import std;

export namespace GW2Viewer::Data::Encryption
{

struct TextKeyInfo
{
    uint64 Key { };
    External::Encounter Encounter { Time::Now() };

    inline static uint32 NextIndex = 0;
    uint32 Index = NextIndex++;
};

}
