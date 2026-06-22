export module GW2Viewer.Services.Filter;
import GW2Viewer.Common.Time;
import GW2Viewer.UI.ImGui;
import GW2Viewer.Utils.ConstString;
import GW2Viewer.Utils.Encoding;
import GW2Viewer.Utils.Enum;
import GW2Viewer.Utils.Format;
import GW2Viewer.Utils.Math;
import GW2Viewer.Utils.Scan;
import GW2Viewer.Utils.String;
import std;
#include "Macros.h"

using namespace std::literals;

namespace GW2Viewer::Services
{

template<typename T>
using CheapInput = std::conditional_t<std::is_trivially_copyable_v<T> && sizeof(T) <= 2 * sizeof(void*), T, T const&>;

std::string Format(Time::Fuzzy const& fuzzy, std::string_view prefixOn, std::string_view prefixIn, bool end = false)
{
    return std::format("{} {}", fuzzy.Day || fuzzy.HasTime() ? prefixOn : prefixIn, end ? fuzzy.FormatEnd() : fuzzy.FormatStart());
}

export
{

struct Filter
{
    template<typename T> struct Term
    {
        virtual ~Term() = default;
        virtual bool HasError() const { return false; }
        virtual bool Test(CheapInput<T> input) const = 0;
        virtual void Draw() const = 0;
    };

    #pragma region Errors
    template<typename T> struct Error : Term<T>
    {
        std::string Context;
        Error(std::string_view context) : Context(context) { }
        bool HasError() const override { return true; }
        bool Test(CheapInput<T> input) const override { return true; }
        void Draw() const override
        {
            if (scoped::DisableMarkup())
                I::TextColored({ 1, 0, 0, 1 }, "Malformed Input: %s", Context.c_str());
        }
    };
    template<typename T> struct Unsupported : Term<T>
    {
        std::string Context;
        Unsupported(std::string_view context) : Context(context) { }
        bool HasError() const override { return true; }
        bool Test(CheapInput<T> input) const override { return true; }
        void Draw() const override
        {
            if (scoped::DisableMarkup())
                I::TextColored({ 1, 0, 0, 1 }, "Unsupported Type: %s", Context.c_str());
        }
    };
    #pragma endregion
    #pragma region Generic
    template<typename T> struct Equals : Term<T>
    {
        T Value;
        Equals(T value) : Value(value) { }
        bool Test(CheapInput<T> input) const override { return input == Value; }
        void Draw() const override { I::TextUnformatted(std::format("= {}", Value).c_str()); }
    };
    template<typename T> struct NotEquals : Term<T>
    {
        T Value;
        NotEquals(T value) : Value(value) { }
        bool Test(CheapInput<T> input) const override { return input != Value; }
        void Draw() const override { I::TextUnformatted(std::format("!= {}", Value).c_str()); }
    };
    #pragma endregion
    #pragma region Arithmetic
    template<typename T> static constexpr bool Arithmetic = std::is_arithmetic_v<T>;
    template<typename T> struct MinMaxRangeInclusive : Term<T>
    {
        std::optional<T> Min, Max;
        MinMaxRangeInclusive(std::optional<T> min, std::optional<T> max) : Min(min), Max(max) { }
        bool Test(CheapInput<T> input) const override { return (!Min || input >= Min) && (!Max || input <= Max); }
        void Draw() const override { I::TextUnformatted((Min && Max ? std::format("from {} to {}", *Min, *Max) : Min ? std::format("=> {}", *Min) : Max ? std::format("<= {}", *Max) : "???").c_str()); }
    };
    template<typename T> struct MinMaxRangeExclusive : Term<T>
    {
        std::optional<T> Min, Max;
        MinMaxRangeExclusive(std::optional<T> min, std::optional<T> max) : Min(min), Max(max) { }
        bool Test(CheapInput<T> input) const override { return (!Min || input > Min) && (!Max || input < Max); }
        void Draw() const override { I::TextUnformatted((Min && Max ? std::format("between {} and {}", *Min, *Max) : Min ? std::format("> {}", *Min) : Max ? std::format("< {}", *Max) : "???").c_str()); }
    };
    template<typename T> struct ProximityRange : Term<T>
    {
        T Base, Range;
        ProximityRange(T base, T range) : Base(base), Range(range) { }
        bool Test(CheapInput<T> input) const override { return Utils::Math::InRange(input, Base, Range); }
        void Draw() const override { I::TextUnformatted(std::format("within {} of {}", Range, Base).c_str()); }
    };
    #pragma endregion
    #pragma region Integral
    template<typename T> static constexpr bool Integral = std::is_integral_v<T>;
    template<typename T> struct BitMask : Term<T>
    {
        T Mask;
        BitMask(T mask) : Mask(mask) { }
        bool Test(CheapInput<T> input) const override { return input & Mask; }
        void Draw() const override { I::TextUnformatted(std::format("has bits 0x{:X}", Mask).c_str()); }
    };
    #pragma endregion
    #pragma region Time
    template<typename T> static constexpr bool Time = std::is_same_v<T, Time::Point> || std::is_same_v<T, Time::LocalPoint>;
    template<typename T> struct TimeDurationUntilNow : Term<T>
    {
        std::optional<Time::Duration> Min, Max;
        Time::Point const Now = Time::Now();
        Time::LocalPoint const LocalNow = Time::LocalNow();
        TimeDurationUntilNow(std::optional<Time::Duration> min, std::optional<Time::Duration> max) : Min(min), Max(max) { }
        bool Test(CheapInput<T> input) const override
        {
            if constexpr (std::is_same_v<T, Time::Point>)
                return (!Min || input <= Now - *Min) && (!Max || input >= Now - *Max);
            else if constexpr (std::is_same_v<T, Time::LocalPoint>)
                return (!Min || input <= LocalNow - *Min) && (!Max || input >= LocalNow - *Max);
            else
                static_assert(false);
        }
        void Draw() const override { I::TextUnformatted((Min && Max ? std::format("between {} and {} ago", Utils::Format::DurationLong(*Min), Utils::Format::DurationLong(*Max)) : Min ? std::format("over {} ago", Utils::Format::DurationLong(*Min)) : Max ? std::format("under {} ago", Utils::Format::DurationLong(*Max)) : "???").c_str()); }
    };
    template<typename T> struct TimeEquals : Term<T>
    {
        Time::Fuzzy Fuzzy;
        TimeEquals(Time::Fuzzy const& fuzzy) : Fuzzy(fuzzy) { }
        bool Test(CheapInput<T> input) const override { return input == Fuzzy; }
        void Draw() const override { I::TextUnformatted(Format(Fuzzy, "on", "in").c_str()); }
    };
    template<typename T> struct TimeNotEquals : Term<T>
    {
        Time::Fuzzy Fuzzy;
        TimeNotEquals(Time::Fuzzy const& fuzzy) : Fuzzy(fuzzy) { }
        bool Test(CheapInput<T> input) const override { return input != Fuzzy; }
        void Draw() const override { I::TextUnformatted(Format(Fuzzy, "not on", "not in").c_str()); }
    };
    template<typename T> struct TimeMinMaxRangeInclusive : Term<T>
    {
        std::optional<Time::Fuzzy> Min, Max;
        TimeMinMaxRangeInclusive(std::optional<Time::Fuzzy> min, std::optional<Time::Fuzzy> max) : Min(min), Max(max) { }
        bool Test(CheapInput<T> input) const override { return (!Min || input >= Min) && (!Max || input <= Max); }
        void Draw() const override { I::TextUnformatted((Min && Max ? std::format("from {} to {}", Min->FormatStart(), Max->FormatEnd()) : Min ? Format(*Min, "on or after", "in or after", false) : Max ? Format(*Max, "on or before", "in or before", true) : "???").c_str()); }
    };
    template<typename T> struct TimeMinMaxRangeExclusive : Term<T>
    {
        std::optional<Time::Fuzzy> Min, Max;
        TimeMinMaxRangeExclusive(std::optional<Time::Fuzzy> min, std::optional<Time::Fuzzy> max) : Min(min), Max(max) { }
        bool Test(CheapInput<T> input) const override { return (!Min || input > Min) && (!Max || input < Max); }
        void Draw() const override { I::TextUnformatted((Min && Max ? std::format("between {} and {}", Min->FormatEnd(), Max->FormatStart()) : Min ? Format(*Min, "after", "after", true) : Max ? Format(*Max, "before", "before", false) : "???").c_str()); }
    };
    template<typename T> struct TimeProximityRange : Term<T>
    {
        Time::Fuzzy Base;
        Time::Duration Range;
        TimeProximityRange(Time::Fuzzy const& base, Time::Duration const& range) : Base(base), Range(range) { }
        bool Test(CheapInput<T> input) const override
        {
            if constexpr (std::is_same_v<T, Time::Point>)
                return input >= Base.GetStart() - Range && input <= Base.GetEnd() + Range;
            else if constexpr (std::is_same_v<T, Time::LocalPoint>)
                return input >= Base.GetStartLocal() - Range && input <= Base.GetEndLocal() + Range;
            else
                static_assert(false);
        }
        void Draw() const override { I::TextUnformatted(std::format("within {} of {}", Utils::Format::DurationLong(Range), Base.FormatStart()).c_str()); }
    };
    #pragma endregion
    #pragma region String
    template<typename T> static constexpr bool String = std::is_convertible_v<T, std::string_view> || std::is_convertible_v<T, std::wstring_view>;
    template<typename T> struct StringContains : Term<T>
    {
        using Ch = std::conditional_t<std::is_convertible_v<T, std::string_view>, char, std::conditional_t<std::is_convertible_v<T, std::wstring_view>, wchar_t, void>>;
        std::basic_string<Ch> Substring;
        StringContains(std::string_view substring) requires std::is_same_v<Ch, char> : Substring(substring) { }
        StringContains(std::string_view substring) requires std::is_same_v<Ch, wchar_t> : Substring(Utils::Encoding::FromUTF8(substring)) { }
        bool Test(CheapInput<T> input) const override { return Utils::String::ContainsCI(input, Substring); }
        void Draw() const override
        {
            static constexpr ConstString format = R"(containing "{}" case-insensitively)";
            auto str = std::format(format.get<Ch>(), Substring);
            if constexpr (std::is_same_v<Ch, wchar_t>)
                I::TextUnformatted(Utils::Encoding::ToUTF8(str).c_str());
            else
                I::TextUnformatted(str.c_str());
        }
    };
    #pragma endregion
    #pragma region Logic
    template<typename T> struct Not : Term<T>
    {
        std::unique_ptr<Term<T>> Inner;
        Not(std::unique_ptr<Term<T>>&& inner) : Inner(std::move(inner)) { }
        bool HasError() const override { return Inner->HasError(); }
        bool Test(CheapInput<T> input) const override { return !Inner || !Inner->Test(input); }
        void Draw() const override
        {
            I::TextUnformatted("NOT ");
            I::SameLine(0, 0);
            Inner->Draw();
        }
    };
    template<typename T> struct And : Term<T>
    {
        std::unique_ptr<Term<T>> L, R;
        And(std::unique_ptr<Term<T>>&& l, std::unique_ptr<Term<T>>&& r) : L(std::move(l)), R(std::move(r)) { }
        bool HasError() const override { return L->HasError() || R->HasError(); }
        bool Test(CheapInput<T> input) const override { return (!L || L->Test(input)) && (!R || R->Test(input)); }
        void Draw() const override
        {
            L->Draw();
            I::SameLine(0, 0);
            I::TextUnformatted(" AND");
            R->Draw();
        }
    };
    template<typename T> struct Or : Term<T>
    {
        std::unique_ptr<Term<T>> L, R;
        Or(std::unique_ptr<Term<T>>&& l, std::unique_ptr<Term<T>>&& r) : L(std::move(l)), R(std::move(r)) { }
        bool HasError() const override { return L->HasError() || R->HasError(); }
        bool Test(CheapInput<T> input) const override { return L && L->Test(input) || R && R->Test(input); }
        void Draw() const override
        {
            L->Draw();
            I::TextUnformatted("<c=#8>OR</c>");
            R->Draw();
        }
    };
    #pragma endregion

    template<typename T>
    auto Parse(std::string_view input) const
    {
        using Utils::Scan::Into;
        auto parse = [](std::string_view term) -> Term<T>*
        {
            if (term.empty())
                return nullptr;

            if constexpr (Arithmetic<T>)
            {
                T a, b;
                if (Into(term, "{}-{}", a, b) || Into(term, "{:x}-{:x}", a, b) ||
                    Into(term, "{}..{}", a, b) || Into(term, "{:x}..{:x}", a, b))
                    return new MinMaxRangeInclusive<T>(a, b);
                if (Into(term, ">={}", a) || Into(term, ">={:x}", a))
                    return new MinMaxRangeInclusive<T>(a, std::nullopt);
                if (Into(term, ">{}", a) || Into(term, ">{:x}", a))
                    return new MinMaxRangeExclusive<T>(a, std::nullopt);
                if (Into(term, "<={}", a) || Into(term, "<={:x}", a))
                    return new MinMaxRangeInclusive<T>(std::nullopt, a);
                if (Into(term, "<{}", a) || Into(term, "<{:x}", a))
                    return new MinMaxRangeExclusive<T>(std::nullopt, a);
                if (Into(term, "{}+{}", a, b) || Into(term, "{:x}+{}", a, b) || Into(term, "{:x}+{:x}", a, b))
                    return new ProximityRange<T>(a, b);
                if constexpr (Integral<T>)
                {
                    if (Into(term, "&{}", a) || Into(term, "&{:x}", a))
                        return new BitMask<T>(a);
                }
                if (Into(term, "!={}", a) || Into(term, "!={:x}", a) ||
                    Into(term, "<>{}", a) || Into(term, "<>{:x}", a) ||
                    Into(term, "~{}", a) || Into(term, "~{:x}", a))
                    return new NotEquals<T>(a);
                if (Into(term, "=={}", a) || Into(term, "=={:x}", a) ||
                    Into(term, "={}", a) || Into(term, "={:x}", a) ||
                    Into(term, "{}", a) || Into(term, "{:x}", a))
                    return new Equals<T>(a);

                return new Error<T>(term);
            }
            else if constexpr (Time<T>)
            {
                Time::Duration dur, durB;
                if (Into(term, "{}-{}", dur, durB) ||
                    Into(term, "{}..{}", dur, durB))
                    return new TimeDurationUntilNow<T>(dur, durB);
                if (Into(term, ">={}", dur))
                    return new TimeDurationUntilNow<T>(dur, std::nullopt);
                if (Into(term, ">{}", dur))
                    return new TimeDurationUntilNow<T>(dur, std::nullopt);
                if (Into(term, "<={}", dur))
                    return new TimeDurationUntilNow<T>(std::nullopt, dur);
                if (Into(term, "<{}", dur))
                    return new TimeDurationUntilNow<T>(std::nullopt, dur);
                if (Into(term, "{}", dur))
                    return new TimeDurationUntilNow<T>(std::nullopt, dur);

                Time::Fuzzy a, b;
                if (Into(term, "{}..{}", a, b))
                    return new TimeMinMaxRangeInclusive<T>(a, b);
                if (Into(term, ">={}", a))
                    return new TimeMinMaxRangeInclusive<T>(a, std::nullopt);
                if (Into(term, ">{}", a))
                    return new TimeMinMaxRangeExclusive<T>(a, std::nullopt);
                if (Into(term, "<={}", a))
                    return new TimeMinMaxRangeInclusive<T>(std::nullopt, a);
                if (Into(term, "<{}", a))
                    return new TimeMinMaxRangeExclusive<T>(std::nullopt, a);
                if (Into(term, "{}+{}", a, dur))
                    return new TimeProximityRange<T>(a, dur);
                if (Into(term, "!={}", a) ||
                    Into(term, "<>{}", a) ||
                    Into(term, "~{}", a))
                    return new TimeNotEquals<T>(a);
                if (Into(term, "=={}", a) ||
                    Into(term, "={}", a) ||
                    Into(term, "{}", a))
                    return new TimeEquals<T>(a);

                return new Error<T>(term);
            }
            else if constexpr (Enumeration<T>)
            {
            }
            else if constexpr (String<T>)
            {
                return new StringContains<T>(term);
            }
            return new Unsupported<T>(typeid(T).name());
        };

        std::unique_ptr<Term<T>> result;
        for (auto orRange : input | std::views::split(","sv))
        {
            std::unique_ptr<Term<T>> andTerm;
            for (auto andRange : orRange | std::views::split("&&"sv))
            {
                std::unique_ptr<Term<T>> newTerm;
                std::string_view term(andRange);
                uint32 nots = 0;
                while (term.starts_with('!') && !term.starts_with("!="))
                {
                    ++nots;
                    term = term.substr(1);
                    term.remove_prefix(std::min(term.size(), term.find_first_not_of(" \t\r\n")));
                }
                newTerm.reset(parse(term));
                if (!newTerm)
                    continue;

                while (nots--)
                    newTerm.reset(new Not<T>(std::move(newTerm)));

                if (andTerm)
                    andTerm.reset(new And<T>(std::move(andTerm), std::move(newTerm)));
                else
                    andTerm = std::move(newTerm);
            }
            if (!andTerm)
                continue;

            if (result)
                result.reset(new Or<T>(std::move(result), std::move(andTerm)));
            else
                result = std::move(andTerm);
        }
        return result;
    }

    template<typename T>
    char const* GetSyntaxHelp() const
    {
        #define CODE(str) "<c=#FFFF><code>" #str "</code></c>"
        #define COMMON CODE(term) " =" \
                "\n   | " CODE(termA && termB) " - Requires both " CODE(termA) " and " CODE(termB) " to succeed" \
                "\n   | <c=#FFFF><code>termA, termB</code></c> - Requires either " CODE(termA) " or " CODE(termB) " to succeed (always lower precedence than " CODE(&&) ")" \
                "\n   | " CODE(!term) " - Requires " CODE(term) " to fail" \
                "\n   | " CODE(condition) " - Requires " CODE(condition) " to succeed" \
                "\n" \
                "\n" CODE(condition) " ="

        if constexpr (Arithmetic<T>)
        {
            return COMMON
                "\n   When " CODE(A) " and " CODE(B) " are decimal or hexadecimal integers or real numbers"
                "\n   | " CODE(A) " or " CODE(=A) " or " CODE(==A) " - Equals to " CODE(A)
                "\n   | " CODE(~A) " or " CODE(<>A) " or " CODE(!=A) " - Not equals to " CODE(A)
                "\n   | " CODE(<A) " - Less than " CODE(A)
                "\n   | " CODE(<=A) " - Less than or equal to " CODE(A)
                "\n   | " CODE(>A) " - Greater than " CODE(A)
                "\n   | " CODE(>=A) " - Greater than or equal to " CODE(A)
                "\n   | " CODE(A-B) " or " CODE(A..B) " - From " CODE(A) " to " CODE(B) " (inclusive)"
                "\n   | " CODE(A+B) " - Within " CODE(B) " values from " CODE(A)
                "\n   When " CODE(A) " is a decimal or hexadecimal integer"
                "\n   | " CODE(&A) " - Test presence of bits " CODE(A)
            ;
        }
        else if constexpr (Time<T>)
        {
            return COMMON
                "\n   When " CODE(A) "/" CODE(B) " are " CODE(duration) ":"
                "\n   | " CODE(A) " or " CODE(<A) " or " CODE(<=A) " - Less than " CODE(A) " duration ago"
                "\n   | " CODE(>A) " or " CODE(>=A) " - More than " CODE(A) " duration ago"
                "\n   | " CODE(A-B) " or " CODE(A..B) " - Between " CODE(A) " and " CODE(B) " duration ago (inclusive)"
                "\n   When " CODE(A) "/" CODE(B) " are " CODE(timepoint) ":"
                "\n   | " CODE(A) " or " CODE(=A) " or " CODE(==A) " - On " CODE(A)
                "\n   | " CODE(~A) " or " CODE(<>A) " or " CODE(!=A) " - Not on " CODE(A)
                "\n   | " CODE(<A) " - Before " CODE(A)
                "\n   | " CODE(<=A) " - On or before " CODE(A)
                "\n   | " CODE(>A) " - After " CODE(A)
                "\n   | " CODE(>=A) " - On or after " CODE(A)
                "\n   | " CODE(A..B) " - From " CODE(A) " to " CODE(B) " (inclusive)"
                "\n   | " CODE(A+duration) " - Within " CODE(duration) " from " CODE(A)
                "\n"
                "\n" CODE(duration) " = " CODE([#y] [#mo] [#d] [#h] [#m] [#s] [#ms]) " - Defines duration of time"
                "\n" CODE(timepoint) " = " CODE([date] [time]) " - Matches specified " CODE(time) " of the specified " CODE(date) ". If " CODE(time) " is specified - any unspecified components of " CODE(date) " are set to today's date"
                "\n" CODE(date) " ="
                "\n   | " CODE(YYYY) " - Matches entire year " CODE(YYYY)
                "\n   | " CODE(YYYY-MM) " - Matches entire month " CODE(MM) " of year " CODE(YYYY)
                "\n   | " CODE(YYYY-MM-DD) " - Matches entire day " CODE(DD) " of month " CODE(MM) " of year " CODE(YYYY)
                "\n   | " CODE(MM-DD) " - Matches entire day " CODE(DD) " of month " CODE(MM) " of current year"
                "\n   | " CODE(DD) " - Matches entire day " CODE(DD) " of current month and year"
                "\n" CODE(time) " = (24 hour clock expected)"
                "\n   | " CODE(HH:mm:ss) " - Matches the specified time"
                "\n   | " CODE(HH:mm) " - Matches entire minute of the specified time"
                "\n   | " CODE(HH:) " - Matches entire hour of the specified time"
                "\n   | " CODE(HH) " - Matches entire hour of the specified time (only if preceded by " CODE(date) ")"
            ;
        }
        else if constexpr (Enumeration<T>)
        {
            return nullptr;
        }
        else if constexpr (String<T>)
        {
            return COMMON
                "\n   | " CODE(substring) " - Contains text " CODE(substring) " case-insensitively"
            ;
        }
        return nullptr;
        #undef COMMON
        #undef CODE
    }
};

}

}

export namespace GW2Viewer::G::Services { GW2Viewer::Services::Filter Filter; }
