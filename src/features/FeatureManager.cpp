#include "features/FeatureManager.h"

#include "core/Config.h"
#include "features/FeatureCatalog.h"
#include "hooks/GameHooks.h"

#include <utility>

namespace er {
namespace {

class PlayerFlagFeature final : public IFeature {
public:
    PlayerFlagFeature(FeatureInfo info, int flagIndex)
        : info_(std::move(info)), flagIndex_(flagIndex) {}

    const FeatureInfo& info() const override { return info_; }
    FeatureStatus status() const override { return status_; }
    std::string_view reason() const override { return reason_; }

    bool resolve(SignatureScanner&) override {
        if (GameHooks::instance().playerSiteFound()) {
            status_ = FeatureStatus::Ready;
            reason_ = "Сигнатура найдена. Хук ставится при включении.";
            return true;
        }
        status_ = FeatureStatus::Unavailable;
        reason_ = "Нет aobplayer на 1.17.2";
        return false;
    }

    void enable() override {
        if (!memory_ || !GameHooks::instance().ensurePlayerHook(*memory_)) {
            status_ = FeatureStatus::Error;
            reason_ = "Не удалось поставить хук HP (cave). Игра не патчилась.";
            enabled_ = false;
            return;
        }
        enabled_ = true;
        status_ = FeatureStatus::Enabled;
        applyFlag(true);
    }

    void disable() override {
        enabled_ = false;
        if (memory_ && status_ != FeatureStatus::Unavailable) {
            applyFlag(false);
            status_ = FeatureStatus::Ready;
        }
    }

    void tick(float) override {}
    void bindMemory(ProcessMemory* m) override { memory_ = m; }

private:
    void applyFlag(bool on) {
        auto& h = GameHooks::instance();
        switch (flagIndex_) {
            case 0:
                h.setHpFlag(*memory_, on);
                break;
            case 1:
                h.setFocusFlag(*memory_, on);
                break;
            case 2:
                h.setStaminaFlag(*memory_, on);
                break;
            default:
                break;
        }
    }

    FeatureInfo info_;
    int flagIndex_;
    FeatureStatus status_ = FeatureStatus::Unavailable;
    std::string reason_ = "Нет хука";
    bool enabled_ = false;
    ProcessMemory* memory_ = nullptr;
};

class EditRunesFeature final : public IFeature {
public:
    explicit EditRunesFeature(FeatureInfo info) : info_(std::move(info)) {}

    const FeatureInfo& info() const override { return info_; }
    FeatureStatus status() const override { return status_; }
    std::string_view reason() const override { return reason_; }

    bool resolve(SignatureScanner&) override {
        if (GameHooks::instance().soulsSiteFound()) {
            status_ = FeatureStatus::Ready;
            reason_ = "Сигнатура рун найдена. Хук ставится при включении.";
            return true;
        }
        status_ = FeatureStatus::Unavailable;
        reason_ = "Нет aobsouls";
        return false;
    }

    void enable() override {
        if (!memory_ || !GameHooks::instance().ensureSoulsHook(*memory_)) {
            status_ = FeatureStatus::Error;
            reason_ = "Не удалось поставить хук рун. Игра не патчилась.";
            enabled_ = false;
            return;
        }
        enabled_ = true;
        status_ = FeatureStatus::Enabled;
        apply();
    }

    void disable() override {
        enabled_ = false;
        if (memory_) {
            GameHooks::instance().setSoulEdit(*memory_, 0);
        }
        if (status_ != FeatureStatus::Unavailable) {
            status_ = FeatureStatus::Ready;
        }
    }

    void tick(float) override {
        if (enabled_) {
            apply();
        }
    }

    void applyNow() override { apply(); }
    void bindMemory(ProcessMemory* m) override { memory_ = m; }
    void setIntValue(i32 v) override { value_ = v; }

private:
    void apply() {
        if (!memory_ || !GameHooks::instance().soulsHooked()) {
            return;
        }
        GameHooks::instance().setSoulEdit(*memory_, value_);
    }

