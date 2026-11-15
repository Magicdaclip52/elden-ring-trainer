#include "core/Logger.h"

#include "core/Types.h"
#include "core/Utf8.h"

#include <Windows.h>
#include <ShlObj.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <share.h>
#include <sstream>

namespace er {
namespace {

std::filesystem::path appDataDir() {
    PWSTR roaming = nullptr;
    std::filesystem::path dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roaming))) {
        dir = std::filesystem::path(roaming) / kAppFolder;
        CoTaskMemFree(roaming);
    }
    return dir;
}

std::filesystem::path exeDir() {
    wchar_t buf[MAX_PATH]{};
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(buf).parent_path();
}

std::string timestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::tm local{};
    localtime_s(&local, &t);
    char buf[64]{};
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                  local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                  local.tm_hour, local.tm_min, local.tm_sec, static_cast<int>(ms.count()));
    return buf;
}

}  // namespace

Logger& Logger::instance() {
    static Logger g;
    return g;
}

bool Logger::init() {
#ifdef ERTRAINER_DIST
    return true;
#else
    std::lock_guard lock(mutex_);
    if (file_) {
        return true;
    }

    std::error_code ec;
    auto dir = appDataDir();
    if (dir.empty()) {
        dir = exeDir();
    }
    std::filesystem::create_directories(dir, ec);
    const auto appDataLog = dir / kLogFileName;
    const auto localLog = exeDir() / kLogFileName;

    FILE* f = _wfsopen(appDataLog.c_str(), L"ab", _SH_DENYNO);
    if (!f) {
        f = _wfsopen(localLog.c_str(), L"ab", _SH_DENYNO);
        path_ = wideToUtf8(localLog.wstring());
    } else {
        path_ = wideToUtf8(appDataLog.wstring());
    }
    if (f) {
        std::fseek(f, 0, SEEK_END);
        if (std::ftell(f) == 0) {
            const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
            std::fwrite(bom, 1, 3, f);
            std::fflush(f);
        }
    }
    file_ = f;
    return f != nullptr;
#endif
}

void Logger::shutdown() {
    std::lock_guard lock(mutex_);
    if (file_) {
        std::fclose(static_cast<FILE*>(file_));
        file_ = nullptr;
    }
}

void Logger::info(std::string_view msg) {
    write("INFO", msg);
}

void Logger::warn(std::string_view msg) {
    write("WARN", msg);
}

void Logger::error(std::string_view msg) {
    write("ERROR", msg);
}

std::vector<std::string> Logger::recent(usize maxLines) const {
    std::lock_guard lock(mutex_);
    if (ring_.size() <= maxLines) {
        return ring_;
    }
    return {ring_.end() - static_cast<std::ptrdiff_t>(maxLines), ring_.end()};
}

std::string Logger::logPath() const {
    std::lock_guard lock(mutex_);
    return path_;
}

void Logger::write(std::string_view level, std::string_view msg) {
#ifdef ERTRAINER_DIST
    (void)level;
    (void)msg;
    return;
#else
    std::lock_guard lock(mutex_);
    std::ostringstream line;
    line << '[' << timestamp() << "] [" << level << "] " << msg;
    const std::string s = line.str();
    ring_.push_back(s);
    if (ring_.size() > 500) {
        ring_.erase(ring_.begin(), ring_.begin() + static_cast<std::ptrdiff_t>(ring_.size() - 400));
    }
    if (file_) {
        auto* f = static_cast<FILE*>(file_);
        std::fwrite(s.data(), 1, s.size(), f);
        std::fputc('\n', f);
        std::fflush(f);
    }
#endif
}

}  // namespace er
