#include "hooks/GameHooks.h"

#include "core/Logger.h"
#include "hooks/CaveEmit.h"
#include "signatures/PatternTable.h"

#include <cstring>
#include <string>
#include <vector>

namespace er {
namespace {

bool commitCave(ProcessMemory& memory, CodeCave& cave, uptr site, usize patchLen, CaveEmit& e,
                const char* name) {
    std::string why;
    if (!e.finish(&why)) {
        Logger::instance().error(std::string(name) + ": " + why + " cave=0x" + toHex(e.base) +
                                 " bytes=" + std::to_string(e.b.size()));
        return false;
    }
    if (!cave.write(memory, 0, e.b) || !cave.installRelJmpHook(memory, site, patchLen)) {
        Logger::instance().error(std::string(name) + ": не удалось записать jmp");
        return false;
    }
    Logger::instance().info(std::string(name) + " ON @ 0x" + toHex(site));
    return true;
}

}  // namespace

GameHooks& GameHooks::instance() {
    static GameHooks g;
    return g;
}

bool GameHooks::writePlayerU32(ProcessMemory& memory, usize off, u32 v) {
    return playerHooked_ && playerCave_.writeU32(memory, kFlags + off, v);
}

bool GameHooks::writeSoulsU32(ProcessMemory& memory, usize off, u32 v) {
    return soulsHooked_ && soulsCave_.writeU32(memory, kFlags + off, v);
}

bool GameHooks::installPlayerHook(ProcessMemory& memory) {
    if (playerHooked_ || playerSite_ == 0) {
        return playerHooked_;
    }
    if (!memory.read(playerSite_ + 2, playerDisp_)) {
        return false;
    }
    if (!playerCave_.allocateNear(memory, playerSite_)) {
        lastError_ += "aobplayer: нет cave. ";
        return false;
    }

    const uptr cave = playerCave_.base();
    const uptr hp = cave + kFlags;
    const uptr fp = cave + kFlags + 4;
    const uptr st = cave + kFlags + 8;
    const uptr maxHp = cave + kFlags + 0x0C;
    const uptr maxFp = cave + kFlags + 0x10;
    const uptr maxSt = cave + kFlags + 0x14;
    const uptr playerData = cave + kPtrs;
    const uptr playerIns = cave + kPtrs + 8;

    CaveEmit e;
    e.base = cave;
    e.bytes({0x52, 0x48, 0x8B, 0xD0});  // push rdx; mov rdx, rax
    e.movRipRdx(playerData);
    e.bytes({0x48, 0x8B, 0x52, 0x08});  // mov rdx, [rdx+8]
    e.movRipRdx(playerIns);
    e.bytes({0x48, 0x8D, 0x90});  // lea rdx, [rax+disp]
    e.put32(playerDisp_);

    e.cmpRip1(hp);
    e.jne8("nohp");
    e.bytes({0x8B, 0x4A, 0x04, 0x89, 0x0A});
    e.L("nohp");
    e.cmpRip1(fp);
    e.jne8("nofp");
    e.bytes({0x8B, 0x4A, 0x14, 0x89, 0x4A, 0x10});
    e.L("nofp");
    e.cmpRip1(st);
    e.jne8("nost");
    e.bytes({0x8B, 0x4A, 0x20, 0x89, 0x4A, 0x1C});
    e.L("nost");

    e.movEcxRip(maxHp);
    e.bytes({0x83, 0xF9, 0x00});
    e.jle8("nomh");
    e.bytes({0x89, 0x4A, 0x08, 0x89, 0x4A, 0x0C});
    e.L("nomh");
    e.movEcxRip(maxFp);
    e.bytes({0x83, 0xF9, 0x00});
    e.jle8("nomf");
    e.bytes({0x89, 0x4A, 0x18});
    e.L("nomf");
    e.movEcxRip(maxSt);
    e.bytes({0x83, 0xF9, 0x00});
    e.jle8("noms");
    e.bytes({0x89, 0x4A, 0x24});
    e.L("noms");

    e.bytes({0x8B, 0x0A, 0x5A});  // mov ecx,[rdx]; pop rdx
    e.absJmp(playerSite_ + 6);

    for (usize off = 0; off <= 0x14; off += 4) {
        playerCave_.writeU32(memory, kFlags + off, 0);
    }
    if (!commitCave(memory, playerCave_, playerSite_, 6, e, "player")) {
        return false;
    }
    playerHooked_ = true;
    return true;
}

bool GameHooks::installSoulsHook(ProcessMemory& memory) {
    if (soulsHooked_ || soulsSite_ == 0) {
        return soulsHooked_;
    }
    if (!soulsCave_.allocateNear(memory, soulsSite_)) {
        lastError_ += "aobsouls: нет cave. ";
        return false;
    }

    const uptr cave = soulsCave_.base();
    const uptr soul = cave + kFlags;
    const uptr mul = cave + kFlags + 4;
    const uptr level = cave + kFlags + 8;

    CaveEmit e;
    e.base = cave;
    e.movRipRcx(cave + kPtrs);
    if (soulsVariant41_) {
        e.movR8dRip(soul);
        e.bytes({0x41, 0x83, 0xF8, 0x00});
        e.jle8("nosoul");
        e.bytes({0x44, 0x89, 0x41, 0x6C});
    } else {
        e.movR9dRip(soul);
        e.bytes({0x41, 0x83, 0xF9, 0x00});
        e.jle8("nosoul");
        e.bytes({0x44, 0x89, 0x49, 0x6C});
    }
    e.L("nosoul");

    e.bytes({0x83, 0xFA, 0x00});
    e.jle8("nomul");
    e.cmpRip0(mul);
    e.je8("nomul");
    e.bytes({0x52, 0xDB, 0x04, 0x24});  // push rdx; fild dword [rsp]
    e.fmulRip(mul);
    e.bytes({0xDB, 0x1C, 0x24, 0x5A});  // fistp dword [rsp]; pop rdx
    e.L("nomul");

    auto emitStat = [&](uptr slot, u8 rcxOff, const char* skip) {
        e.leaR11Rip(slot);
        e.bytes({0x45, 0x8B, 0x0B, 0x41, 0x83, 0xF9, 0x00});
        e.jle8(skip);
        e.bytes({0x44, 0x89, 0x49, rcxOff});
        e.L(skip);
    };
    emitStat(level, 0x68, "nlv");
    emitStat(level + 4, 0x3C, "ns1");
    emitStat(level + 8, 0x40, "ns2");
    emitStat(level + 12, 0x44, "ns3");
    emitStat(level + 16, 0x48, "ns4");
    emitStat(level + 20, 0x4C, "ns5");
    emitStat(level + 24, 0x50, "ns6");
    emitStat(level + 28, 0x54, "ns7");
    emitStat(level + 32, 0x58, "ns8");

    if (soulsVariant41_) {
        e.bytes({0x44, 0x8B, 0x41, 0x6C, 0x45, 0x33, 0xD2});
    } else {
        e.bytes({0x44, 0x8B, 0x49, 0x6C, 0x45, 0x33, 0xDB});
    }
    e.absJmp(soulsSite_ + 7);

    for (usize off = 0; off <= 0x28; off += 4) {
        soulsCave_.writeU32(memory, kFlags + off, 0);
    }
    if (!commitCave(memory, soulsCave_, soulsSite_, 7, e, "souls")) {
        return false;
    }
    soulsHooked_ = true;
    return true;
}

bool GameHooks::installHorseHook(ProcessMemory& memory) {
    if (horseHooked_ || horseSite_ == 0) {
        return horseHooked_;
    }
    if (!memory.read(horseSite_ + 2, horseDisp_)) {
        return false;
    }
    if (!horseCave_.allocateNear(memory, horseSite_)) {
        lastError_ += "aobhorse: нет cave. ";
        return false;
    }

    const uptr cave = horseCave_.base();
    const uptr flag = cave + kFlags;
    const uptr hdata = cave + kPtrs;
    const uptr hins = cave + kPtrs + 8;

    CaveEmit e;
    e.base = cave;
    e.movRipRdx(hdata);
    e.bytes({0x48, 0x8B, 0x4A, 0x08});
    e.movRipRcx(hins);
    e.bytes({0x48, 0x8D, 0x8A});
    e.put32(horseDisp_);
    e.cmpRip1(flag);
    e.jne8("code");
    e.bytes({0x50, 0x8B, 0x41, 0x04, 0x89, 0x01, 0x58});
    e.L("code");
    e.bytes({0x8B, 0x09});
    e.absJmp(horseSite_ + 6);

    horseCave_.writeU32(memory, kFlags, 0);
    if (!commitCave(memory, horseCave_, horseSite_, 6, e, "horse")) {
        return false;
    }
    horseHooked_ = true;
    return true;
}

bool GameHooks::installGodHook(ProcessMemory& memory) {
    if (godHooked_ || godSite_ == 0) {
        return godHooked_;
    }
    if (!ensurePlayerHook(memory)) {
        lastError_ += "godmode нужен aobplayer. ";
        return false;
    }
    ensureHorseHook(memory);
    if (!godCave_.allocateNear(memory, godSite_)) {
        lastError_ += "aobgodmode: нет cave. ";
        return false;
    }

    const uptr cave = godCave_.base();
    const uptr flag = cave + kFlags;
    const uptr pins = playerCave_.base() + kPtrs + 8;
    uptr hins = cave + kPtrs;
    if (horseHooked_) {
        hins = horseCave_.base() + kPtrs + 8;
    } else {
        godCave_.writeU64(memory, kPtrs, 0);
    }

    CaveEmit e;
    e.base = cave;
    e.bytes({0x66, 0xC1, 0xE9, 0x08, 0x50});  // shr cx,8; push rax
    e.cmpRip1(flag);
    e.jne8("code");
    e.movRaxRip(pins);
    e.bytes({0x48, 0x3B, 0xC6});  // cmp rax, rsi  (1.17: rsi = ChrIns)
    e.je8("hit");
    e.bytes({0x48, 0x3B, 0x46, 0x08});  // cmp rax, [rsi+8]
    e.je8("hit");
    e.movRaxRip(hins);
    e.bytes({0x48, 0x3B, 0xC6});
    e.je8("hit");
    e.bytes({0x48, 0x3B, 0x46, 0x08});
    e.jne8("code");
    e.L("hit");
    e.bytes({0xB1, 0x01});
    e.L("code");
    e.bytes({0x58, 0x84, 0xC9});
    e.absJmp(godSite_ + 6);

    godCave_.writeU32(memory, kFlags, 0);
    if (!commitCave(memory, godCave_, godSite_, 6, e, "godmode")) {
        return false;
    }
    godHooked_ = true;
    return true;
}

bool GameHooks::installImmuneHook(ProcessMemory& memory) {
    if (immuneHooked_ || immuneSite_ == 0) {
        return immuneHooked_;
    }
    if (!ensurePlayerHook(memory)) {
        lastError_ += "immunity нужен aobplayer. ";
        return false;
    }
    if (!immuneCave_.allocateNear(memory, immuneSite_)) {
        lastError_ += "aobimmunity: нет cave. ";
        return false;
    }

    const uptr cave = immuneCave_.base();
    const uptr flag = cave + kFlags;
    const uptr pins = playerCave_.base() + kPtrs + 8;

    CaveEmit e;
    e.base = cave;
    e.cmpRip1(flag);
    e.jne8("code");
    e.movRaxRip(pins);
    e.bytes({0x48, 0x3B, 0x41, 0x08});
    e.jne8("code");
    e.bytes({0x8B, 0x44, 0x91, 0x2C, 0x89, 0x44, 0x91, 0x10});
    e.L("code");
    e.bytes({0x8B, 0xC2, 0x8B, 0x44, 0x81, 0x2C});
    e.absJmp(immuneSite_ + 6);

    immuneCave_.writeU32(memory, kFlags, 0);
    if (!commitCave(memory, immuneCave_, immuneSite_, 6, e, "immunity")) {
        return false;
    }
    immuneHooked_ = true;
    return true;
}

bool GameHooks::installDayHook(ProcessMemory& memory) {
    if (dayHooked_ || daySite_ == 0) {
        return dayHooked_;
    }
    if (!dayCave_.allocateNear(memory, daySite_)) {
        lastError_ += "aobdaytime: нет cave. ";
        return false;
    }

    const uptr cave = dayCave_.base();
    const uptr frz = cave + kFlags;
    const uptr add = cave + kFlags + 4;

    CaveEmit e;
    e.base = cave;
    e.bytes({0xF3, 0x0F, 0x2C, 0xD0});
    e.cmpRip1(frz);
    e.jne8("nofz");
    e.bytes({0x31, 0xD2});
    e.L("nofz");
    e.cmpRip1(add);
    e.jne8("code");
    e.movImmRip0(add);
    e.bytes({0xBA, 0x80, 0xEE, 0x36, 0x00});  // mov edx, 3600000
    e.L("code");
    e.bytes({0x85, 0xD2});
    e.absJmp(daySite_ + 6);

    dayCave_.writeU32(memory, kFlags, 0);
    dayCave_.writeU32(memory, kFlags + 4, 0);
    if (!commitCave(memory, dayCave_, daySite_, 6, e, "daytime")) {
        return false;
    }
    dayHooked_ = true;
    return true;
}

bool GameHooks::installLocationHook(ProcessMemory& memory) {
    if (locHooked_ || locSite_ == 0) {
        return locHooked_;
    }
    if (!ensurePlayerHook(memory)) {
        lastError_ += "location нужен aobplayer. ";
        return false;
    }
    ensureHorseHook(memory);
    if (!locCave_.allocateNear(memory, locSite_)) {
        lastError_ += "aoblocation: нет cave. ";
        return false;
    }

    constexpr usize kLocFlags = 0x600;
    constexpr usize kLocPins = 0x610;
    constexpr usize kLocHins = 0x618;
    constexpr usize kLocPtr = 0x620;
    constexpr usize kSave1 = 0x630;
    constexpr usize kSave0 = 0x640;

    const uptr cave = locCave_.base();
    const uptr fly = cave + kLocFlags;
    const uptr up = cave + kLocFlags + 4;
    const uptr down = cave + kLocFlags + 8;
    const uptr frz = cave + kLocFlags + 12;
    const uptr pins = cave + kLocPins;
    const uptr hins = cave + kLocHins;
    const uptr locPtr = cave + kLocPtr;
    const uptr pdat = cave + 0x628;
    const uptr saveXmm1 = cave + kSave1;
    const uptr saveXmm0 = cave + kSave0;

    CaveEmit e;
    e.base = cave;
    e.movupsRipXmm1(saveXmm1);
    e.bytes({0x50, 0x68, 0x00, 0x00, 0x80, 0x3F});
    e.movRaxRip(pins);
    e.bytes({0x48, 0x39, 0xC3});
    e.je32("pl");
    e.bytes({0x48, 0x39, 0x43, 0x08});
    e.je32("pl");
    e.movRaxRip(hins);
    e.bytes({0x48, 0x39, 0xC3});
    e.je32("pl");
    e.bytes({0x48, 0x39, 0x43, 0x08});
    e.je32("pl");
    e.movRaxRip(pdat);
    e.bytes({0x48, 0x39, 0xC3});
    e.je32("pl");
    e.bytes({0x48, 0x39, 0x43, 0x08});
    e.jne32("notpl");
    e.L("pl");
    e.movRipRbx(locPtr);
    e.cmpRip1(fly);
    e.jne32("code");
    e.bytes({0xC6, 0x83, 0x92, 0x00, 0x00, 0x00, 0x01});
    e.bytes({0xC6, 0x83, 0x93, 0x00, 0x00, 0x00, 0x01});
    e.cmpRip1(up);
    e.jne32("noup");
    e.movImmRip0(up);
    e.bytes({0xF3, 0x0F, 0x10, 0x4B, 0x74, 0xF3, 0x0F, 0x58, 0x0C, 0x24, 0xF3, 0x0F, 0x11, 0x4B, 0x74});
    e.L("noup");
    e.cmpRip1(down);
    e.jne32("nodn");
    e.movImmRip0(down);
    e.bytes({0xF3, 0x0F, 0x10, 0x4B, 0x74, 0xF3, 0x0F, 0x5C, 0x0C, 0x24, 0xF3, 0x0F, 0x11, 0x4B, 0x74});
    e.L("nodn");
    e.movupsRipXmm0(saveXmm0);
    e.movssXmm1Rip(saveXmm0);
    e.bytes({0xF3, 0x0F, 0x11, 0x4B, 0x70});
    e.movssXmm1Rip(saveXmm0 + 8);
    e.bytes({0xF3, 0x0F, 0x11, 0x4B, 0x78, 0xC7, 0x43, 0x7C, 0x00, 0x00, 0x80, 0x3F});
    e.jmp32("skip");
    e.L("notpl");
    e.cmpRip1(frz);
    e.je32("skip");
    e.L("code");
    e.bytes({0x0F, 0x11, 0x43, 0x70, 0xC7, 0x43, 0x7C, 0x00, 0x00, 0x80, 0x3F});
    e.L("skip");
    e.bytes({0x48, 0x83, 0xC4, 0x08, 0x58});
    e.movupsXmm1Rip(saveXmm1);
    e.absJmp(locSite_ + 11);

    locCave_.writeU64(memory, kLocPins, 0);
    locCave_.writeU64(memory, kLocHins, 0);
    locCave_.writeU64(memory, 0x628, 0);
    locCave_.writeU32(memory, kLocFlags, 0);
    locCave_.writeU32(memory, kLocFlags + 4, 0);
    locCave_.writeU32(memory, kLocFlags + 8, 0);
    locCave_.writeU32(memory, kLocFlags + 12, 0);
    if (!commitCave(memory, locCave_, locSite_, 11, e, "location")) {
        return false;
    }
    locHooked_ = true;
    copyLocPtrs(memory);
    Logger::instance().info("location cave bytes=" + std::to_string(e.b.size()));
    return true;
}

bool GameHooks::installDmgHook(ProcessMemory& memory) {
    if (dmgHooked_ || dmgSite_ == 0) {
        return dmgHooked_;
    }
    if (!ensurePlayerHook(memory)) {
        lastError_ += "dmg нужен aobplayer. ";
        return false;
    }
    if (!memory.read(dmgSite_ + 3, dmgDisp_)) {
        return false;
    }
    ensureHorseHook(memory);
    if (!dmgCave_.allocateNear(memory, dmgSite_)) {
        lastError_ += "aobdmg: нет cave. ";
        return false;
    }

    const uptr cave = dmgCave_.base();
    const uptr dmg = cave + kFlags;
    const uptr dmul = cave + kFlags + 4;
    const uptr defm = cave + kFlags + 8;
    const uptr pdata = playerCave_.base() + kPtrs;
    const uptr hdata = horseHooked_ ? (horseCave_.base() + kPtrs) : (cave + kPtrs);

    CaveEmit e;
    e.base = cave;
    e.bytes({0x49, 0x8D, 0x96});  // lea rdx, [r14+disp]
    e.put32(dmgDisp_);
    e.bytes({0x83, 0x3A, 0x00});  // cmp dword [rdx], 0
    e.jle8("code");
    e.bytes({0x48, 0xB9});
    e.put64(pdata);
    e.bytes({0x4C, 0x39, 0x29});  // cmp [rcx], r13
    e.je8("pl");
    e.bytes({0x48, 0xB9});
    e.put64(hdata);
    e.bytes({0x4C, 0x39, 0x29});
    e.jne8("npl");
    e.L("pl");
    e.cmpRip0(defm);
    e.je8("code");
    e.bytes({0xDB, 0x02});
    e.fdivRip(defm);
    e.bytes({0xDB, 0x1A});
    e.jmp8("code");

    e.L("npl");
    e.cmpRip0(dmul);
    e.je8("ohk");
    e.bytes({0xDB, 0x02});
    e.fmulRip(dmul);
    e.bytes({0xDB, 0x1A});
    e.L("ohk");
    e.cmpRip1(dmg);
    e.jne8("code");
    e.bytes({0xC7, 0x02, 0x7F, 0x96, 0x98, 0x00});  // mov [rdx], 9999999

    e.L("code");
    e.bytes({0x8B, 0x12});  // mov edx, [rdx]
    e.absJmp(dmgSite_ + 7);

    if (!horseHooked_) {
        dmgCave_.writeU64(memory, kPtrs, 0);
    }
    dmgCave_.writeU32(memory, kFlags, 0);
    dmgCave_.writeU32(memory, kFlags + 4, 0);
    dmgCave_.writeU32(memory, kFlags + 8, 0);
    if (!commitCave(memory, dmgCave_, dmgSite_, 7, e, "dmg")) {
        return false;
    }
    dmgHooked_ = true;
    return true;
}

bool GameHooks::ensureWeightHook(ProcessMemory& memory) {
    if (weightHooked_ || weightSite_ == 0) {
        return weightHooked_;
    }
    if (!weightCave_.allocateNear(memory, weightSite_)) {
        return false;
    }
    CaveEmit e;
    e.base = weightCave_.base();
    e.bytes({0x0F, 0x57, 0xF6, 0xFF, 0xC3, 0x83, 0xFB, 0x05});
    e.absJmp(weightSite_ + 5);
    if (!commitCave(memory, weightCave_, weightSite_, 5, e, "weight")) {
        return false;
    }
    weightHooked_ = true;
    return true;
}

bool GameHooks::ensureDropHook(ProcessMemory& memory) {
    if (dropHooked_ || dropSite_ == 0) {
        return dropHooked_;
    }
    if (!dropCave_.allocateNear(memory, dropSite_)) {
        return false;
    }
    CaveEmit e;
    e.base = dropCave_.base();
    e.bytes({0x68, 0x00, 0x80, 0x3B, 0x45});  // push 3000.0f
    e.bytes({0xF3, 0x0F, 0x10, 0x3C, 0x24, 0x48, 0x83, 0xC4, 0x08, 0x48, 0x85, 0xD2});
    e.absJmp(dropSite_ + 7);
    if (!commitCave(memory, dropCave_, dropSite_, 7, e, "droprate")) {
        return false;
    }
    dropHooked_ = true;
    return true;
}

bool GameHooks::ensureSpeedHook(ProcessMemory& memory) { return installSpeedHook(memory); }

void GameHooks::tick(ProcessMemory& memory) {
    if (horseHooked_) {
        u32 flag = 0;
        u64 hdata = 0;
        memory.read(horseCave_.base() + kFlags, flag);
        memory.read(horseCave_.base() + kPtrs, hdata);
        if (flag == 1 && hdata != 0) {
            i32 mx = 0;
            if (memory.read(hdata + static_cast<uptr>(horseDisp_) + 4, mx) && mx > 0) {
                memory.write(hdata + static_cast<uptr>(horseDisp_), mx);
            }
        }
    }
    copyLocPtrs(memory);
}

void GameHooks::copyLocPtrs(ProcessMemory& memory) {
    if (!locHooked_ || !playerHooked_) {
        return;
    }
    u64 pins = 0;
    u64 pdat = 0;
    u64 hins = 0;
    memory.read(playerCave_.base() + kPtrs + 8, pins);
    memory.read(playerCave_.base() + kPtrs, pdat);
    locCave_.writeU64(memory, 0x610, pins);
    locCave_.writeU64(memory, 0x628, pdat);
    if (horseHooked_) {
        memory.read(horseCave_.base() + kPtrs + 8, hins);
        locCave_.writeU64(memory, 0x618, hins);
    }
}

bool GameHooks::ensureItemHook(ProcessMemory& memory) {
    if (itemHooked_ || itemSite_ == 0) {
        return itemHooked_;
    }
    if (!itemCave_.allocateNear(memory, itemSite_)) {
        return false;
    }
    CaveEmit e;
    e.base = itemCave_.base();
    e.bytes({0x0F, 0x42, 0xE8, 0x83, 0xF8, 0x01});
    e.jle8("code");
    e.bytes({0x3B, 0xEB});
    e.jg8("code");
    e.bytes({0x8B, 0xE8, 0x8B, 0xD8, 0x03, 0xD8});
    e.L("code");
    e.bytes({0x3B, 0xEB});
    e.absJmp(itemSite_ + 5);
    if (!commitCave(memory, itemCave_, itemSite_, 5, e, "itemusage")) {
        return false;
    }
    itemHooked_ = true;
    return true;
}

bool GameHooks::installSpeedAt(ProcessMemory& memory, CodeCave& cave, uptr site, const char* name) {
    if (site == 0) {
        return false;
    }
    if (cave.active()) {
        return true;
    }
    if (!cave.allocateNear(memory, site)) {
        lastError_ += std::string(name) + ": нет cave. ";
        return false;
    }
    const uptr base = cave.base();
    const uptr flag = base + kFlags;
    const uptr mul = base + kFlags + 4;

    CaveEmit e;
    e.base = base;
    e.bytes({0xF3, 0x0F, 0x10, 0x88, 0xCC, 0x02, 0x00, 0x00});
    e.bytes({0xF3, 0x0F, 0x59, 0x88, 0x68, 0x02, 0x00, 0x00});
    e.cmpRip1(flag);
    e.jne8("done");
    e.mulssXmm1Rip(mul);
    e.L("done");
    e.absJmp(site + 16);

    cave.writeU32(memory, kFlags, 0);
    cave.writeU32(memory, kFlags + 4, 0x3F800000);
    return commitCave(memory, cave, site, 16, e, name);
}

void GameHooks::resolveCsFlipper(ProcessMemory& memory) {
    if (csFlipper_ != 0 || speedSite_ == 0) {
        return;
    }
    i32 disp = 0;
    if (!memory.read(speedSite_ - 4, disp)) {
        return;
    }
    const uptr slot = speedSite_ + static_cast<uptr>(static_cast<i64>(disp));
    u64 ptr = 0;
    if (!memory.read(slot, ptr) || ptr == 0) {
        return;
    }
    csFlipper_ = ptr;
    Logger::instance().info("CSFlipper=0x" + toHex(csFlipper_));
}

bool GameHooks::installSpeedHook(ProcessMemory& memory) {
    if (speedSite_ == 0 && speedSite2_ == 0) {
        return false;
    }
    const bool a = installSpeedAt(memory, speedCave_, speedSite_, "gamespeed");
    const bool b = installSpeedAt(memory, speedCave2_, speedSite2_, "gamespeed2");
    resolveCsFlipper(memory);
    speedHooked_ = a || b || speedSite_ != 0;
    if (!a && !b) {
        Logger::instance().warn("gamespeed: cave не встал, пишем CSFlipper+0x268");
    }
    return speedHooked_;
}

bool GameHooks::ensurePlayerHook(ProcessMemory& memory) { return installPlayerHook(memory); }
bool GameHooks::ensureSoulsHook(ProcessMemory& memory) { return installSoulsHook(memory); }
bool GameHooks::ensureGodHook(ProcessMemory& memory) { return installGodHook(memory); }
bool GameHooks::ensureImmuneHook(ProcessMemory& memory) { return installImmuneHook(memory); }
bool GameHooks::ensureHorseHook(ProcessMemory& memory) { return installHorseHook(memory); }
bool GameHooks::ensureDayHook(ProcessMemory& memory) { return installDayHook(memory); }
bool GameHooks::ensureLocationHook(ProcessMemory& memory) { return installLocationHook(memory); }
bool GameHooks::ensureDmgHook(ProcessMemory& memory) { return installDmgHook(memory); }

bool GameHooks::removeWeightHook(ProcessMemory& memory) {
    if (!weightHooked_) {
        return true;
    }
    const bool ok = weightCave_.removeHook(memory);
    weightHooked_ = false;
    return ok;
}

bool GameHooks::removeDropHook(ProcessMemory& memory) {
    if (!dropHooked_) {
        return true;
    }
    const bool ok = dropCave_.removeHook(memory);
    dropHooked_ = false;
    return ok;
}

bool GameHooks::removeItemHook(ProcessMemory& memory) {
    if (!itemHooked_) {
        return true;
    }
    const bool ok = itemCave_.removeHook(memory);
    itemHooked_ = false;
    return ok;
}

bool GameHooks::applyStealth(ProcessMemory& memory, bool on) {
    if (stealthSite_ == 0) {
        return false;
    }
    if (on && !stealthOn_) {
        const u8 patch[] = {0x66, 0x90};
        if (!memory.patch(stealthSite_, patch)) {
            return false;
        }
        stealthOn_ = true;
        Logger::instance().info("stealth ON @ 0x" + toHex(stealthSite_));
        return true;
    }
    if (!on && stealthOn_) {
        memory.restore(stealthSite_);
        stealthOn_ = false;
    }
    return true;
}

bool GameHooks::applyNoSoulLoss(ProcessMemory& memory, bool on) {
    if (noSoulLossSite_ == 0) {
        return false;
    }
    if (on && !noSoulOn_) {
        const u8 patch[] = {0xEB, 0x03};
        if (!memory.patch(noSoulLossSite_, patch)) {
            return false;
        }
        noSoulOn_ = true;
        Logger::instance().info("nosoullost ON @ 0x" + toHex(noSoulLossSite_));
        return true;
    }
    if (!on && noSoulOn_) {
        memory.restore(noSoulLossSite_);
        noSoulOn_ = false;
    }
    return true;
}

bool GameHooks::resolve(ProcessMemory& /*memory*/, SignatureScanner& scanner) {
    hooksAttempted_ = true;
    lastError_.clear();
    playerSite_ = soulsSite_ = godSite_ = weightSite_ = dropSite_ = stealthSite_ = 0;
    immuneSite_ = horseSite_ = daySite_ = locSite_ = itemSite_ = dmgSite_ = speedSite_ = speedSite2_ =
        noSoulLossSite_ = 0;
    csFlipper_ = 0;
    speedSaved_ = false;

    logPatternHits(scanner);
    Logger::instance().info("хуки ставлю только когда фичу включают");

    auto take = [&](uptr& dst, const Signature& a, const Signature* b = nullptr) {
        if (auto s = scanner.find(a, true)) {
            dst = *s;
            return;
        }
        if (b) {
            if (auto s = scanner.find(*b, true)) {
                dst = *s;
            }
        }
    };

    Signature pHook{"player_hook",
                    "8B 88 ?? ?? 00 00 89 ?? ?? ?? 00 00 ?? 8B ?? ?? ?? ?? 00 00 48 8B ?? E8 ?? ?? ?? ?? 89 ?? ?? ?? "
                    "00 00"};
    Signature pHookFb{
        "player_hook_fb",
        "8B 88 ?? ?? 00 00 89 ?? ?? ?? 00 00 ?? 8B ?? ?? ?? 00 00 48 8B ?? E8 ?? ?? ?? ?? 89 ?? ?? 00 00"};
    take(playerSite_, pHook, &pHookFb);

    Signature souls{"souls_hook", "44 8B 41 6C 45 33 D2 ?? 89 ?? 24 ?? 45 8D 0C 10"};
    Signature soulsFb{"souls_hook_fb", "44 8B 49 6C 45 33 DB ?? 89 ?? 24 ?? 45 8D 04 11"};
    if (auto hit = scanner.find(souls, true)) {
        soulsSite_ = *hit;
        soulsVariant41_ = true;
    } else if (auto hitFb = scanner.find(soulsFb, true)) {
        soulsSite_ = *hitFb;
        soulsVariant41_ = false;
    }

    Signature god{"godmode",
                  "66 C1 E9 08 84 C9 75 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CE E8 ?? ?? ?? ?? 83 ?? 03 0F 85 ?? ?? "
                  "00 00"};
    take(godSite_, god);

    Signature w{"weight", "FF C3 83 FB 05 7C ?? ?? ?? ?? ?? ?? 0F 28 C6"};
    take(weightSite_, w);
    Signature dr{"droprate", "41 0F 28 F8 48 85 D2 74 ?? BA ?? 00 00 00 ?? ?? ?? E8 ?? ?? ?? ?? 84 C0"};
    take(dropSite_, dr);
    Signature st{"stealth", "74 0C ?? 88 ?? ?? ?? 00 00 E9 ?? ?? 00 00 80 ?? ?? ?? 00 00 00 74"};
    take(stealthSite_, st);
    Signature im{"immunity", "8B C2 8B 44 81 2C C3"};
    take(immuneSite_, im);
    Signature hs{"horse", "8B 8A ?? ?? 00 00 89 ?? ?? ?? 8B ?? E8 ?? ?? ?? ?? 83 63 ?? FD"};
    take(horseSite_, hs);
    Signature dy{"daytime", "F3 0F 2C D0 85 D2 7E ?? 48 ?? ?? E8"};
    take(daySite_, dy);
    Signature loc{"location", "0F 11 43 70 C7 43 7C 00 00 80 3F 48 8B ?? 24"};
    take(locSite_, loc);

    Signature noLoss{"no_soul_loss", "8B 00 89 ?? 6C 48 8B ?? ?? ?? ?? ?? 48 85 ?? 75"};
    Signature noLoss2{"no_soul_loss2", "8B 00 89 ?? 6C 48 8B ?? ?? ?? ?? 48 85 ?? 75"};
    take(noSoulLossSite_, noLoss, &noLoss2);

    Signature item{"itemusage", "0F 42 E8 3B EB 8B D5 ?? ?? ?? ?? ?? 73 ?? E8"};
    Signature item2{"itemusage2", "0F 42 E8 3B EB 8B D5 ?? ?? ?? ?? ?? 73"};
    take(itemSite_, item, &item2);

    Signature dmg{"dmg1", "41 8B 96 ?? ?? 00 00 48 8B CE ?? 0F B6 ?? ?? ?? 00 00"};
    Signature dmg2{"dmg2", "41 8B 96 ?? ?? 00 00 49 8B CD ?? 0F B6 ?? ?? ?? 00 00"};
    Signature dmg3{"dmg3", "41 8B 96 ?? ?? 00 00 ?? ?? ?? ?? F3 ?? 0F 10 ?? ?? ?? 00 00 85"};
    if (auto hit = scanner.find(dmg, true)) {
        dmgSite_ = *hit;
    } else if (auto hit2 = scanner.find(dmg2, true)) {
        dmgSite_ = *hit2;
    } else if (auto hit3 = scanner.find(dmg3, true)) {
        dmgSite_ = *hit3;
    }

    Signature spd{"gamespeed", "F3 0F 10 88 CC 02 00 00 F3 0F 59 88 68 02 00 00"};
    {
        const auto hits = scanner.findAll(spd);
        if (!hits.empty()) {
            speedSite_ = hits[0];
        }
        if (hits.size() > 1) {
            speedSite2_ = hits[1];
        }
    }

    return playerSite_ != 0 || soulsSite_ != 0;
}

void GameHooks::shutdown(ProcessMemory& memory) {
    applyStealth(memory, false);
    applyNoSoulLoss(memory, false);
    if (speedSaved_ && csFlipper_ != 0) {
        memory.write(csFlipper_ + 0x268, speedOrig268_);
    }
    locCave_.removeHook(memory);
    godCave_.removeHook(memory);
    immuneCave_.removeHook(memory);
    dmgCave_.removeHook(memory);
    horseCave_.removeHook(memory);
    dayCave_.removeHook(memory);
    weightCave_.removeHook(memory);
    dropCave_.removeHook(memory);
    itemCave_.removeHook(memory);
    speedCave_.removeHook(memory);
    speedCave2_.removeHook(memory);
    playerCave_.removeHook(memory);
    soulsCave_.removeHook(memory);
    playerHooked_ = soulsHooked_ = godHooked_ = immuneHooked_ = horseHooked_ = false;
    dayHooked_ = locHooked_ = dmgHooked_ = weightHooked_ = dropHooked_ = itemHooked_ = speedHooked_ = false;
    csFlipper_ = 0;
    speedSaved_ = false;
}

void GameHooks::setHpFlag(ProcessMemory& memory, bool on) { writePlayerU32(memory, 0, on ? 1u : 0u); }
void GameHooks::setFocusFlag(ProcessMemory& memory, bool on) { writePlayerU32(memory, 4, on ? 1u : 0u); }
void GameHooks::setStaminaFlag(ProcessMemory& memory, bool on) { writePlayerU32(memory, 8, on ? 1u : 0u); }
void GameHooks::setMaxHp(ProcessMemory& memory, i32 value) {
    writePlayerU32(memory, 0x0C, static_cast<u32>(value));
}
void GameHooks::setMaxFp(ProcessMemory& memory, i32 value) {
    writePlayerU32(memory, 0x10, static_cast<u32>(value));
}
void GameHooks::setMaxStamina(ProcessMemory& memory, i32 value) {
    writePlayerU32(memory, 0x14, static_cast<u32>(value));
}

void GameHooks::setSoulEdit(ProcessMemory& memory, i32 value) {
    writeSoulsU32(memory, 0, static_cast<u32>(value));
}
void GameHooks::setSoulMul(ProcessMemory& memory, float mul) {
    u32 bits = 0;
    std::memcpy(&bits, &mul, 4);
    writeSoulsU32(memory, 4, bits);
}
void GameHooks::setStat(ProcessMemory& memory, int index, i32 value) {
    if (index < 0 || index > 8) {
        return;
    }
    writeSoulsU32(memory, static_cast<usize>(8 + index * 4), static_cast<u32>(value));
    if (!soulsHooked_) {
        return;
    }
    static constexpr u8 kOff[] = {0x68, 0x3C, 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58};
    u64 gameData = 0;
    if (!memory.read(soulsCave_.base() + kPtrs, gameData) || gameData == 0) {
        return;
    }
    memory.write(gameData + kOff[index], value);
}

void GameHooks::setGodFlag(ProcessMemory& memory, bool on) {
    if (godHooked_) {
        godCave_.writeU32(memory, kFlags, on ? 1u : 0u);
    }
}
void GameHooks::setImmuneFlag(ProcessMemory& memory, bool on) {
    if (immuneHooked_) {
        immuneCave_.writeU32(memory, kFlags, on ? 1u : 0u);
    }
}
void GameHooks::setHorseFlag(ProcessMemory& memory, bool on) {
    if (horseHooked_) {
        horseCave_.writeU32(memory, kFlags, on ? 1u : 0u);
    }
}
void GameHooks::setFreezeDay(ProcessMemory& memory, bool on) {
    if (dayHooked_) {
        dayCave_.writeU32(memory, kFlags, on ? 1u : 0u);
    }
}
void GameHooks::pulseDayAdd(ProcessMemory& memory) {
    if (dayHooked_) {
        dayCave_.writeU32(memory, kFlags + 4, 1u);
    }
}
void GameHooks::setFlyMode(ProcessMemory& memory, bool on) {
    if (locHooked_) {
        locCave_.writeU32(memory, 0x600, on ? 1u : 0u);
    }
}
void GameHooks::pulseFly(ProcessMemory& memory, int dir) {
    if (!locHooked_) {
        return;
    }
    if (dir > 0) {
        locCave_.writeU32(memory, 0x604, 1u);
    } else {
        locCave_.writeU32(memory, 0x608, 1u);
    }
}
void GameHooks::setFreezeAi(ProcessMemory& memory, bool on) {
    if (locHooked_) {
        locCave_.writeU32(memory, 0x60C, on ? 1u : 0u);
    }
}
void GameHooks::setOneHit(ProcessMemory& memory, bool on) {
    if (dmgHooked_) {
        dmgCave_.writeU32(memory, kFlags, on ? 1u : 0u);
    }
}
void GameHooks::setDmgMul(ProcessMemory& memory, float mul) {
    if (!dmgHooked_) {
        return;
    }
    u32 bits = 0;
    std::memcpy(&bits, &mul, 4);
    dmgCave_.writeU32(memory, kFlags + 4, bits);
}
void GameHooks::setDefMul(ProcessMemory& memory, float mul) {
    if (!dmgHooked_) {
        return;
    }
    u32 bits = 0;
    std::memcpy(&bits, &mul, 4);
    dmgCave_.writeU32(memory, kFlags + 8, bits);
}

void GameHooks::writeSpeedMul(ProcessMemory& memory, CodeCave& cave, bool on, float mul) {
    if (!cave.active()) {
        return;
    }
    cave.writeU32(memory, kFlags, on ? 1u : 0u);
    u32 bits = 0x3F800000;
    if (on) {
        std::memcpy(&bits, &mul, 4);
    }
    cave.writeU32(memory, kFlags + 4, bits);
}

void GameHooks::setGameSpeed(ProcessMemory& memory, bool on, float mul) {
    writeSpeedMul(memory, speedCave_, on, mul);
    writeSpeedMul(memory, speedCave2_, on, mul);
    resolveCsFlipper(memory);
    if (csFlipper_ == 0) {
        return;
    }
    if (!speedSaved_) {
        memory.read(csFlipper_ + 0x268, speedOrig268_);
        speedSaved_ = true;
    }
    if (on) {
        u32 bits = 0;
        std::memcpy(&bits, &mul, 4);
        memory.write(csFlipper_ + 0x268, bits);
    } else {
        memory.write(csFlipper_ + 0x268, speedOrig268_);
    }
}

}  // namespace er
