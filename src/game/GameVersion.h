#pragma once

#include <string>
#include <string_view>

namespace er {

struct GameVersion {
    std::string fileVersion;
    std::string productVersion;
    std::string displayVersion;
    bool known = false;
    bool warning = false;
    std::string warningText;
};

GameVersion parseFileVersion(std::wstring_view exePath);
GameVersion mapKnownVersion(std::string_view fileVersion);

}  // namespace er
