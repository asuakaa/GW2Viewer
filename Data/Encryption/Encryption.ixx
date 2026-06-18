export module GW2Viewer.Data.Encryption;
import std;
#include "Macros.h"

namespace GW2Viewer::Data::Encryption
{

export enum class Status
{
    Missing,
    Unencrypted,
    Encrypted,
    Decrypted,
};

std::unordered_map<Status, std::tuple<char const*, char const*, char const*>> const statuses
{
    { Status::Missing,     { "F00", "<nosel><c=#F00>" ICON_FA_BAN "</c></nosel>", "<nosel><c=#F00>" ICON_FA_BAN "</c> </nosel>" } },
    { Status::Unencrypted, { "FFF", "",                                           "<nosel>" ICON_FA_PLAY " </nosel>"            } },
    { Status::Encrypted,   { "F80", "<nosel>" ICON_FA_KEY " </nosel>",            "<nosel>" ICON_FA_KEY " </nosel>"             } },
    { Status::Decrypted,   { "0F0", "<nosel><c=#4>" ICON_FA_KEY "</c> </nosel>",  "<nosel>" ICON_FA_PLAY " </nosel>"            } },
};

export char const* GetStatusColor(Status status) { return std::get<0>(statuses.at(status)); }
export char const* GetStatusText(Status status) { return std::get<1>(statuses.at(status)); }
export char const* GetVoiceStatusText(Status status) { return std::get<2>(statuses.at(status)); }

export std::string const EncryptedStatusText = GetStatusText(Status::Encrypted);
export std::string const EncryptedStatusTextVoice = std::format("{}<nosel><c=#{}>" ICON_FA_PLAY "</c> </nosel>", GetStatusText(Status::Encrypted), GetStatusColor(Status::Encrypted));

}