    FeatureInfo info_;
    FeatureStatus status_ = FeatureStatus::Unavailable;
    std::string reason_ = "Нет хука рун";
    bool enabled_ = false;
    ProcessMemory* memory_ = nullptr;
    i32 value_ = 99999999;
};

enum class ToggleKind {
    God,
    Weight,
    Drop,
    Stealth,
    Immune,
    Horse,
    NoSoul,
    Item,
    FreezeDay,
    Fly,
    FreezeAi,
    OneHit,
    DmgMul,
    DefMul,
    SoulMul,
    GameSpeed,
};

class ToggleHookFeature final : public IFeature {
public:
    ToggleHookFeature(FeatureInfo info, ToggleKind kind) : info_(std::move(info)), kind_(kind) {}

    const FeatureInfo& info() const override { return info_; }
    FeatureStatus status() const override { return status_; }
    std::string_view reason() const override { return reason_; }

    bool resolve(SignatureScanner&) override {
        auto& h = GameHooks::instance();
        bool ok = false;
        switch (kind_) {
            case ToggleKind::God:
                ok = h.godSiteFound();
                break;
            case ToggleKind::Weight:
                ok = h.weightSiteFound();
                break;
            case ToggleKind::Drop:
                ok = h.dropSiteFound();
                break;
            case ToggleKind::Stealth:
                ok = h.stealthSiteFound();
                break;
            case ToggleKind::Immune:
                ok = h.immuneSiteFound();
                break;
            case ToggleKind::Horse:
                ok = h.horseSiteFound();
                break;
            case ToggleKind::NoSoul:
                ok = h.noSoulLossSiteFound();
                break;
            case ToggleKind::Item:
                ok = h.itemSiteFound();
                break;
            case ToggleKind::FreezeDay:
                ok = h.daySiteFound();
                break;
            case ToggleKind::Fly:
            case ToggleKind::FreezeAi:
                ok = h.locSiteFound();
                break;
            case ToggleKind::OneHit:
            case ToggleKind::DmgMul:
            case ToggleKind::DefMul:
                ok = h.dmgSiteFound();
                break;
            case ToggleKind::SoulMul:
                ok = h.soulsSiteFound();
                break;
            case ToggleKind::GameSpeed:
                ok = h.speedSiteFound();
                break;
        }
        if (ok) {
            status_ = FeatureStatus::Ready;
            reason_ = "Готово. Хук/патч при включении.";
            return true;
        }
        status_ = FeatureStatus::Unavailable;
        reason_ = "Сигнатура не найдена на 1.17.2";
        return false;
    }

    void enable() override {
        if (!memory_) {
            fail("Нет процесса");
            return;
        }
        auto& h = GameHooks::instance();
        auto& cfg = Config::instance();
        bool ok = false;
        switch (kind_) {
            case ToggleKind::God:
                ok = h.ensureGodHook(*memory_);
                if (ok) {
                    h.setGodFlag(*memory_, true);
                }
                break;
            case ToggleKind::Weight:
                ok = h.ensureWeightHook(*memory_);
                break;
            case ToggleKind::Drop:
                ok = h.ensureDropHook(*memory_);
                break;
            case ToggleKind::Stealth:
                ok = h.applyStealth(*memory_, true);
                break;
            case ToggleKind::Immune:
                ok = h.ensureImmuneHook(*memory_);
                if (ok) {
                    h.setImmuneFlag(*memory_, true);
                }
                break;
            case ToggleKind::Horse:
                ok = h.ensureHorseHook(*memory_);
                if (ok) {
                    h.setHorseFlag(*memory_, true);
                }
                break;
            case ToggleKind::NoSoul:
                ok = h.applyNoSoulLoss(*memory_, true);
                break;
            case ToggleKind::Item:
                ok = h.ensureItemHook(*memory_);
                break;
            case ToggleKind::FreezeDay:
                ok = h.ensureDayHook(*memory_);
                if (ok) {
                    h.setFreezeDay(*memory_, true);
                }
                break;
            case ToggleKind::Fly:
                ok = h.ensureLocationHook(*memory_);
                if (ok) {
                    h.setFlyMode(*memory_, true);
                }
                break;
            case ToggleKind::FreezeAi:
                ok = h.ensureLocationHook(*memory_);
                if (ok) {
                    h.setFreezeAi(*memory_, true);
                }
                break;
            case ToggleKind::OneHit:
                ok = h.ensureDmgHook(*memory_);
                if (ok) {
                    h.setOneHit(*memory_, true);
                }
                break;
            case ToggleKind::DmgMul:
                ok = h.ensureDmgHook(*memory_);
                if (ok) {
                    float v = 2.0f;
                    if (const auto it = cfg.featureFloat.find(info_.id); it != cfg.featureFloat.end()) {
                        v = it->second;
                    }
                    h.setDmgMul(*memory_, v);
                }
                break;
            case ToggleKind::DefMul:
                ok = h.ensureDmgHook(*memory_);
                if (ok) {
                    float v = 2.0f;
                    if (const auto it = cfg.featureFloat.find(info_.id); it != cfg.featureFloat.end()) {
                        v = it->second;
                    }
                    h.setDefMul(*memory_, v);
                }
                break;
            case ToggleKind::SoulMul:
                ok = h.ensureSoulsHook(*memory_);
                if (ok) {
                    float v = 2.0f;
                    if (const auto it = cfg.featureFloat.find(info_.id); it != cfg.featureFloat.end()) {
                        v = it->second;
                    }
                    h.setSoulMul(*memory_, v);
                }
                break;
            case ToggleKind::GameSpeed:
                ok = h.ensureSpeedHook(*memory_);
                if (ok) {
                    float v = 2.5f;
                    if (const auto it = cfg.featureFloat.find(info_.id); it != cfg.featureFloat.end()) {
                        v = it->second;
                    }
                    h.setGameSpeed(*memory_, true, v);
                }
                break;
        }
        if (!ok) {
            fail("Не удалось включить (cave/патч). Игра не должна была упасть.");
            return;
        }
        enabled_ = true;
        status_ = FeatureStatus::Enabled;
        reason_ = "Включено";
    }

