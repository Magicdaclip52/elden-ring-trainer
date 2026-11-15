#include "game/GameAttach.h"

#include "core/Logger.h"
#include "core/Utf8.h"
#include "memory/ProcessMemory.h"

#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cwctype>
#include <string_view>
#include <vector>

namespace er {
namespace {

bool nameEquals(std::wstring_view a, std::wstring_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (usize i = 0; i < a.size(); ++i) {
        if (std::towlower(a[i]) != std::towlower(b[i])) {
            return false;
        }
    }
    return true;
}

}  // namespace

GameAttach::GameAttach() : scanner_(memory_) {
    state_.status = "Ожидание процесса eldenring.exe";
}

void GameAttach::enableDebugPrivilege() {
    if (privilegeTried_) {
        return;
    }
    privilegeTried_ = true;
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        Logger::instance().warn("OpenProcessToken: " + lastWinError());
        return;
    }
    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
        Logger::instance().warn("LookupPrivilegeValue SE_DEBUG: " + lastWinError());
        CloseHandle(token);
        return;
    }
    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
        Logger::instance().warn("AdjustTokenPrivileges: " + lastWinError());
    }
    CloseHandle(token);
}

bool GameAttach::findProcess(DWORD& pidOut) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return false;
    }
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    bool found = false;
    DWORD first = 0;
    int count = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (nameEquals(pe.szExeFile, kProcessName)) {
                ++count;
                if (!found) {
                    first = pe.th32ProcessID;
                    found = true;
                }
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    if (count > 1) {
        Logger::instance().warn("Найдено несколько eldenring.exe, берём PID " + std::to_string(first));
    }
    pidOut = first;
    return found;
}

bool GameAttach::readModuleInfo() {
    HMODULE mods[512]{};
    DWORD needed = 0;
    if (!EnumProcessModulesEx(memory_.handle(), mods, sizeof(mods), &needed, LIST_MODULES_64BIT)) {
        if (!EnumProcessModules(memory_.handle(), mods, sizeof(mods), &needed)) {
            Logger::instance().error("EnumProcessModules: " + lastWinError());
            return false;
        }
    }
    if (needed < sizeof(HMODULE)) {
        return false;
    }

    wchar_t name[MAX_PATH]{};
    wchar_t path[MAX_PATH]{};
    bool picked = false;
    const usize n = needed / sizeof(HMODULE);
    for (usize i = 0; i < n && i < 512; ++i) {
        name[0] = 0;
        GetModuleBaseNameW(memory_.handle(), mods[i], name, MAX_PATH);
        if (nameEquals(name, kProcessName)) {
            MODULEINFO info{};
            if (!GetModuleInformation(memory_.handle(), mods[i], &info, sizeof(info))) {
                Logger::instance().error("GetModuleInformation: " + lastWinError());
                return false;
            }
            GetModuleFileNameExW(memory_.handle(), mods[i], path, MAX_PATH);
            std::lock_guard lock(mutex_);
            state_.base = reinterpret_cast<uptr>(info.lpBaseOfDll);
            state_.moduleSize = info.SizeOfImage;
            state_.path = wideToUtf8(path);
            picked = true;
            break;
        }
    }
    if (!picked) {
        Logger::instance().error("Модуль eldenring.exe не найден в процессе");
        return false;
    }
    scanner_.setModule(state_.base, state_.moduleSize);
    return true;
}

bool GameAttach::readTextSection() {
    IMAGE_DOS_HEADER dos{};
    if (!memory_.read(state_.base, dos) || dos.e_magic != IMAGE_DOS_SIGNATURE) {
        Logger::instance().error("DOS-заголовок eldenring.exe не прочитан");
        return false;
    }
    IMAGE_NT_HEADERS64 nt{};
    if (!memory_.read(state_.base + static_cast<uptr>(dos.e_lfanew), nt) ||
        nt.Signature != IMAGE_NT_SIGNATURE) {
        Logger::instance().error("NT-заголовок eldenring.exe не прочитан");
        return false;
    }
    const uptr sectionStart = state_.base + static_cast<uptr>(dos.e_lfanew) +
                              offsetof(IMAGE_NT_HEADERS64, OptionalHeader) +
                              nt.FileHeader.SizeOfOptionalHeader;
    for (u16 i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
        IMAGE_SECTION_HEADER sec{};
        if (!memory_.read(sectionStart + static_cast<uptr>(i) * sizeof(IMAGE_SECTION_HEADER), sec)) {
            continue;
        }
        char name[9]{};
        std::memcpy(name, sec.Name, 8);
        if (std::string_view(name) == ".text") {
            std::lock_guard lock(mutex_);
            state_.textBase = state_.base + sec.VirtualAddress;
            state_.textSize = sec.Misc.VirtualSize ? sec.Misc.VirtualSize : sec.SizeOfRawData;
            scanner_.setTextRange(state_.textBase, state_.textSize);

            u8 preview[16]{};
            if (memory_.read(state_.textBase, preview, sizeof(preview))) {
                char hex[64]{};
                int pos = 0;
                for (int b = 0; b < 16 && pos + 4 < static_cast<int>(sizeof(hex)); ++b) {
                    pos += std::snprintf(hex + pos, sizeof(hex) - static_cast<usize>(pos), "%02X ", preview[b]);
                }
                state_.textPreviewHex = hex;
            }
            return true;
        }
    }
    Logger::instance().error("Секция .text не найдена");
    return false;
}

