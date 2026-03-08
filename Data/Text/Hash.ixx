export module GW2Viewer.Data.Text.Hash;
import GW2Viewer.Common;
import std;

namespace GW2Viewer::Data::Text
{

inline constexpr uint32 s_hashValue[]
{
    0x92B9A528, 0x25D4FC88, 0xEDCBEFB8, 0x51063A80,
    0x91341C61, 0x0261229D, 0x726F48ED, 0xCE1C088C,
    0x76253EB5, 0x31E3A0DE, 0xA2AAD215, 0xCA7D6D27,
    0xA5F98970, 0x0541C365, 0x3C14FF04, 0x5056AF4F,
};

export
{

template<typename Ch>
constexpr uint32 IStrHash(std::basic_string_view<Ch> string)
{
    uint32 temp0 = 0xE2C15C9D;
    uint32 temp1 = 0x2170A28A;
    uint32 result = 0x325D1EAE;
    for (Ch ch : string)
    {
        temp0 = temp0 << 3 ^ ch;
        temp1 += s_hashValue[temp0 & 0x0F];
        result ^= temp0 + temp1;
    }
    return result;
}

template<typename Ch>
constexpr uint32 IStrHash(std::basic_string<Ch> string) { return IStrHash(std::basic_string_view(string)); }

template<typename Ch>
constexpr uint32 IStrHash(Ch const* string) { return IStrHash(std::basic_string_view(string)); }

template<typename Ch>
constexpr uint32 IStrHashI(std::basic_string_view<Ch> string)
{
    uint32 temp0 = 0xE2C15C9D;
    uint32 temp1 = 0x2170A28A;
    uint32 result = 0x325D1EAE;
    for (Ch ch : string)
    {
        if (ch >= 'a' && ch <= 'z')
            ch = ch + 'A' - 'a';
        temp0 = temp0 << 3 ^ ch;
        temp1 += s_hashValue[temp0 & 0x0F];
        result ^= temp0 + temp1;
    }
    return result;
}

template<typename Ch>
constexpr uint32 IStrHashI(std::basic_string<Ch> string) { return IStrHashI(std::basic_string_view(string)); }

template<typename Ch>
constexpr uint32 IStrHashI(Ch const* string) { return IStrHashI(std::basic_string_view(string)); }

}

}
