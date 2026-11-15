#include "app/Application.h"

#include <Windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    er::Application app;
    return app.run();
}
