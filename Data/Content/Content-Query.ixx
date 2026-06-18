export module GW2Viewer.Data.Content:Query;
import :TypeInfo;
import magic_enum;
import <boost/container/small_vector.hpp>;
import <experimental/generator>;

export namespace GW2Viewer::Data::Content
{
struct ContentObject;

enum class QueryPurpose
{
    Unspecified,
    Draw,
    DrawRespectShowOriginalNamesConfig,
    Search,
    Sort,
    SortRespectShowOriginalNamesConfig,
    Copy,
    MetaContentDisplay,
    MetaContentSymbol,
    Mangle,
    Demangle,
    Wiki,
};
struct NameQueryTraits
{
    bool Custom : 1 = true;
    bool Color : 1 = true;
    bool DisplayFormat : 1 = true;
    bool Draw : 1 = false;
    bool RespectShowOriginalNamesConfig : 1 = false;

    static constexpr NameQueryTraits const& Get(QueryPurpose purpose)
    {
        static constexpr auto traits = []
        {
            std::array<NameQueryTraits, magic_enum::enum_count<QueryPurpose>()> traits;
            traits[std::to_underlying(QueryPurpose::Unspecified)] = { };
            traits[std::to_underlying(QueryPurpose::Draw)] = { .Draw = true };
            traits[std::to_underlying(QueryPurpose::DrawRespectShowOriginalNamesConfig)] = { .Draw = true, .RespectShowOriginalNamesConfig = true };
            traits[std::to_underlying(QueryPurpose::Search)] = { .Color = false };
            traits[std::to_underlying(QueryPurpose::Sort)] = { .Color = false };
            traits[std::to_underlying(QueryPurpose::SortRespectShowOriginalNamesConfig)] = { .Color = false, .RespectShowOriginalNamesConfig = true };
            traits[std::to_underlying(QueryPurpose::Copy)] = { .Color = false };
            traits[std::to_underlying(QueryPurpose::MetaContentDisplay)] = { };
            traits[std::to_underlying(QueryPurpose::MetaContentSymbol)] = { .DisplayFormat = false };
            traits[std::to_underlying(QueryPurpose::Mangle)] = { .Color = false, .DisplayFormat = false };
            traits[std::to_underlying(QueryPurpose::Demangle)] = { .Color = false, .DisplayFormat = false };
            traits[std::to_underlying(QueryPurpose::Wiki)] = { .Color = false, .DisplayFormat = false };
            return traits;
        }();
        return traits[std::to_underlying(purpose)];
    }
};

struct SymbolPath
{
    enum class Type : byte
    {
        String = 0, // Reusing the last byte of std::string_view for this, which should always be 0 too
        Meta,
        Reference,
        Backtrack,
    };
    union Value
    {
        std::string_view String;
        TypeInfo::Symbol* MetaSymbol;
        ContentTypeInfo const* ReferenceType;
    };
    struct Part
    {
        union
        {
            Value Value;
            struct
            {
                byte Padding[sizeof(Value) - sizeof(std::underlying_type_t<Type>)];
                Type Type;
            };
        };

        Part() : Value({ .String = { } }) { Type = Type::String; }
        Part(std::string_view string) : Value({ .String = string }) { Type = Type::String; }
        Part(TypeInfo::Symbol& metaSymbol) : Value({ .MetaSymbol = &metaSymbol }) { Type = Type::Meta; }
        Part(ContentTypeInfo const* referenceType) : Value({ .ReferenceType = referenceType }) { Type = Type::Reference; }
    };
    using Span = std::span<Part const>;

    boost::container::small_vector<Part, 5> Parts;

    SymbolPath(std::string_view path);

    operator Span() const { return Parts; }
};

struct QuerySymbolDataResult : TypeInfo::Context
{
    //using Generator = std::experimental::generator<QuerySymbolDataResult>;
    struct Generator
    {
        using internal_type = std::experimental::generator<QuerySymbolDataResult>;
        using promise_type = internal_type::promise_type;
        internal_type Internal;
        Generator(internal_type&& internal) : Internal(std::move(internal)) { }
        decltype(auto) begin() { return Internal.begin(); }
        decltype(auto) end() { return Internal.end(); }
        QuerySymbolDataResult const& operator*() { return *begin(); }
        QuerySymbolDataResult const* operator->() { return &**this; }
        template<typename T> operator T() { return **this; }
        operator ContentObject() = delete;
        operator ContentObject() const = delete;
        operator ContentObject const() = delete;
        operator ContentObject const() const = delete;
        operator ContentObject const* () { return **this; }
        operator ContentObject const& () { return **this; }
    };

    using Context::Context;

    template<typename T> operator T() const { return Data<T>(); }
    operator ContentObject() = delete;
    operator ContentObject() const = delete;
    operator ContentObject const() = delete;
    operator ContentObject const() const = delete;
    operator ContentObject const* () const { return Symbol.GetType()->GetContent(*this).value_or(nullptr); }
    operator ContentObject const& () const { return **this; }
};
QuerySymbolDataResult::Generator QuerySymbolData(ContentObject const& content, SymbolPath::Span path);
QuerySymbolDataResult::Generator QuerySymbolData(ContentObject const& content, std::string_view path);
QuerySymbolDataResult::Generator QuerySymbolData(ContentObject const& content, TypeInfo::SymbolType const& type, TypeInfo::Condition::ValueType value);
struct ExportOptions
{
    enum class ContentPointerFormats
    {
        GUID,
        Verbose,
        Joined,
    } ContentPointerFormat = ContentPointerFormats::Verbose;
    std::string ContentPointerFormatJoinedSeparator = "|";
    uint32 ContentPointerMaxInlineDepth = 0;
    bool IgnoreUnnamedFields = false;
};
ordered_json ExportSymbolData(ContentObject const& content, ExportOptions const& options = { });

}