    void disable() override {
        if (memory_) {
            auto& h = GameHooks::instance();
            switch (kind_) {
                case ToggleKind::God:
                    h.setGodFlag(*memory_, false);
                    break;
                case ToggleKind::Weight:
                    h.removeWeightHook(*memory_);
                    break;
                case ToggleKind::Drop:
                    h.removeDropHook(*memory_);
                    break;
                case ToggleKind::Stealth:
                    h.applyStealth(*memory_, false);
                    break;
                case ToggleKind::Immune:
                    h.setImmuneFlag(*memory_, false);
                    break;
                case ToggleKind::Horse:
                    h.setHorseFlag(*memory_, false);
                    break;
                case ToggleKind::NoSoul:
                    h.applyNoSoulLoss(*memory_, false);
                    break;
                case ToggleKind::Item:
                    h.removeItemHook(*memory_);
                    break;
                case ToggleKind::FreezeDay:
                    h.setFreezeDay(*memory_, false);
                    break;
                case ToggleKind::Fly:
                    h.setFlyMode(*memory_, false);
                    break;
                case ToggleKind::FreezeAi:
                    h.setFreezeAi(*memory_, false);
                    break;
                case ToggleKind::OneHit:
                    h.setOneHit(*memory_, false);
                    break;
                case ToggleKind::DmgMul:
                    h.setDmgMul(*memory_, 0.0f);
                    break;
                case ToggleKind::DefMul:
                    h.setDefMul(*memory_, 0.0f);
                    break;
                case ToggleKind::SoulMul:
                    h.setSoulMul(*memory_, 0.0f);
                    break;
                case ToggleKind::GameSpeed:
                    h.setGameSpeed(*memory_, false, 1.0f);
                    break;
            }
        }
        enabled_ = false;
        if (status_ != FeatureStatus::Unavailable) {
            status_ = FeatureStatus::Ready;
            reason_ = "Готово";
        }
    }

    void tick(float) override {
        if (!enabled_ || !memory_) {
            return;
        }
        if (kind_ == ToggleKind::Horse) {
            GameHooks::instance().tick(*memory_);
            return;
        }
        if (kind_ != ToggleKind::DmgMul && kind_ != ToggleKind::DefMul && kind_ != ToggleKind::SoulMul &&
            kind_ != ToggleKind::GameSpeed) {
            return;
        }
        auto& cfg = Config::instance();
        float v = 2.0f;
        if (const auto it = cfg.featureFloat.find(info_.id); it != cfg.featureFloat.end()) {
            v = it->second;
        }
        auto& h = GameHooks::instance();
        if (kind_ == ToggleKind::DmgMul) {
            h.setDmgMul(*memory_, v);
        } else if (kind_ == ToggleKind::DefMul) {
            h.setDefMul(*memory_, v);
        } else if (kind_ == ToggleKind::SoulMul) {
            h.setSoulMul(*memory_, v);
        } else if (kind_ == ToggleKind::GameSpeed) {
            h.setGameSpeed(*memory_, true, v);
        }
    }

