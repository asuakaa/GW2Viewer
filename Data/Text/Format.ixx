export module GW2Viewer.Data.Text.Format;
import GW2Viewer.Common;
import GW2Viewer.Data.Game;
import GW2Viewer.Data.Text.Hash;
import GW2Viewer.User.Config;
import GW2Viewer.Utils.String;
import std;

namespace GW2Viewer::Data::Text
{

enum
{
    TERM_FINAL = 0,
    TERM_INTERMEDIATE = 1,
    CONCAT_CODED = 2,
    CONCAT_LITERAL = 3,

    SPAN_TYPE_FIXUP = 0,
    FIXUP_TYPE_PLURAL = 1,

    FIXUP_TYPE_ARTICLE = 3,
    FIXUP_TYPE_ESCAPE = 4,

    FIXUP_ESCAPE_AUTO_DIGIT_SEPARATOR = IStrHashI(L"sep"),      // 0x782D2CF2 (the only verified name)
    FIXUP_ESCAPE_BRACKET_LEFT         = IStrHashI(L"lbracket"), // 0x69D5F712
    FIXUP_ESCAPE_BRACKET_RIGHT        = IStrHashI(L"rbracket"), // 0x45FB21B0
    FIXUP_ESCAPE_NULL                 = IStrHashI(L"null"),     // 0x9439A742
    FIXUP_GENERAL_NOSEP               = IStrHashI(L"nosep"),    // 0x2F8DE7BE
    FIXUP_GENERAL_PROPER              = IStrHashI(L"proper"),   // 0x0C5AEFEC
    FIXUP_GENERAL_TOPIC_M             = IStrHashI(L"topic-m"),  // 0x0477E01A
    FIXUP_GENERAL_TOPIC_F             = IStrHashI(L"topic-f"),  // 0x7E23DCAA
    FIXUP_GENERAL_PLUR                = IStrHashI(L"plur"),     // 0x48D633B8
    FIXUP_GENERAL_SING                = IStrHashI(L"sing"),     // 0x9F439ECD
    FIXUP_GENERAL_M                   = IStrHashI(L"m"),        // 0x0B81B762
    FIXUP_GENERAL_F                   = IStrHashI(L"f"),        // 0x41CD9892
    FIXUP_GENERAL_N                   = IStrHashI(L"n"),        // 0x9BB7CEB3
    FIXUP_GENERAL_P                   = IStrHashI(L"p"),        // 0x9FFDDB59
    FIXUP_GENERAL_PM                  = IStrHashI(L"pm"),       // 0xD2D31168
    FIXUP_GENERAL_PF                  = IStrHashI(L"pf"),       // 0x25A194EB
    FIXUP_GENERAL_PN                  = IStrHashI(L"pn"),       // 0x1BFFDE88
    FIXUP_GENERAL_U                   = IStrHashI(L"u"),        // 0x0EE05402
    FIXUP_GENERAL_NUM_START           = IStrHashI(L"#:\""),     // 0xF38BED4A
    FIXUP_INSERT_B                    = IStrHashI(L"b"),        // 0xE87B47E7
    FIXUP_PLURAL_END                  = IStrHashI(L"\""),       // 0xE87B47C7
    FIXUP_PLURAL_PL_START             = IStrHashI(L"pl:\""),    // 0xF327AD12
    FIXUP_PLURAL_PM_START             = IStrHashI(L"pm:\""),    // 0x51B73700
    FIXUP_PLURAL_PF_START             = IStrHashI(L"pf:\""),    // 0xBA8DF0CB
    FIXUP_SEX_F_START                 = IStrHashI(L"f:\""),     // 0x81FF8DB2
    FIXUP_SEX_N_START                 = IStrHashI(L"n:\""),     // 0xDA59F993

    FIXUP_ENGLISH_ARTICLE_AN          = IStrHashI(L"an"),       // 0x2D1887E4
    FIXUP_ENGLISH_ARTICLE_THE         = IStrHashI(L"the"),      // 0xC067241E
    FIXUP_ENGLISH_PLURAL_S            = IStrHashI(L"s"),        // 0x33A5EAC2

