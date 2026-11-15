#pragma once

#include "core/Types.h"
#include "memory/ProcessMemory.h"

#include <optional>
#include <span>

namespace er {

// Resolves: [base + offsets[0]] -> +offsets[1] -> ... -> final address (last offset is NOT dereferenced).
std::optional<uptr> resolvePointerChain(const ProcessMemory& memory, uptr base, std::span<const i32> offsets);

}  // namespace er