    void bindMemory(ProcessMemory* m) override { memory_ = m; }

private:
    void fail(const char* msg) {
        status_ = FeatureStatus::Error;
        reason_ = msg;
        enabled_ = false;
    }

    FeatureInfo info_;
    ToggleKind kind_;
    FeatureStatus status_ = FeatureStatus::Unavailable;
    std::string reason_ = "Нет хука";
    bool enabled_ = false;
    ProcessMemory* memory_ = nullptr;
};

enum class ApplyKind {
    DayPlus,
    FlyUp,
    FlyDown,
    Level,
    St1,
    St2,
    St3,
    St4,
    St5,
    St6,
    St7,
    St8,
    MaxHp,
    MaxFp,
    MaxSt,
};

class ApplyHookFeature final : public IFeature {
public:
    ApplyHookFeature(FeatureInfo info, ApplyKind kind) : info_(std::move(info)), kind_(kind) {}

    const FeatureInfo& info() const override { return info_; }
    FeatureStatus status() const override { return status_; }
    std::string_view reason() const override { return reason_; }

    bool resolve(SignatureScanner&) override {
        auto& h = GameHooks::instance();
        bool ok = false;
        switch (kind_) {
            case ApplyKind::DayPlus:
                ok = h.daySiteFound();
                break;
            case ApplyKind::FlyUp:
            case ApplyKind::FlyDown:
                ok = h.locSiteFound();
                break;
            case ApplyKind::MaxHp:
            case ApplyKind::MaxFp:
            case ApplyKind::MaxSt:
                ok = h.playerSiteFound();
                break;
            default:
                ok = h.soulsSiteFound();
                break;
        }
        if (ok) {
            status_ = FeatureStatus::Ready;
            reason_ = "Готово. Жми ПРИМЕНИТЬ.";
            return true;
        }
        status_ = FeatureStatus::Unavailable;
        reason_ = "Сигнатура не найдена";
        return false;
    }

    void enable() override {
        applyNow();
        if (kind_ == ApplyKind::FlyUp || kind_ == ApplyKind::FlyDown) {
            return;
        }
        if (status_ != FeatureStatus::Error) {
            status_ = FeatureStatus::Enabled;
        }
    }
    void disable() override {
        if (memory_) {
            auto& h = GameHooks::instance();
            if (kind_ == ApplyKind::MaxHp) {
                h.setMaxHp(*memory_, 0);
            } else if (kind_ == ApplyKind::MaxFp) {
                h.setMaxFp(*memory_, 0);
            } else if (kind_ == ApplyKind::MaxSt) {
                h.setMaxStamina(*memory_, 0);
            } else if (kind_ >= ApplyKind::Level && kind_ <= ApplyKind::St8) {
                h.setStat(*memory_, static_cast<int>(kind_) - static_cast<int>(ApplyKind::Level), 0);
            }
        }
        if (status_ != FeatureStatus::Unavailable) {
            status_ = FeatureStatus::Ready;
        }
    }
    void tick(float) override {}
    void bindMemory(ProcessMemory* m) override { memory_ = m; }