bool GameAttach::detectEac() {
    HMODULE mods[512]{};
    DWORD needed = 0;
    if (!EnumProcessModulesEx(memory_.handle(), mods, sizeof(mods), &needed, LIST_MODULES_64BIT) &&
        !EnumProcessModules(memory_.handle(), mods, sizeof(mods), &needed)) {
        return false;
    }
    const usize n = needed / sizeof(HMODULE);
    wchar_t name[MAX_PATH]{};
    for (usize i = 0; i < n && i < 512; ++i) {
        name[0] = 0;
        GetModuleBaseNameW(memory_.handle(), mods[i], name, MAX_PATH);
        std::wstring lower(name);
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
        if (lower.find(L"easyanticheat") != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

void GameAttach::tick() {
    enableDebugPrivilege();

    DWORD pid = 0;
    if (!findProcess(pid)) {
        if (state_.attached) {
            Logger::instance().warn("Процесс eldenring.exe закрыт, отключаемся");
            detach();
        }
        std::lock_guard lock(mutex_);
        state_.status = "Ожидание процесса eldenring.exe";
        return;
    }

    if (state_.attached && state_.pid == pid && memory_.isOpen()) {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(memory_.handle(), &exitCode) && exitCode == STILL_ACTIVE) {
            return;
        }
        Logger::instance().warn("Процесс больше не активен");
        detach();
    }

    if (!memory_.open(pid)) {
        std::lock_guard lock(mutex_);
        state_.status = "Не удалось OpenProcess";
        return;
    }

    if (!readModuleInfo() || !readTextSection()) {
        memory_.close();
        std::lock_guard lock(mutex_);
        state_.attached = false;
        state_.status = "Модуль не прочитан";
        return;
    }

    const bool eac = detectEac();
    GameVersion ver;
    {
        std::lock_guard lock(mutex_);
        ver = parseFileVersion(utf8ToWide(state_.path));
        state_.pid = pid;
        state_.attached = true;
        state_.eacLoaded = eac;
        state_.version = ver;
        state_.status = eac ? "Подключено, но обнаружен EasyAntiCheat" : "Подключено";
    }

    Logger::instance().info("Подключено к eldenring.exe PID=" + std::to_string(pid));
    Logger::instance().info("Путь: " + state_.path);
    Logger::instance().info("База модуля: 0x" + toHex(state_.base) + " размер=0x" + toHex(state_.moduleSize));
    Logger::instance().info(".text: 0x" + toHex(state_.textBase) + " размер=0x" + toHex(state_.textSize));
    if (!state_.textPreviewHex.empty()) {
        Logger::instance().info(".text[0..15]: " + state_.textPreviewHex);
    }
    Logger::instance().info("FileVersion=" + ver.fileVersion + "  отображаемая=" + ver.displayVersion);
    if (ver.warning) {
        Logger::instance().warn(ver.warningText);
    } else {
        Logger::instance().info("Версия совпадает с ожидаемой App Ver. " + std::string(kExpectedAppVersion));
    }
    if (eac) {
        Logger::instance().error("В процессе загружен EasyAntiCheat. Трейнер не будет писать в память. Запусти игру офлайн без EAC.");
    } else {
        Logger::instance().info("EasyAntiCheat в процессе не найден (офлайн, ок).");
    }
}

void GameAttach::detach() {
    memory_.restoreAll();
    memory_.close();
    std::lock_guard lock(mutex_);
    const std::string keepStatus = "Процесс потерян";
    state_ = {};
    state_.status = keepStatus;
}

AttachSnapshot GameAttach::snapshot() const {
    std::lock_guard lock(mutex_);
    return state_;
}

}  // namespace er
