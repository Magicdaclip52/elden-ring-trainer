#include "memory/SignatureScanner.h"

#include "core/Logger.h"

#include <cctype>

namespace er {
namespace {

int hexNibble(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

}  // namespace

SignatureScanner::SignatureScanner(ProcessMemory& memory) : memory_(memory) {}

void SignatureScanner::setModule(uptr base, usize size) {
    module_ = {base, size};
}

void SignatureScanner::setTextRange(uptr start, usize size) {
    text_ = {start, size};
}

std::optional<SignatureScanner::Pattern> SignatureScanner::parseAob(std::string_view aob) {
    Pattern pat;
    usize i = 0;
    while (i < aob.size()) {
        while (i < aob.size() && std::isspace(static_cast<unsigned char>(aob[i]))) {
            ++i;
        }
        if (i >= aob.size()) {
            break;
        }
        if (i + 1 < aob.size() && aob[i] == '?' && aob[i + 1] == '?') {
            pat.bytes.push_back(0);
            pat.mask.push_back(0);
            i += 2;
            continue;
        }
        const int hi = hexNibble(aob[i]);
        const int lo = (i + 1 < aob.size()) ? hexNibble(aob[i + 1]) : -1;
        if (hi < 0 || lo < 0) {
            return std::nullopt;
        }
        pat.bytes.push_back(static_cast<u8>((hi << 4) | lo));
        pat.mask.push_back(0xFF);
        i += 2;
    }
    if (pat.bytes.empty()) {
        return std::nullopt;
    }
    return pat;
}

std::vector<uptr> SignatureScanner::scanAll(const Pattern& pat) const {
    std::vector<uptr> hits;
    if (!memory_.isOpen() || text_.base == 0 || text_.size < pat.bytes.size()) {
        return hits;
    }

    constexpr usize kChunk = 1024 * 1024;
    std::vector<u8> buf;
    buf.resize(kChunk + pat.bytes.size());

    for (usize off = 0; off < text_.size; ) {
        const usize remain = text_.size - off;
        const usize readSize = remain < buf.size() ? remain : buf.size();
        if (!memory_.read(text_.base + off, buf.data(), readSize)) {
            Logger::instance().warn("AOB: не удалось прочитать .text по смещению " + std::to_string(off));
            off += kChunk;
            continue;
        }
        const usize searchLen = readSize >= pat.bytes.size() ? readSize - pat.bytes.size() + 1 : 0;
        for (usize i = 0; i < searchLen; ++i) {
            bool ok = true;
            for (usize j = 0; j < pat.bytes.size(); ++j) {
                if (pat.mask[j] && buf[i + j] != pat.bytes[j]) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                hits.push_back(text_.base + off + i);
            }
        }
        if (remain <= kChunk) {
            break;
        }
        off += kChunk;
    }
    return hits;
}

std::optional<uptr> SignatureScanner::find(const Signature& sig, bool requireUnique) {
    if (const auto it = cache_.find(sig.name); it != cache_.end()) {
        return it->second;
    }
    const auto pat = parseAob(sig.aob);
    if (!pat) {
        Logger::instance().error("Сигнатура '" + sig.name + "': некорректный AOB");
        return std::nullopt;
    }
    const auto hits = scanAll(*pat);
    if (hits.empty()) {
        Logger::instance().warn("Сигнатура '" + sig.name + "': не найдена");
        return std::nullopt;
    }
    if (requireUnique && hits.size() != 1) {
        Logger::instance().warn("Сигнатура '" + sig.name + "': не уникальна (" +
                                std::to_string(hits.size()) + " совпадений), расширьте маску");
        return std::nullopt;
    }
    cache_[sig.name] = hits.front();
    Logger::instance().info("Сигнатура '" + sig.name + "' найдена: 0x" + toHex(hits.front()));
    return hits.front();
}

std::vector<uptr> SignatureScanner::findAll(const Signature& sig) {
    const auto pat = parseAob(sig.aob);
    if (!pat) {
        Logger::instance().error("Сигнатура '" + sig.name + "': некорректный AOB");
        return {};
    }
    auto hits = scanAll(*pat);
    Logger::instance().info("Сигнатура '" + sig.name + "': " + std::to_string(hits.size()) + " совпадений");
    for (usize i = 0; i < hits.size(); ++i) {
        Logger::instance().info("  [" + std::to_string(i) + "] 0x" + toHex(hits[i]));
    }
    if (!hits.empty()) {
        cache_[sig.name] = hits.front();
    }
    return hits;
}

std::optional<uptr> SignatureScanner::findWithFallback(const Signature& primary, const Signature& fallback) {
    if (auto a = find(primary, true)) {
        return a;
    }
    Logger::instance().info("Пробуем fallback для '" + primary.name + "'");
    return find(fallback, true);
}

std::optional<uptr> SignatureScanner::resolveRip(uptr insnAddr, int dispOffset, int insnLength) const {
    i32 disp = 0;
    if (!memory_.read(insnAddr + static_cast<uptr>(dispOffset), disp)) {
        return std::nullopt;
    }
    return insnAddr + static_cast<uptr>(insnLength) + static_cast<uptr>(static_cast<i64>(disp));
}

std::optional<uptr> SignatureScanner::cached(std::string_view name) const {
    const auto it = cache_.find(std::string(name));
    if (it == cache_.end()) {
        return std::nullopt;
    }
    return it->second;
}

}  // namespace er
