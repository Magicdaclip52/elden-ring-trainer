#pragma once

#include "memory/SignatureScanner.h"

#include <span>

namespace er {

struct ScanPattern {
    const char* id;
    const char* primary;
    const char* fallback;
};

std::span<const ScanPattern> knownPatterns();
void logPatternHits(SignatureScanner& scanner);

}  // namespace er
