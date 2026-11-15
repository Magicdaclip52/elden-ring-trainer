#pragma once

#include "ui/Hotkeys.h"

#include <string>
#include <unordered_map>

namespace er {

struct WindowConfig {
    int x = 80;
    int y = 80;
    int w = 1180;
    int h = 760;
};

class Config {
public:
    static Config& instance();

    bool load();
    bool save() const;

    WindowConfig window{};
    std::unordered_map<std::string, Hotkey> hotkeys;
    std::unordered_map<std::string, float> featureFloat;
    std::unordered_map<std::string, int> featureInt;
    std::string path;

private:
    Config() = default;
};

}  // namespace er
