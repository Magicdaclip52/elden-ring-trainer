#pragma once

#include <string>
#include <string_view>

namespace er {

struct Hotkey {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    int vk = 0;

    [[nodiscard]] std::string toString() const;
    [[nodiscard]] bool matches(bool downCtrl, bool downAlt, bool downShift, int vkCode) const;
};

Hotkey parseHotkey(std::string_view text);
std::string vkName(int vk);

}  // namespace er
