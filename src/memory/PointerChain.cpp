#include "memory/PointerChain.h"

namespace er {

std::optional<uptr> resolvePointerChain(const ProcessMemory& memory, uptr base, std::span<const i32> offsets) {
    if (offsets.empty()) {
        return base;
    }
    uptr addr = base;
    for (usize i = 0; i < offsets.size(); ++i) {
        addr += static_cast<uptr>(static_cast<i64>(offsets[i]));
        if (i + 1 == offsets.size()) {
            return addr;
        }
        uptr next = 0;
        if (!memory.read(addr, next) || next == 0) {
            return std::nullopt;
        }
        addr = next;
    }
    return addr;
}

}  // namespace er
