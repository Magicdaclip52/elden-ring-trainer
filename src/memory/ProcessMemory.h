#pragma once

#include "core/Types.h"

#include <Windows.h>

#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace er {

class ProcessMemory {
public:
    ProcessMemory() = default;
    ~ProcessMemory();

    ProcessMemory(const ProcessMemory&) = delete;
    ProcessMemory& operator=(const ProcessMemory&) = delete;

    bool open(DWORD pid);
    void close();
    [[nodiscard]] bool isOpen() const { return handle_ != nullptr; }
    [[nodiscard]] HANDLE handle() const { return handle_; }
    [[nodiscard]] DWORD pid() const { return pid_; }

    bool read(uptr addr, void* dst, usize size) const;
    bool write(uptr addr, const void* src, usize size);

    template <typename T>
    bool read(uptr addr, T& out) const {
        return read(addr, &out, sizeof(T));
    }

    template <typename T>
    bool write(uptr addr, const T& value) {
        return write(addr, &value, sizeof(T));
    }

    bool patch(uptr addr, std::span<const u8> bytes);
    bool restore(uptr addr);
    void restoreAll();

private:
    HANDLE handle_ = nullptr;
    DWORD pid_ = 0;
    std::unordered_map<uptr, std::vector<u8>> originals_;
};

std::string lastWinError();

}  // namespace er
