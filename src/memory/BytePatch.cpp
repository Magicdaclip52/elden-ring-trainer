#include "memory/BytePatch.h"

namespace er {

bool BytePatch::apply(ProcessMemory& memory, uptr addr, std::span<const u8> patched) {
    if (applied_ && addr_ == addr) {
        return true;
    }
    if (applied_) {
        revert(memory);
    }
    original_.assign(patched.size(), 0);
    if (!memory.read(addr, original_.data(), original_.size())) {
        original_.clear();
        return false;
    }
    if (!memory.write(addr, patched.data(), patched.size())) {
        original_.clear();
        return false;
    }
    addr_ = addr;
    applied_ = true;
    return true;
}

bool BytePatch::revert(ProcessMemory& memory) {
    if (!applied_) {
        return true;
    }
    const bool ok = memory.write(addr_, original_.data(), original_.size());
    if (ok) {
        applied_ = false;
        addr_ = 0;
        original_.clear();
    }
    return ok;
}

}  // namespace er
