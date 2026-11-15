#include "ui/Hotkeys.h"

#include <Windows.h>

#include <cctype>
#include <cstdio>
#include <sstream>
#include <unordered_map>

namespace er {
namespace {

const std::unordered_map<int, const char*> kVkNames = {
    {VK_NUMPAD0, "NUM 0"}, {VK_NUMPAD1, "NUM 1"}, {VK_NUMPAD2, "NUM 2"},
    {VK_NUMPAD3, "NUM 3"}, {VK_NUMPAD4, "NUM 4"}, {VK_NUMPAD5, "NUM 5"},
    {VK_NUMPAD6, "NUM 6"}, {VK_NUMPAD7, "NUM 7"}, {VK_NUMPAD8, "NUM 8"},
    {VK_NUMPAD9, "NUM 9"}, {VK_DECIMAL, "NUM ."}, {VK_ADD, "NUM +"},
    {VK_DIVIDE, "NUM /"},  {VK_SUBTRACT, "NUM -"}, {VK_MULTIPLY, "NUM *"},
};

}  // namespace

std::string vkName(int vk) {
    if (const auto it = kVkNames.find(vk); it != kVkNames.end()) {
        return it->second;
    }
    if (vk >= '0' && vk <= '9') {
        return std::string(1, static_cast<char>(vk));
    }
    if (vk >= 'A' && vk <= 'Z') {
        return std::string(1, static_cast<char>(vk));
    }
    if (vk >= VK_F1 && vk <= VK_F12) {
        return "F" + std::to_string(vk - VK_F1 + 1);
    }
    char buf[16]{};
    std::snprintf(buf, sizeof(buf), "VK_%02X", vk);
    return buf;
}

std::string Hotkey::toString() const {
    std::ostringstream o;
    if (ctrl) {
        o << "CTRL+";
    }
    if (alt) {
        o << "ALT+";
    }
    if (shift) {
        o << "SHIFT+";
    }
    o << vkName(vk);
    return o.str();
}

bool Hotkey::matches(bool downCtrl, bool downAlt, bool downShift, int vkCode) const {
    return vk != 0 && vk == vkCode && ctrl == downCtrl && alt == downAlt && shift == downShift;
}

Hotkey parseHotkey(std::string_view text) {
    Hotkey h{};
    std::string cur;
    auto flush = [&]() {
        if (cur.empty()) {
            return;
        }
        std::string u = cur;
        for (char& c : u) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        if (u == "CTRL" || u == "CONTROL") {
            h.ctrl = true;
        } else if (u == "ALT") {
            h.alt = true;
        } else if (u == "SHIFT") {
            h.shift = true;
        } else if (u == "NUM 0" || u == "NUM0") {
            h.vk = VK_NUMPAD0;
        } else if (u == "NUM 1" || u == "NUM1") {
            h.vk = VK_NUMPAD1;
        } else if (u == "NUM 2" || u == "NUM2") {
            h.vk = VK_NUMPAD2;
        } else if (u == "NUM 3" || u == "NUM3") {
            h.vk = VK_NUMPAD3;
        } else if (u == "NUM 4" || u == "NUM4") {
            h.vk = VK_NUMPAD4;
        } else if (u == "NUM 5" || u == "NUM5") {
            h.vk = VK_NUMPAD5;
        } else if (u == "NUM 6" || u == "NUM6") {
            h.vk = VK_NUMPAD6;
        } else if (u == "NUM 7" || u == "NUM7") {
            h.vk = VK_NUMPAD7;
        } else if (u == "NUM 8" || u == "NUM8") {
            h.vk = VK_NUMPAD8;
        } else if (u == "NUM 9" || u == "NUM9") {
            h.vk = VK_NUMPAD9;
        } else if (u == "NUM ." || u == "NUM.") {
            h.vk = VK_DECIMAL;
        } else if (u == "NUM +" || u == "NUM+") {
            h.vk = VK_ADD;
        } else if (u == "NUM /" || u == "NUM/") {
            h.vk = VK_DIVIDE;
        }
        cur.clear();
    };

    for (char c : text) {
        if (c == '+') {
            flush();
        } else {
            cur.push_back(c);
        }
    }
    flush();
    return h;
}

}  // namespace er
