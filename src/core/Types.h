#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

namespace er {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using uptr = std::uintptr_t;
using usize = std::size_t;

inline constexpr wchar_t kProcessName[] = L"eldenring.exe";
inline constexpr char kProcessNameUtf8[] = "eldenring.exe";
inline constexpr char kAppFolder[] = "ERTrainer";
inline constexpr char kLogFileName[] = "ERTrainer.log";
inline constexpr char kConfigFileName[] = "config.json";

inline constexpr char kExpectedAppVersion[] = "1.17.2";
inline constexpr char kExpectedFileVersion[] = "2.7.2.0";

struct ModuleRange {
    uptr base = 0;
    usize size = 0;
};

inline std::string toHex(uptr v) {
    char buf[32]{};
    std::snprintf(buf, sizeof(buf), "%llX", static_cast<unsigned long long>(v));
    return buf;
}

}  // namespace er