    void applyNow() override {
        if (!memory_) {
            return;
        }
        auto& h = GameHooks::instance();
        auto& cfg = Config::instance();
        i32 iv = info_.defaultInt;
        if (const auto it = cfg.featureInt.find(info_.id); it != cfg.featureInt.end()) {
            iv = it->second;
        }
        bool ok = false;
        switch (kind_) {
            case ApplyKind::DayPlus:
                ok = h.ensureDayHook(*memory_);
                if (ok) {
                    h.pulseDayAdd(*memory_);
                }
                break;
            case ApplyKind::FlyUp:
                ok = h.ensureLocationHook(*memory_);
                if (ok) {
                    h.pulseFly(*memory_, 1);
                }
                break;
            case ApplyKind::FlyDown:
                ok = h.ensureLocationHook(*memory_);
                if (ok) {
                    h.pulseFly(*memory_, -1);
                }
                break;
            case ApplyKind::MaxHp:
                ok = h.ensurePlayerHook(*memory_);
                if (ok) {
                    h.setMaxHp(*memory_, iv);
                }
                break;
            case ApplyKind::MaxFp:
                ok = h.ensurePlayerHook(*memory_);
                if (ok) {
                    h.setMaxFp(*memory_, iv);
                }
                break;
            case ApplyKind::MaxSt:
                ok = h.ensurePlayerHook(*memory_);
                if (ok) {
                    h.setMaxStamina(*memory_, iv);
                }
                break;
            default: {
                ok = h.ensureSoulsHook(*memory_);
                if (ok) {
                    int idx = 0;
                    switch (kind_) {
                        case ApplyKind::Level:
                            idx = 0;
                            break;
                        case ApplyKind::St1:
                            idx = 1;
                            break;
                        case ApplyKind::St2:
                            idx = 2;
                            break;
                        case ApplyKind::St3:
                            idx = 3;
                            break;
                        case ApplyKind::St4:
                            idx = 4;
                            break;
                        case ApplyKind::St5:
                            idx = 5;
                            break;
                        case ApplyKind::St6:
                            idx = 6;
                            break;
                        case ApplyKind::St7:
                            idx = 7;
                            break;
                        case ApplyKind::St8:
                            idx = 8;
                            break;
                        default:
                            break;
                    }
                    h.setStat(*memory_, idx, iv);
                }
                break;
            }
        }
        if (!ok) {
            status_ = FeatureStatus::Error;
            reason_ = "Не удалось применить";
        }
    }

private:
    FeatureInfo info_;
    ApplyKind kind_;
    FeatureStatus status_ = FeatureStatus::Unavailable;
    std::string reason_;
    ProcessMemory* memory_ = nullptr;
};

class StubFeature final : public IFeature {
public:
    explicit StubFeature(FeatureInfo info) : info_(std::move(info)) {
        reason_ = "пока заглушка";
    }

    const FeatureInfo& info() const override { return info_; }
    FeatureStatus status() const override { return status_; }
    std::string_view reason() const override { return reason_; }

    bool resolve(SignatureScanner&) override {
        status_ = FeatureStatus::Ready;
        reason_ = "пока заглушка";
        return true;
    }

    void enable() override {
        status_ = FeatureStatus::Enabled;
        reason_ = "вкл, хука нет";
    }

    void disable() override {
        status_ = FeatureStatus::Ready;
        reason_ = "пока заглушка";
    }

