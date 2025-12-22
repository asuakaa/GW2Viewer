export module GW2Viewer.UI.Controls:TypeInfo;
import GW2Viewer.Common;
import GW2Viewer.Common.GUID;
import GW2Viewer.Data.Content;
import GW2Viewer.UI.ImGui;
import GW2Viewer.Utils.ConstString;
import GW2Viewer.Utils.Enum;
import std;
#include "Macros.h"

namespace GW2Viewer::UI::Controls
{

static Data::Content::ContentObject dummyContent { };

template<ConstString TypeName> struct SymbolTypeName { static constexpr std::string_view Type = TypeName.template get<char>(); };
template<typename T> struct SymbolTypeFromDataType { };
template<> struct SymbolTypeFromDataType<byte> : SymbolTypeName<"uint8"> { };
template<> struct SymbolTypeFromDataType<uint16> : SymbolTypeName<"uint16"> { };
template<> struct SymbolTypeFromDataType<uint32> : SymbolTypeName<"uint32"> { };
template<> struct SymbolTypeFromDataType<uint64> : SymbolTypeName<"uint64"> { };
template<> struct SymbolTypeFromDataType<sbyte> : SymbolTypeName<"int8"> { };
template<> struct SymbolTypeFromDataType<int16> : SymbolTypeName<"int16"> { };
template<> struct SymbolTypeFromDataType<int32> : SymbolTypeName<"int32"> { };
template<> struct SymbolTypeFromDataType<int64> : SymbolTypeName<"int64"> { };
template<> struct SymbolTypeFromDataType<ImVec2> : SymbolTypeName<"Point2D"> { };
template<> struct SymbolTypeFromDataType<GUID> : SymbolTypeName<"GUID"> { };
template<Enumeration E> struct SymbolTypeFromDataType<E> : SymbolTypeFromDataType<std::underlying_type_t<E>> { };
template<typename T> static constexpr auto GetType = SymbolTypeFromDataType<std::decay_t<T>>::Type;

export
{

void Symbol(auto const& data, Data::Content::TypeInfo::Symbol const& symbol)
{
    const_cast<Data::Content::TypeInfo::Symbol&>(symbol).Draw((byte const*)&data, Data::Content::TypeInfo::Symbol::DrawType::Inline, dummyContent);
}
void Symbol(std::string_view name, auto const& data, Data::Content::TypeInfo::Symbol const& symbol)
{
    auto dummySymbol = symbol;
    dummySymbol.Name = std::string(name);
    Symbol(data, dummySymbol);
}
void Symbol(std::string_view name, auto const& data) { Symbol(data, { .Name = std::string(name), .Type = std::string(GetType<decltype(data)>) }); }
void SymbolEnum(std::string_view name, auto const& data, std::string_view enumType) { Symbol(data, { .Name = std::string(name), .Type = std::string(GetType<decltype(data)>), .Enum = Data::Content::TypeInfo::Enum { .Name = std::string(enumType) } }); }
void SymbolFlags(std::string_view name, auto const& data, std::string_view enumType = { }) { Symbol(data, { .Name = std::string(name), .Type = std::string(GetType<decltype(data)>), .Enum = Data::Content::TypeInfo::Enum { .Name = std::string(enumType), .Flags = true } }); }
void SymbolText(std::string_view name, uint32 const& textID) { Symbol(textID, { .Name = std::string(name), .Type = "StringID" }); }
void SymbolFile(std::string_view name, uint32 const& fileID) { Symbol(fileID, { .Name = std::string(name), .Type = "FileID" }); }

void Value(auto const& data, Data::Content::TypeInfo::Symbol const& symbol)
{
    scoped::WithID(&data);
    symbol.GetType()->Draw({ &data, dummyContent, const_cast<Data::Content::TypeInfo::Symbol&>(symbol), Data::Content::TypeInfo::Symbol::DrawType::Inline });
}
void Value(auto const& data) { Value(data, { .Type = std::string(GetType<decltype(data)>) }); }
void ValueEnum(auto const& data, std::string_view enumType) { Value(data, { .Type = std::string(GetType<decltype(data)>), .Enum = Data::Content::TypeInfo::Enum { .Name = std::string(enumType) } }); }
void ValueFlags(auto const& data, std::string_view enumType = { }) { Value(data, { .Type = std::string(GetType<decltype(data)>), .Enum = Data::Content::TypeInfo::Enum { .Name = std::string(enumType), .Flags = true } }); }
void ValueText(uint32 const& textID) { Value(textID, { .Type = "StringID" }); }
void ValueFile(uint32 const& fileID) { Value(fileID, { .Type = "FileID" }); }

}

}