    FIXUP_KOREAN_SOUND_CONNECTION     = IStrHashI(L"과와"),     // 0x0F42F2A2
    FIXUP_KOREAN_SOUND_SUBJECT        = IStrHashI(L"이가"),     // 0xE55CD33A
    FIXUP_KOREAN_SOUND_OBJECT         = IStrHashI(L"을를"),     // 0xF29C088E
    FIXUP_KOREAN_SOUND_TOPIC          = IStrHashI(L"은는"),     // 0x46E6B73D
    FIXUP_KOREAN_SOUND_DIRECTION      = IStrHashI(L"으로로"),   // 0x733972C1

    FIXUP_FRENCH_ARTICLE_LE           = IStrHashI(L"le"),       // 0x9FAFBBCC
    FIXUP_FRENCH_ARTICLE_UN           = IStrHashI(L"un"),       // 0x47988080
    FIXUP_FRENCH_PLURAL_AL            = IStrHashI(L"al"),       // 0xCFDDB272
    FIXUP_FRENCH_PLURAL_S             = IStrHashI(L"s"),        // 0x33A5EAC2
    FIXUP_FRENCH_PLURAL_X             = IStrHashI(L"x"),        // 0xF86832CC

    FIXUP_GERMAN_ARTICLE_DEM          = IStrHashI(L"dem"),      // 0xDBEFE72D
    FIXUP_GERMAN_ARTICLE_DEN          = IStrHashI(L"den"),      // 0x2BF9BD5E
    FIXUP_GERMAN_ARTICLE_DER          = IStrHashI(L"der"),      // 0x7A253792
    FIXUP_GERMAN_ARTICLE_DES          = IStrHashI(L"des"),      // 0x820B988D
    FIXUP_GERMAN_ARTICLE_EIN          = IStrHashI(L"ein"),      // 0xB3910700
    FIXUP_GERMAN_ARTICLE_EINEM        = IStrHashI(L"einem"),    // 0x16B35BF5
    FIXUP_GERMAN_ARTICLE_EINEN        = IStrHashI(L"einen"),    // 0x86A175A2
    FIXUP_GERMAN_ARTICLE_EINES        = IStrHashI(L"eines"),    // 0x5EAF9055
    FIXUP_GERMAN_PLURAL_E             = IStrHashI(L"e"),        // 0x0EE05432
    FIXUP_GERMAN_PLURAL_EN            = IStrHashI(L"en"),       // 0x47988030
    FIXUP_GERMAN_PLURAL_ER            = IStrHashI(L"er"),       // 0x7754090C
    FIXUP_GERMAN_PLURAL_N             = IStrHashI(L"n"),        // 0x9BB7CEB3
    FIXUP_GERMAN_PLURAL_S             = IStrHashI(L"s"),        // 0x33A5EAC2

