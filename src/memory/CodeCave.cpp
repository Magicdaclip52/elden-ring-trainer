#include "memory/CodeCave.h"

#include "core/Logger.h"

#include <climits>
#include <cstring>
#include <vector>

namespace er {
namespace {

bool inRel32Range(uptr target, uptr hookSite) {
    const i64 rel = static_cast<i64>(target) - static_cast<i64>(hookSite + 5);
    return rel >= INT32_MIN && rel <= INT32_MAX;
}

}  // namespace

bool CodeCave::ensureAllocated(ProcessMemory& memory, usize size) {
    if (base_ != 0) {
        return true;
    }
    void* p = VirtualAllocEx(memory.handle(), nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!p) {
        Logger::instance().error("VirtualAllocEx cave failed: " + lastWinError());
        return false;
    }
    base_ = reinterpret_cast<uptr>(p);
    return true;
}

bool CodeCave::allocateNear(ProcessMemory& memory, uptr hookSite, usize size) {
    if (!memory.isOpen()) {
        return false;
    }
    if (base_ != 0 && inRel32Range(base_, hookSite)) {
        return true;
    }
    if (base_ != 0) {
        VirtualFreeEx(memory.handle(), reinterpret_cast<void*>(base_), 0, MEM_RELEASE);
        base_ = 0;
    }

    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    const usize gran = si.dwAllocationGranularity ? si.dwAllocationGranularity : 0x10000;
    const uptr alignedHook = hookSite & ~(gran - 1);

    auto tryAt = [&](uptr tryAddr) -> bool {
        tryAddr &= ~(gran - 1);
        if (tryAddr < 0x10000 || !inRel32Range(tryAddr, hookSite)) {
            return false;
        }
        void* p = VirtualAllocEx(memory.handle(), reinterpret_cast<void*>(tryAddr), size, MEM_COMMIT | MEM_RESERVE,
                                 PAGE_EXECUTE_READWRITE);
        if (!p) {
            return false;
        }
        const uptr cave = reinterpret_cast<uptr>(p);
        if (inRel32Range(cave, hookSite)) {
            base_ = cave;
            Logger::instance().info("Cave near: 0x" + toHex(cave) + " (hook 0x" + toHex(hookSite) + ")");
            return true;
        }
        VirtualFreeEx(memory.handle(), p, 0, MEM_RELEASE);
        return false;
    };

    for (usize step = 1; step < 0x7F00; ++step) {
        const uptr delta = step * gran;
        if (tryAt(alignedHook + delta)) {
            return true;
        }
        if (alignedHook > delta && tryAt(alignedHook - delta)) {
            return true;
        }
    }

    Logger::instance().error("Нет свободной страницы в ±2GB от хука 0x" + toHex(hookSite) +
                             ", инжект отменён (игра не патчится)");
    return false;
}

bool CodeCave::write(ProcessMemory& memory, usize offset, std::span<const u8> data) {
    if (base_ == 0 || data.empty()) {
        return false;
    }
    return memory.write(base_ + offset, data.data(), data.size());
}

bool CodeCave::writeU32(ProcessMemory& memory, usize offset, u32 value) {
    return write(memory, offset, std::span<const u8>(reinterpret_cast<const u8*>(&value), sizeof(value)));
}

bool CodeCave::writeU64(ProcessMemory& memory, usize offset, u64 value) {
    return write(memory, offset, std::span<const u8>(reinterpret_cast<const u8*>(&value), sizeof(value)));
}

bool CodeCave::installRelJmpHook(ProcessMemory& memory, uptr hookSite, usize patchLen) {
    if (base_ == 0 || patchLen < 5 || !inRel32Range(base_, hookSite)) {
        Logger::instance().error("installRelJmpHook: cave не в диапазоне rel32, отказ, без патча");
        return false;
    }

    hookOriginal_.assign(patchLen, 0);
    if (!memory.read(hookSite, hookOriginal_.data(), hookOriginal_.size())) {
        return false;
    }

    const i32 rel = static_cast<i32>(static_cast<i64>(base_) - static_cast<i64>(hookSite + 5));
    std::vector<u8> patch(patchLen, 0x90);
    patch[0] = 0xE9;
    std::memcpy(&patch[1], &rel, sizeof(rel));
    if (!memory.write(hookSite, patch.data(), patch.size())) {
        return false;
    }

    hookSite_ = hookSite;
    patchLen_ = patchLen;
    hooked_ = true;
    return true;
}

bool CodeCave::removeHook(ProcessMemory& memory) {
    if (!hooked_ || hookSite_ == 0 || hookOriginal_.empty()) {
        hooked_ = false;
        return true;
    }
    const bool ok = memory.write(hookSite_, hookOriginal_.data(), hookOriginal_.size());
    hookOriginal_.clear();
    hooked_ = false;
    hookSite_ = 0;
    if (base_ != 0 && memory.handle()) {
        VirtualFreeEx(memory.handle(), reinterpret_cast<void*>(base_), 0, MEM_RELEASE);
        base_ = 0;
    }
    return ok;
}

}  // namespace er
