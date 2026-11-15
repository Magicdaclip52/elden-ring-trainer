#include "core/Config.h"

#include "core/Logger.h"
#include "core/Types.h"
#include "core/Utf8.h"
#include "features/FeatureCatalog.h"

#include <Windows.h>
#include <ShlObj.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace er {
namespace {

std::filesystem::path configPath() {
    PWSTR roaming = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roaming))) {
        return {};
    }
    std::filesystem::path dir = std::filesystem::path(roaming) / kAppFolder;
    CoTaskMemFree(roaming);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir / kConfigFileName;
}

}  // namespace

Config& Config::instance() {
    static Config g;
    return g;
}

bool Config::load() {
    path = wideToUtf8(configPath().wstring());
    for (const auto& info : featureCatalog()) {
        hotkeys[info.id] = info.defaultHotkey;
        if (info.defaultFloat != 0.0f) {
            featureFloat[info.id] = info.defaultFloat;
        }
        if (info.defaultInt != 0) {
            featureInt[info.id] = info.defaultInt;
        }
    }

    std::ifstream in(utf8ToWide(path), std::ios::binary);
    if (!in) {
        Logger::instance().info("Конфиг не найден, создаём по умолчанию: " + path);
        return save();
    }

    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& ex) {
        Logger::instance().warn(std::string("Конфиг битый, перезаписываем: ") + ex.what());
        return save();
    }

    if (j.contains("window")) {
        window.x = j["window"].value("x", window.x);
        window.y = j["window"].value("y", window.y);
        window.w = j["window"].value("w", window.w);
        window.h = j["window"].value("h", window.h);
    }
    if (j.contains("hotkeys") && j["hotkeys"].is_object()) {
        for (auto it = j["hotkeys"].begin(); it != j["hotkeys"].end(); ++it) {
            if (it.value().is_string()) {
                hotkeys[it.key()] = parseHotkey(it.value().get<std::string>());
            }
        }
    }
    if (j.contains("feature_float") && j["feature_float"].is_object()) {
        for (auto it = j["feature_float"].begin(); it != j["feature_float"].end(); ++it) {
            featureFloat[it.key()] = it.value().get<float>();
        }
    }
    if (j.contains("feature_int") && j["feature_int"].is_object()) {
        for (auto it = j["feature_int"].begin(); it != j["feature_int"].end(); ++it) {
            featureInt[it.key()] = it.value().get<int>();
        }
    }
    Logger::instance().info("Конфиг загружен: " + path);
    return true;
}

bool Config::save() const {
    nlohmann::json j;
    j["window"] = {{"x", window.x}, {"y", window.y}, {"w", window.w}, {"h", window.h}};
    nlohmann::json hk = nlohmann::json::object();
    for (const auto& [id, key] : hotkeys) {
        hk[id] = key.toString();
    }
    j["hotkeys"] = hk;
    nlohmann::json ff = nlohmann::json::object();
    for (const auto& [id, v] : featureFloat) {
        ff[id] = v;
    }
    j["feature_float"] = ff;
    nlohmann::json fi = nlohmann::json::object();
    for (const auto& [id, v] : featureInt) {
        fi[id] = v;
    }
    j["feature_int"] = fi;

    const auto p = utf8ToWide(path.empty() ? wideToUtf8(configPath().wstring()) : path);
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out) {
        Logger::instance().error("Не удалось записать конфиг");
        return false;
    }
    out << std::setw(4) << j << '\n';
    return true;
}

}  // namespace er
