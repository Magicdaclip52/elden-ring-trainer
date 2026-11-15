#pragma once

#include "core/Types.h"

#include <cstdint>
#include <climits>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace er {

struct CaveEmit {
    std::vector<u8> b;
    uptr base = 0;

    void bytes(std::initializer_list<u8> x) { b.insert(b.end(), x.begin(), x.end()); }

    void put32(::er::i32 v) {
        u8 t[4];
        std::memcpy(t, &v, 4);
        b.insert(b.end(), t, t + 4);
    }

    void put64(u64 v) {
        u8 t[8];
        std::memcpy(t, &v, 8);
        b.insert(b.end(), t, t + 8);
    }

    void L(const char* n) { lab_[n] = static_cast<int>(b.size()); }

    void jcc8(u8 cc, const char* n) {
        b.push_back(cc);
        b.push_back(0);
        j8_.push_back({static_cast<int>(b.size()) - 1, n});
    }

    void jmp8(const char* n) { jcc8(0xEB, n); }
    void je8(const char* n) { jcc8(0x74, n); }
    void jne8(const char* n) { jcc8(0x75, n); }
    void jle8(const char* n) { jcc8(0x7E, n); }
    void jg8(const char* n) { jcc8(0x7F, n); }

    void jmp32(const char* n) {
        bytes({0xE9});
        put32(0);
        j32_.push_back({static_cast<int>(b.size()) - 4, n});
    }
    void je32(const char* n) {
        bytes({0x0F, 0x84});
        put32(0);
        j32_.push_back({static_cast<int>(b.size()) - 4, n});
    }
    void jne32(const char* n) {
        bytes({0x0F, 0x85});
        put32(0);
        j32_.push_back({static_cast<int>(b.size()) - 4, n});
    }

    void absJmp(u64 target) {
        bytes({0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});
        put64(target);
    }

    void cmpRip1(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x83, 0x3D});
        put32(0);
        b.push_back(0x01);
        rips_.push_back({at, 7, 2, addr});
    }

    void cmpRip0(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x83, 0x3D});
        put32(0);
        b.push_back(0x00);
        rips_.push_back({at, 7, 2, addr});
    }

    void movEaxRip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x8B, 0x05});
        put32(0);
        rips_.push_back({at, 6, 2, addr});
    }

    void movEcxRip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x8B, 0x0D});
        put32(0);
        rips_.push_back({at, 6, 2, addr});
    }

    void movR9dRip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x44, 0x8B, 0x0D});
        put32(0);
        rips_.push_back({at, 7, 3, addr});
    }

    void movR8dRip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x44, 0x8B, 0x05});
        put32(0);
        rips_.push_back({at, 7, 3, addr});
    }

    void movRaxRip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x48, 0x8B, 0x05});
        put32(0);
        rips_.push_back({at, 7, 3, addr});
    }

    void movRipRdx(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x48, 0x89, 0x15});
        put32(0);
        rips_.push_back({at, 7, 3, addr});
    }

    void movRipRbx(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x48, 0x89, 0x1D});
        put32(0);
        rips_.push_back({at, 7, 3, addr});
    }

    void movupsRipXmm0(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x0F, 0x11, 0x05});
        put32(0);
        rips_.push_back({at, 7, 3, addr});
    }

    void fdivRip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0xD8, 0x35});
        put32(0);
        rips_.push_back({at, 6, 2, addr});
    }

    void movRipRcx(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x48, 0x89, 0x0D});
        put32(0);
        rips_.push_back({at, 7, 3, addr});
    }

    void leaR11Rip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x4C, 0x8D, 0x1D});
        put32(0);
        rips_.push_back({at, 7, 3, addr});
    }

    void movupsRipXmm1(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x0F, 0x11, 0x0D});
        put32(0);
        rips_.push_back({at, 7, 3, addr});
    }

    void movupsXmm1Rip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0x0F, 0x10, 0x0D});
        put32(0);
        rips_.push_back({at, 7, 3, addr});
    }

    void movssXmm1Rip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0xF3, 0x0F, 0x10, 0x0D});
        put32(0);
        rips_.push_back({at, 8, 4, addr});
    }

    void mulssXmm1Rip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0xF3, 0x0F, 0x59, 0x0D});
        put32(0);
        rips_.push_back({at, 8, 4, addr});
    }

    void fmulRip(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0xD8, 0x0D});
        put32(0);
        rips_.push_back({at, 6, 2, addr});
    }

    void movImmRip0(uptr addr) {
        const int at = static_cast<int>(b.size());
        bytes({0xC7, 0x05});
        put32(0);
        put32(0);
        rips_.push_back({at, 10, 2, addr});
    }

    [[nodiscard]] bool finish(std::string* why = nullptr) {
        for (const auto& r : rips_) {
            const std::int64_t delta =
                static_cast<std::int64_t>(r.target) - static_cast<std::int64_t>(base + static_cast<uptr>(r.at + r.len));
            if (delta < INT32_MIN || delta > INT32_MAX) {
                if (why) {
                    *why = "RIP at+" + std::to_string(r.at) + " len=" + std::to_string(r.len) +
                           " tgt=" + toHex(r.target);
                }
                return false;
            }
            const std::int32_t d32 = static_cast<std::int32_t>(delta);
            std::memcpy(b.data() + r.at + r.off, &d32, 4);
        }
        for (const auto& j : j8_) {
            const auto it = lab_.find(j.name);
            if (it == lab_.end()) {
                if (why) {
                    *why = "no label " + j.name;
                }
                return false;
            }
            const int rel = it->second - (j.at + 1);
            if (rel < -128 || rel > 127) {
                if (why) {
                    *why = "rel8 " + j.name + " d=" + std::to_string(rel);
                }
                return false;
            }
            b[static_cast<usize>(j.at)] = static_cast<u8>(rel);
        }
        for (const auto& j : j32_) {
            const auto it = lab_.find(j.name);
            if (it == lab_.end()) {
                if (why) {
                    *why = "no j32 label " + j.name;
                }
                return false;
            }
            const int rel = it->second - (j.at + 4);
            const std::int32_t d32 = static_cast<std::int32_t>(rel);
            std::memcpy(b.data() + j.at, &d32, 4);
        }
        return true;
    }

private:
    struct Rip {
        int at;
        int len;
        int off;
        uptr target;
    };
    struct J8 {
        int at;
        std::string name;
    };
    std::vector<Rip> rips_;
    std::vector<J8> j8_;
    std::vector<J8> j32_;
    std::unordered_map<std::string, int> lab_;
};

}  // namespace er
