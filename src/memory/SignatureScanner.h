#pragma once

#include "core/Types.h"
#include "memory/ProcessMemory.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace er {

struct Signature {
    std::string name;
    std::string aob;
};

class SignatureScanner {
public:
    explicit SignatureScanner(ProcessMemory& memory);

    void setModule(uptr base, usize size);
    void setTextRange(uptr start, usize size);

    std::optional<uptr> find(const Signature& sig, bool requireUnique = true);
    std::vector<uptr> findAll(const Signature& sig);
    std::optional<uptr> findWithFallback(const Signature& primary, const Signature& fallback);

    std::optional<uptr> resolveRip(uptr insnAddr, int dispOffset, int insnLength) const;
    std::optional<uptr> cached(std::string_view name) const;

    [[nodiscard]] uptr textStart() const { return text_.base; }
    [[nodiscard]] usize textSize() const { return text_.size; }

private:
    struct Pattern {
        std::vector<u8> bytes;
        std::vector<u8> mask;
    };

    static std::optional<Pattern> parseAob(std::string_view aob);
    std::vector<uptr> scanAll(const Pattern& pat) const;

    ProcessMemory& memory_;
    ModuleRange module_{};
    ModuleRange text_{};
    std::unordered_map<std::string, uptr> cache_;
};

}  // namespace er
