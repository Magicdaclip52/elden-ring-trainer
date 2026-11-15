#include "worker/WorkerThread.h"

#include "core/Logger.h"

#include <chrono>

namespace er {

WorkerThread::WorkerThread(GameAttach& game, FeatureManager& features)
    : game_(game), features_(features) {}

WorkerThread::~WorkerThread() {
    stop();
}

void WorkerThread::start() {
    stop_ = false;
    thread_ = std::thread([this] { loop(); });
}

void WorkerThread::stop() {
    stop_ = true;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void WorkerThread::loop() {
    using clock = std::chrono::steady_clock;
    auto nextAttach = clock::now();
    auto nextTick = clock::now();
    bool resolved = false;
    bool hooksResolved = false;

    Logger::instance().info("Рабочий поток запущен");
    while (!stop_) {
        const auto now = clock::now();
        if (now >= nextAttach) {
            game_.tick();
            const auto snap = game_.snapshot();
            if (snap.attached && !snap.eacLoaded) {
                features_.bindMemory(&game_.memory());
                if (!hooksResolved) {
                    features_.resolveAll(game_.scanner());
                    hooksResolved = true;
                    Logger::instance().info("сигнатуры прогнал");
                }
                resolved = true;
            }
            if (!snap.attached) {
                if (resolved) {
                    features_.shutdown();
                }
                resolved = false;
                hooksResolved = false;
            }
            nextAttach = now + std::chrono::milliseconds(500);
        }
        if (now >= nextTick) {
            const auto snap = game_.snapshot();
            if (snap.attached && !snap.eacLoaded) {
                features_.tick(1.0f / 30.0f);
            }
            nextTick = now + std::chrono::milliseconds(33);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    features_.shutdown();
    game_.detach();
    Logger::instance().info("Рабочий поток остановлен, патчи восстановлены");
}

}  // namespace er
