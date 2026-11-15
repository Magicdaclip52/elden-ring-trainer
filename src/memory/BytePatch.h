#pragma once

#include "core/Types.h"
#include "memory/ProcessMemory.h"

#include <span>
#include <vector>

namespace er {

class BytePatch {
public:
    BytePatch() = default;

    bool apply(ProcessMemory& memory, uptr addr, std::span<const u8> patched);
    bool revert(ProcessMemory& memory);
    [[nodiscard]] bool applied() const { return applied_; }
    [[nodiscard]] uptr address() const { return addr_; }

private:
    uptr addr_ = 0;
    std::vector<u8> original_;
    bool applied_ = false;
};

}  // namespace er
