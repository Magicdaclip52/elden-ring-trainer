#pragma once

#include "core/Types.h"
#include "memory/CodeCave.h"
#include "memory/ProcessMemory.h"
#include "memory/SignatureScanner.h"

#include <string>

namespace er {

class GameHooks {
public:
    static GameHooks& instance();

    bool resolve(ProcessMemory& memory, SignatureScanner& scanner);
    void shutdown(ProcessMemory& memory);

    [[nodiscard]] bool playerSiteFound() const { return playerSite_ != 0; }
    [[nodiscard]] bool soulsSiteFound() const { return soulsSite_ != 0; }
    [[nodiscard]] bool godSiteFound() const { return godSite_ != 0; }
    [[nodiscard]] bool weightSiteFound() const { return weightSite_ != 0; }
    [[nodiscard]] bool dropSiteFound() const { return dropSite_ != 0; }
    [[nodiscard]] bool stealthSiteFound() const { return stealthSite_ != 0; }
    [[nodiscard]] bool immuneSiteFound() const { return immuneSite_ != 0; }
    [[nodiscard]] bool horseSiteFound() const { return horseSite_ != 0; }
    [[nodiscard]] bool daySiteFound() const { return daySite_ != 0; }
    [[nodiscard]] bool locSiteFound() const { return locSite_ != 0; }
    [[nodiscard]] bool itemSiteFound() const { return itemSite_ != 0; }
    [[nodiscard]] bool dmgSiteFound() const { return dmgSite_ != 0; }
    [[nodiscard]] bool speedSiteFound() const { return speedSite_ != 0; }
    [[nodiscard]] bool noSoulLossSiteFound() const { return noSoulLossSite_ != 0; }
    [[nodiscard]] uptr noSoulLossSite() const { return noSoulLossSite_; }

    [[nodiscard]] bool playerHooked() const { return playerHooked_; }
    [[nodiscard]] bool soulsHooked() const { return soulsHooked_; }
    [[nodiscard]] bool hooksAttempted() const { return hooksAttempted_; }
    [[nodiscard]] std::string lastError() const { return lastError_; }

    bool ensurePlayerHook(ProcessMemory& memory);
    bool ensureSoulsHook(ProcessMemory& memory);
    bool ensureGodHook(ProcessMemory& memory);
    bool ensureImmuneHook(ProcessMemory& memory);
    bool ensureHorseHook(ProcessMemory& memory);
    bool ensureDayHook(ProcessMemory& memory);
    bool ensureLocationHook(ProcessMemory& memory);
    bool ensureDmgHook(ProcessMemory& memory);
    bool ensureWeightHook(ProcessMemory& memory);
    bool ensureDropHook(ProcessMemory& memory);
    bool ensureItemHook(ProcessMemory& memory);
    bool ensureSpeedHook(ProcessMemory& memory);
    void tick(ProcessMemory& memory);
    bool applyStealth(ProcessMemory& memory, bool on);
    bool applyNoSoulLoss(ProcessMemory& memory, bool on);

    void setHpFlag(ProcessMemory& memory, bool on);
    void setFocusFlag(ProcessMemory& memory, bool on);
    void setStaminaFlag(ProcessMemory& memory, bool on);
    void setMaxHp(ProcessMemory& memory, i32 value);
    void setMaxFp(ProcessMemory& memory, i32 value);
    void setMaxStamina(ProcessMemory& memory, i32 value);

    void setSoulEdit(ProcessMemory& memory, i32 value);
    void setSoulMul(ProcessMemory& memory, float mul);
    void setStat(ProcessMemory& memory, int index, i32 value);

    void setGodFlag(ProcessMemory& memory, bool on);
    void setImmuneFlag(ProcessMemory& memory, bool on);
    void setHorseFlag(ProcessMemory& memory, bool on);
    void setFreezeDay(ProcessMemory& memory, bool on);
    void pulseDayAdd(ProcessMemory& memory);
    void setFlyMode(ProcessMemory& memory, bool on);
    void pulseFly(ProcessMemory& memory, int dir);
    void setFreezeAi(ProcessMemory& memory, bool on);
    void setOneHit(ProcessMemory& memory, bool on);
    void setDmgMul(ProcessMemory& memory, float mul);
    void setDefMul(ProcessMemory& memory, float mul);
    void setGameSpeed(ProcessMemory& memory, bool on, float mul);

    bool removeWeightHook(ProcessMemory& memory);
    bool removeDropHook(ProcessMemory& memory);
    bool removeItemHook(ProcessMemory& memory);

private:
    bool installPlayerHook(ProcessMemory& memory);
    bool installSoulsHook(ProcessMemory& memory);
    bool installGodHook(ProcessMemory& memory);
    bool installImmuneHook(ProcessMemory& memory);
    bool installHorseHook(ProcessMemory& memory);
    bool installDayHook(ProcessMemory& memory);
    bool installLocationHook(ProcessMemory& memory);
    bool installDmgHook(ProcessMemory& memory);
    bool installSpeedHook(ProcessMemory& memory);
    bool writePlayerU32(ProcessMemory& memory, usize off, u32 v);
    bool writeSoulsU32(ProcessMemory& memory, usize off, u32 v);
    bool installSpeedAt(ProcessMemory& memory, CodeCave& cave, uptr site, const char* name);
    void resolveCsFlipper(ProcessMemory& memory);
    void copyLocPtrs(ProcessMemory& memory);
    void writeSpeedMul(ProcessMemory& memory, CodeCave& cave, bool on, float mul);

    CodeCave playerCave_{};
    CodeCave soulsCave_{};
    CodeCave godCave_{};
    CodeCave immuneCave_{};
    CodeCave horseCave_{};
    CodeCave dayCave_{};
    CodeCave locCave_{};
    CodeCave dmgCave_{};
    CodeCave weightCave_{};
    CodeCave dropCave_{};
    CodeCave itemCave_{};
    CodeCave speedCave_{};
    CodeCave speedCave2_{};

    uptr playerSite_ = 0;
    uptr soulsSite_ = 0;
    uptr godSite_ = 0;
    uptr weightSite_ = 0;
    uptr dropSite_ = 0;
    uptr stealthSite_ = 0;
    uptr immuneSite_ = 0;
    uptr horseSite_ = 0;
    uptr daySite_ = 0;
    uptr locSite_ = 0;
    uptr itemSite_ = 0;
    uptr dmgSite_ = 0;
    uptr speedSite_ = 0;
    uptr speedSite2_ = 0;
    uptr csFlipper_ = 0;
    u32 speedOrig268_ = 0;
    u32 speedOrig2CC_ = 0;
    bool speedSaved_ = false;
    uptr noSoulLossSite_ = 0;
    i32 playerDisp_ = 0;
    i32 horseDisp_ = 0;
    i32 dmgDisp_ = 0;
    bool soulsVariant41_ = false;
    bool playerHooked_ = false;
    bool soulsHooked_ = false;
    bool godHooked_ = false;
    bool immuneHooked_ = false;
    bool horseHooked_ = false;
    bool dayHooked_ = false;
    bool locHooked_ = false;
    bool dmgHooked_ = false;
    bool weightHooked_ = false;
    bool dropHooked_ = false;
    bool itemHooked_ = false;
    bool speedHooked_ = false;
    bool stealthOn_ = false;
    bool noSoulOn_ = false;
    bool hooksAttempted_ = false;
    std::string lastError_;

    static constexpr usize kFlags = 0x200;
    static constexpr usize kPtrs = 0x400;
};

}  // namespace er
