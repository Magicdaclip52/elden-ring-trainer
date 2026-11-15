#pragma once

#include "features/FeatureManager.h"
#include "game/GameAttach.h"

#include <atomic>
#include <thread>

namespace er {

class WorkerThread {
public:
    WorkerThread(GameAttach& game, FeatureManager& features);
    ~WorkerThread();

    void start();
    void stop();

private:
    void loop();

    GameAttach& game_;
    FeatureManager& features_;
    std::atomic<bool> stop_{false};
    std::thread thread_;
};

}  // namespace er