    void tick(float) override {}

private:
    FeatureInfo info_;
    FeatureStatus status_ = FeatureStatus::Ready;
    std::string reason_;
};

std::unique_ptr<IFeature> makeFeature(const FeatureInfo& info) {
    if (info.id == "infinite_hp") {
        return std::make_unique<PlayerFlagFeature>(info, 0);
    }
    if (info.id == "infinite_fp") {
        return std::make_unique<PlayerFlagFeature>(info, 1);
    }
    if (info.id == "infinite_stamina") {
        return std::make_unique<PlayerFlagFeature>(info, 2);
    }
    if (info.id == "edit_runes") {
        return std::make_unique<EditRunesFeature>(info);
    }

    static const std::pair<const char*, ToggleKind> toggles[] = {
        {"god_mode", ToggleKind::God},
        {"zero_weight", ToggleKind::Weight},
        {"drop_rate_100", ToggleKind::Drop},
        {"stealth_mode", ToggleKind::Stealth},
        {"immune_status", ToggleKind::Immune},
        {"infinite_horse_hp", ToggleKind::Horse},
        {"keep_runes_on_death", ToggleKind::NoSoul},
        {"infinite_item_usage", ToggleKind::Item},
        {"freeze_daytime", ToggleKind::FreezeDay},
        {"fly_mode", ToggleKind::Fly},
        {"freeze_enemies", ToggleKind::FreezeAi},
        {"one_hit_kill", ToggleKind::OneHit},
        {"damage_multiplier", ToggleKind::DmgMul},
        {"defense_multiplier", ToggleKind::DefMul},
        {"runes_multiplier", ToggleKind::SoulMul},
        {"game_speed", ToggleKind::GameSpeed},
    };
    for (const auto& t : toggles) {
        if (info.id == t.first) {
            return std::make_unique<ToggleHookFeature>(info, t.second);
        }
    }

    static const std::pair<const char*, ApplyKind> applies[] = {
        {"daytime_plus_1h", ApplyKind::DayPlus},
        {"fly_up", ApplyKind::FlyUp},
        {"fly_down", ApplyKind::FlyDown},
        {"edit_level", ApplyKind::Level},
        {"edit_vigor", ApplyKind::St1},
        {"edit_mind", ApplyKind::St2},
        {"edit_endurance", ApplyKind::St3},
        {"edit_strength", ApplyKind::St4},
        {"edit_dexterity", ApplyKind::St5},
        {"edit_intelligence", ApplyKind::St6},
        {"edit_faith", ApplyKind::St7},
        {"edit_arcane", ApplyKind::St8},
        {"edit_max_hp", ApplyKind::MaxHp},
        {"edit_max_fp", ApplyKind::MaxFp},
        {"edit_max_stamina", ApplyKind::MaxSt},
    };
    for (const auto& a : applies) {
        if (info.id == a.first) {
            return std::make_unique<ApplyHookFeature>(info, a.second);
        }
    }
    return std::make_unique<StubFeature>(info);
}

}  // namespace

FeatureManager::FeatureManager() {
    for (const auto& info : featureCatalog()) {
        auto f = makeFeature(info);
        byId_[f->info().id] = f.get();
        wanted_[f->info().id] = false;
        features_.push_back(std::move(f));
    }
}

void FeatureManager::bindMemory(ProcessMemory* memory) {
    memory_ = memory;
    for (auto& f : features_) {
        f->bindMemory(memory);
    }
}

void FeatureManager::resolveAll(SignatureScanner& scanner) {
    if (memory_) {
        GameHooks::instance().resolve(*memory_, scanner);
        bindMemory(memory_);
    }
    for (auto& f : features_) {
        f->resolve(scanner);
    }
}

void FeatureManager::shutdown() {
    disableAll();
    if (memory_) {
        GameHooks::instance().shutdown(*memory_);
    }
}

void FeatureManager::requestEnable(std::string_view id, bool on) {
    wanted_[std::string(id)] = on;
    syncStates();
}

void FeatureManager::requestApply(std::string_view id) {
    IFeature* f = find(id);
    if (!f || !memory_) {
        return;
    }
    if (id == "edit_runes") {
        auto& cfg = Config::instance();
        i32 val = 99999999;
        if (const auto it = cfg.featureInt.find("edit_runes"); it != cfg.featureInt.end()) {
            val = it->second;
        }
        f->setIntValue(val);
    }
    f->applyNow();
}

IFeature* FeatureManager::find(std::string_view id) {
    const auto it = byId_.find(std::string(id));
    return it == byId_.end() ? nullptr : it->second;
}

void FeatureManager::syncStates() {
    std::lock_guard lock(mu_);
    for (auto& f : features_) {
        const auto& id = f->info().id;
        const bool want = wanted_[id].load();
        const bool isOn = f->status() == FeatureStatus::Enabled;
        if (f->status() == FeatureStatus::Unavailable) {
            continue;
        }
        if (want && !isOn) {
            if (f->status() != FeatureStatus::Error) {
                f->enable();
            }
        } else if (!want && (isOn || f->status() == FeatureStatus::Error)) {
            f->disable();
        }
    }
}

void FeatureManager::disableAll() {
    for (auto& f : features_) {
        if (f->status() == FeatureStatus::Enabled) {
            f->disable();
        }
        wanted_[f->info().id] = false;
    }
}

void FeatureManager::tick(float dt) {
    syncStates();
    if (memory_) {
        GameHooks::instance().tick(*memory_);
    }
    for (auto& f : features_) {
        if (f->status() == FeatureStatus::Enabled) {
            f->tick(dt);
        }
    }
}

}  // namespace er
