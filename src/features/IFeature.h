#pragma once

#include "core/Types.h"
#include "memory/ProcessMemory.h"
#include "memory/SignatureScanner.h"
#include "ui/Hotkeys.h"

#include <string>
#include <string_view>

namespace er {

enum class FeatureKind {
    Toggle,
    ApplyValue,
    Hold,
};

enum class FeatureStatus {
    Unavailable,
    Ready,
    Enabled,
    Error,
};

enum class FeatureGroup {
    Survival,
    Items,
    Combat,
    World,
    Stats,
};

struct FeatureInfo {
    std::string id;
    std::string nameRu;
    std::string nameEn;
    FeatureGroup group = FeatureGroup::Survival;
    FeatureKind kind = FeatureKind::Toggle;
    Hotkey defaultHotkey{};
    float defaultFloat = 0.0f;
    int defaultInt = 0;
    bool risky = false;
    bool needsInject = false;
};

class IFeature {
public:
    virtual ~IFeature() = default;

    [[nodiscard]] virtual const FeatureInfo& info() const = 0;
    [[nodiscard]] virtual FeatureStatus status() const = 0;
    [[nodiscard]] virtual std::string_view reason() const = 0;

    virtual bool resolve(SignatureScanner& scanner) = 0;
    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual void tick(float dt) = 0;
    virtual void bindMemory(ProcessMemory* memory) { (void)memory; }
    virtual void applyNow() {}
    virtual void setIntValue(i32 v) { (void)v; }
};

}  // namespace er
