#include "app/Application.h"

#include "core/Config.h"
#include "core/Logger.h"
#include "features/FeatureManager.h"
#include "game/GameAttach.h"
#include "ui/UiRenderer.h"
#include "worker/WorkerThread.h"

#include <Windows.h>
#include <ShlObj.h>

#include <exception>

namespace er {

int Application::run() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!Logger::instance().init()) {
        MessageBoxW(nullptr, L"Не удалось открыть ERTrainer.log", L"ERTrainer", MB_ICONERROR);
        return 1;
    }
    if (!IsUserAnAdmin()) {
        Logger::instance().warn("нет прав админа, OpenProcess часто отваливается, запускай от администратора");
    }
    Logger::instance().info("старт");

    Config::instance().load();

    GameAttach game;
    FeatureManager features;
    WorkerThread worker(game, features);
    worker.start();

    UiRenderer ui(game, features);
    const auto& w = Config::instance().window;
    if (!ui.create(w.x, w.y, w.w, w.h)) {
        worker.stop();
        Logger::instance().error("UI не создан");
        Logger::instance().shutdown();
        return 1;
    }

    int code = 0;
    try {
        code = ui.run();
    } catch (const std::exception& ex) {
        Logger::instance().error(std::string("Исключение UI: ") + ex.what());
        code = 1;
    }

    worker.stop();
    Logger::instance().info("выход");
    Logger::instance().shutdown();
    CoUninitialize();
    return code;
}

}  // namespace er
