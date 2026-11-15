#pragma once

#include "core/Types.h"
#include "memory/ProcessMemory.h"

#include <span>
#include <vector>

namespace er {

class CodeCave {
public:
    bool ensureAllocated(ProcessMemory& memory, usize size = 0x1000);
    bool allocateNear(ProcessMemory& memory, uptr hookSite, usize size = 0x1000);
    bool write(ProcessMemory& memory, usize offset, std::span<const u8> data);
    bool writeU32(ProcessMemory& memory, usize offset, u32 value);
    bool writeU64(ProcessMemory& memory, usize offset, u64 value);

    bool installRelJmpHook(ProcessMemory& memory, uptr hookSite, usize patchLen);
    bool removeHook(ProcessMemory& memory);

    [[nodiscard]] bool active() const { return hooked_; }
    [[nodiscard]] uptr base() const { return base_; }
    [[nodiscard]] uptr hookSite() const { return hookSite_; }

private:
    uptr base_ = 0;
    uptr hookSite_ = 0;
    std::vector<u8> hookOriginal_;
    usize patchLen_ = 0;
    bool hooked_ = false;
};

}  // namespace er
