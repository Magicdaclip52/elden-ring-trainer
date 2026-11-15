#include "game/GameVersion.h"

#include "core/Types.h"
#include "core/Utf8.h"

#include <Windows.h>

#include <vector>

namespace er {

GameVersion mapKnownVersion(std::string_view fileVersion) {
    GameVersion v;
    v.fileVersion = std::string(fileVersion);
    v.productVersion = v.fileVersion;
    if (fileVersion == kExpectedFileVersion) {
        v.displayVersion = kExpectedAppVersion;
        v.known = true;
        v.warning = false;
    } else if (fileVersion.empty()) {
        v.displayVersion = "неизвестно";
        v.known = false;
        v.warning = true;
        v.warningText = "Не удалось прочитать FileVersion eldenring.exe.";
    } else {
        v.displayVersion = std::string(fileVersion);
        v.known = false;
        v.warning = true;
        v.warningText = "Версия FileVersion " + std::string(fileVersion) +
                        " не совпадает с ожидаемой " + kExpectedFileVersion +
                        " (App Ver. " + kExpectedAppVersion + "). Запуск разрешён, часть функций может быть недоступна.";
    }
    return v;
}

GameVersion parseFileVersion(std::wstring_view exePath) {
    GameVersion empty = mapKnownVersion("");
    if (exePath.empty()) {
        return empty;
    }
    const std::wstring path(exePath);
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
    if (size == 0) {
        return empty;
    }
    std::vector<u8> buf(size);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, buf.data())) {
        return empty;
    }

    VS_FIXEDFILEINFO* info = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(buf.data(), L"\\", reinterpret_cast<LPVOID*>(&info), &len) || !info) {
        return empty;
    }

    char fv[32]{};
    std::snprintf(fv, sizeof(fv), "%u.%u.%u.%u",
                  HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS),
                  HIWORD(info->dwFileVersionLS), LOWORD(info->dwFileVersionLS));
    GameVersion v = mapKnownVersion(fv);
    v.productVersion = v.fileVersion;
    return v;
}

}  // namespace er
