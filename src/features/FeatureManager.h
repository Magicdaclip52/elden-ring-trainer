#pragma once

#include "features/IFeature.h"
#include "memory/ProcessMemory.h"
#include "memory/SignatureScanner.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace er {

class FeatureManager {
public:
    FeatureManager();

    void bindMemory(ProcessMemory* memory);
    void resolveAll(SignatureScanner& scanner);
    void disableAll();
    void shutdown();

    void requestEnable(std::string_view id, bool on);
    void requestApply(std::string_view id);
    void tick(float dt);

    [[nodiscard]] IFeature* find(std::string_view id);
    [[nodiscard]] const std::vector<std::unique_ptr<IFeature>>& features() const { return features_; }

private:
    void syncStates();

    std::vector<std::unique_ptr<IFeature>> features_;
    std::unordered_map<std::string, IFeature*> byId_;
    std::unordered_map<std::string, std::atomic<bool>> wanted_;
    ProcessMemory* memory_ = nullptr;
    std::mutex mu_;
};

}  // namespace er