    FIXUP_SPANISH_ARTICLE_EL          = IStrHashI(L"el"),       // 0x66DDF346
    FIXUP_SPANISH_ARTICLE_UN          = IStrHashI(L"un"),       // 0x47988080
    FIXUP_SPANISH_PLURAL_ES           = IStrHashI(L"es"),       // 0xAF66AC63
    FIXUP_SPANISH_PLURAL_S            = IStrHashI(L"s"),        // 0x33A5EAC2
};
static_assert(0x782D2CF2 == FIXUP_ESCAPE_AUTO_DIGIT_SEPARATOR);
static_assert(0xF327AD12 == FIXUP_PLURAL_PL_START);
static_assert(0x733972C1 == FIXUP_KOREAN_SOUND_DIRECTION);

void FixupString(std::wstring& string)
{
    if (!string.contains(L'['))
        return;

    std::wstring result;
    result.resize(string.size());
    auto writeDest = result.data();
    auto sex = Sex::Male;
    auto plural = false;
    auto const pStart = string.data();
    auto const pEnd = pStart + string.size();
    auto copyFrom = pStart;
    auto p = pStart;
    auto write = [&]
    {
        if (copyFrom != p)
        {
            writeDest += string.copy(writeDest, p - copyFrom, copyFrom - pStart);
            copyFrom = p;
        }
    };
    for (; p < pEnd; ++p)
    {
        auto const c = *p;
        if (c == L'[')
        {
            write();
            auto const fixupEnd = string.find(']', std::distance(pStart, p + 1));
            if (fixupEnd == std::wstring::npos)
                break;
            std::wstring_view fixup { p + 1, (size_t)std::distance(p + 1, &string[fixupEnd]) };
            auto const quotePos = fixup.find('"');
            switch (auto const hash = IStrHashI(quotePos == std::wstring_view::npos ? fixup : fixup.substr(0, quotePos + 1)))
            {
              //case FIXUP_ESCAPE_AUTO_DIGIT_SEPARATOR: break;
                case FIXUP_ESCAPE_BRACKET_LEFT:         *writeDest++ = L'['; break;
                case FIXUP_ESCAPE_BRACKET_RIGHT:        *writeDest++ = L']'; break;
                case FIXUP_ESCAPE_NULL:                 break;
              //case FIXUP_GENERAL_NOSEP:               break;
              //case FIXUP_GENERAL_PROPER:              break;
              //case FIXUP_GENERAL_TOPIC_M:             break;
              //case FIXUP_GENERAL_TOPIC_F:             break;
                case FIXUP_GENERAL_PLUR:                plural = true; break;
                case FIXUP_GENERAL_SING:                plural = false; break;
                case FIXUP_GENERAL_M:                   sex = Sex::Male; break;
                case FIXUP_GENERAL_F:                   sex = Sex::Female; break;
                case FIXUP_GENERAL_N:                   sex = Sex::None; break;
                case FIXUP_GENERAL_P:                   plural = true; break;
                case FIXUP_GENERAL_PM:                  plural = true; sex = Sex::Male; break;
                case FIXUP_GENERAL_PF:                  plural = true; sex = Sex::Female; break;
                case FIXUP_GENERAL_PN:                  plural = true; sex = Sex::None; break;
              //case FIXUP_GENERAL_U:                   break;
              //case FIXUP_GENERAL_NUM_START:           break;
                case FIXUP_INSERT_B:                    *writeDest++ = '\n'; break;
                case FIXUP_PLURAL_END:                  break;
                case FIXUP_PLURAL_PL_START:
                case FIXUP_PLURAL_PM_START:
                case FIXUP_PLURAL_PF_START:
                case FIXUP_SEX_F_START:
                case FIXUP_SEX_N_START:
                    if (fixup.ends_with('"')) // FIXUP_PLURAL_END
                    {
                        uint32 prefixSize = 0;
                        bool condition = false;
                        switch (hash)
                        {
                            case FIXUP_PLURAL_PL_START: prefixSize = 4; condition = plural; break;
                            case FIXUP_PLURAL_PM_START: prefixSize = 4; condition = plural && sex == Sex::Male; break;
                            case FIXUP_PLURAL_PF_START: prefixSize = 4; condition = plural && sex == Sex::Female; break;
                            case FIXUP_SEX_F_START:     prefixSize = 3; condition = sex == Sex::Female; break;
                            case FIXUP_SEX_N_START:     prefixSize = 3; condition = sex == Sex::None; break;
                        }
                        if (condition)
                        {
                            writeDest = &result[std::wstring_view { result.data(), writeDest }.find_last_of(L' ') + 1];
                            std::fill(writeDest, result.data() + result.size(), L'\0');
                            writeDest += fixup.copy(writeDest, fixup.size() - (prefixSize + 1), prefixSize);
                        }
                    }
                    break;
                default:
                    auto handled = true;
                    switch (G::Config.Language)
                    {
                        case Language::English:
                        {
                            switch (hash)
                            {
                              //case FIXUP_ENGLISH_ARTICLE_AN:          break; (skip if has next [proper] or [u], otherwise check next letter and write "a", "an" or "a(n)")
                              //case FIXUP_ENGLISH_ARTICLE_THE:         break; (if no [proper] or [u] follows - write "the")
                                case FIXUP_ENGLISH_PLURAL_S:            if (plural) *writeDest++ = L's'; break;
                                default: handled = false; break;
                            }
                            break;
                        }
                        case Language::Korean:
                        {
                            switch (hash)
                            {
                              //case FIXUP_KOREAN_SOUND_CONNECTION:     break;
                              //case FIXUP_KOREAN_SOUND_SUBJECT:        break;
                              //case FIXUP_KOREAN_SOUND_OBJECT:         break;
                              //case FIXUP_KOREAN_SOUND_TOPIC:          break;
                              //case FIXUP_KOREAN_SOUND_DIRECTION:      break;
                                default: handled = false; break;
                            }
                            break;
                        }
                        case Language::French:
                        {
                            switch (hash)
                            {
                              //case FIXUP_FRENCH_ARTICLE_LE:           break;
                              //case FIXUP_FRENCH_ARTICLE_UN:           break;
                                case FIXUP_FRENCH_PLURAL_AL:            if (plural) { *writeDest++ = L'a'; *writeDest++ = L'l'; } break;
                                case FIXUP_FRENCH_PLURAL_S:             if (plural) *writeDest++ = L's'; break;
                                case FIXUP_FRENCH_PLURAL_X:             if (plural) *writeDest++ = L'x'; break;
                                default: handled = false; break;
                            }
                            break;
                        }
                        case Language::German:
                        {
                            switch (hash)
                            {
                              //case FIXUP_GERMAN_ARTICLE_DEM:          break;
                              //case FIXUP_GERMAN_ARTICLE_DEN:          break;
                              //case FIXUP_GERMAN_ARTICLE_DER:          break;
                              //case FIXUP_GERMAN_ARTICLE_DES:          break;
                              //case FIXUP_GERMAN_ARTICLE_EIN:          break;
                              //case FIXUP_GERMAN_ARTICLE_EINEM:        break;
                              //case FIXUP_GERMAN_ARTICLE_EINEN:        break;
                              //case FIXUP_GERMAN_ARTICLE_EINES:        break;
                                case FIXUP_GERMAN_PLURAL_E:             if (plural) *writeDest++ = L'e'; break;
                                case FIXUP_GERMAN_PLURAL_EN:            if (plural) { *writeDest++ = L'e'; *writeDest++ = L'n'; } break;
                                case FIXUP_GERMAN_PLURAL_ER:            if (plural) { *writeDest++ = L'e'; *writeDest++ = L'r'; } break;
                                case FIXUP_GERMAN_PLURAL_N:             if (plural) *writeDest++ = L'n'; break;
                                case FIXUP_GERMAN_PLURAL_S:             if (plural) *writeDest++ = L's'; break;
                                default: handled = false; break;
                            }
                            break;
                        }
                        case Language::Spanish:
                        {
                            switch (hash)
                            {
                              //case FIXUP_SPANISH_ARTICLE_EL:          break;
                              //case FIXUP_SPANISH_ARTICLE_UN:          break;
                                case FIXUP_SPANISH_PLURAL_ES:           if (plural) { *writeDest++ = L'e'; *writeDest++ = L's'; } break;
                                case FIXUP_SPANISH_PLURAL_S:            if (plural) *writeDest++ = L's'; break;
                                default: handled = false; break;
                            }
                            break;
                        }
                        default: handled = false; break;
                    }
                    if (handled)
                        break;
                    continue;
            }

            p = &string[fixupEnd];
            copyFrom = p + 1;
        }
    }
    write();

    result.resize(std::distance(result.data(), writeDest));
    string = std::move(result);
}

export
{

enum TEXTPARAM
{
    TEXTPARAM_END = 0,
    TEXTPARAM_NUM1,
    TEXTPARAM_NUM2,
    TEXTPARAM_NUM3,
    TEXTPARAM_NUM4,
    TEXTPARAM_NUM5,
    TEXTPARAM_NUM6,
    TEXTPARAM_STR1_LITERAL,
    TEXTPARAM_STR2_LITERAL,
    TEXTPARAM_STR3_LITERAL,
    TEXTPARAM_STR4_LITERAL,
    TEXTPARAM_STR5_LITERAL,
    TEXTPARAM_STR6_LITERAL,
    TEXTPARAM_STR1_CODED,
    TEXTPARAM_STR2_CODED,
    TEXTPARAM_STR3_CODED,
    TEXTPARAM_STR4_CODED,
    TEXTPARAM_STR5_CODED,
    TEXTPARAM_STR6_CODED,
};


inline void FormatTo(std::wstring& string) { }
template<typename T, typename... Args>
void FormatTo(std::wstring& string, TEXTPARAM param, T&& value, Args&&... args)
{
    using Type = std::decay_t<T>;
    switch (param)
    {
        case TEXTPARAM_END:
            return;
        case TEXTPARAM_NUM1:
        case TEXTPARAM_NUM2:
        case TEXTPARAM_NUM3:
        case TEXTPARAM_NUM4:
        case TEXTPARAM_NUM5:
        case TEXTPARAM_NUM6:
            if constexpr (std::integral<Type> || std::floating_point<Type>)
                Utils::String::ReplaceAll(string, std::format(L"%num{}%", 1 + param - TEXTPARAM_NUM1), std::format(L"{}", std::forward<T>(value)));
            break;
        case TEXTPARAM_STR1_LITERAL:
        case TEXTPARAM_STR2_LITERAL:
        case TEXTPARAM_STR3_LITERAL:
        case TEXTPARAM_STR4_LITERAL:
        case TEXTPARAM_STR5_LITERAL:
        case TEXTPARAM_STR6_LITERAL:
            if constexpr (!(std::integral<Type> || std::floating_point<Type>))
                Utils::String::ReplaceAll(string, std::format(L"%str{}%", 1 + param - TEXTPARAM_STR1_LITERAL), std::forward<T>(value));
            break;
        case TEXTPARAM_STR1_CODED:
        case TEXTPARAM_STR2_CODED:
        case TEXTPARAM_STR3_CODED:
        case TEXTPARAM_STR4_CODED:
        case TEXTPARAM_STR5_CODED:
        case TEXTPARAM_STR6_CODED:
            if constexpr (std::integral<Type>)
                if (value)
                    if (auto const paramString = G::Game.Text.Get(std::forward<T>(value)).first)
                        Utils::String::ReplaceAll(string, std::format(L"%str{}%", 1 + param - TEXTPARAM_STR1_CODED), *paramString);
            break;
    }
    FormatTo(string, std::forward<Args>(args)...);
}

template<typename... Args>
std::wstring FormatString(std::wstring_view format, Args&&... args)
{
    std::wstring result { format };
    FormatTo(result, std::forward<Args>(args)...);
    Utils::String::ReplaceAll(result, L"%%", L"%");
    FixupString(result);
    return result;
}

template<typename... Args>
std::wstring FormatString(uint32 stringID, Args&&... args)
{
    if (auto format = G::Game.Text.Get(stringID).first)
        return FormatString(*format, std::forward<Args>(args)...);
    return { };
}

std::wstring FormatStringEmpty(std::wstring_view format)
{
    return FormatString(format,
        TEXTPARAM_STR1_LITERAL, L"",
        TEXTPARAM_STR2_LITERAL, L"",
        TEXTPARAM_STR3_LITERAL, L"",
        TEXTPARAM_STR4_LITERAL, L"",
        TEXTPARAM_STR5_LITERAL, L"",
        TEXTPARAM_STR6_LITERAL, L"");
}

std::wstring FormatStringEmpty(uint32 stringID)
{
    return FormatString(stringID,
        TEXTPARAM_STR1_LITERAL, L"",
        TEXTPARAM_STR2_LITERAL, L"",
        TEXTPARAM_STR3_LITERAL, L"",
        TEXTPARAM_STR4_LITERAL, L"",
        TEXTPARAM_STR5_LITERAL, L"",
        TEXTPARAM_STR6_LITERAL, L"");
}

}

}
