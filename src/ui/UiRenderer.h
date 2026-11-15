#pragma once

#include "features/FeatureManager.h"
#include "game/GameAttach.h"

#include <Windows.h>

#include <d3d11.h>

struct ImFont;

namespace er {

class UiRenderer {
public:
    UiRenderer(GameAttach& game, FeatureManager& features);
    ~UiRenderer();

    bool create(int x, int y, int w, int h);
    int run();
    void requestClose();

    [[nodiscard]] HWND hwnd() const { return hwnd_; }

private:
    bool createDevice();
    void destroyDevice();
    void createRenderTarget();
    void destroyRenderTarget();
    bool loadFonts();
    void applyTheme();
    void draw();
    void drawHeader(const AttachSnapshot& snap);
    void drawFooter(const AttachSnapshot& snap);
    void drawFeatureList();
    void drawFeatureRow(const struct FeatureInfo& info);
    void drawBrandMark(float x, float y, float size);

    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    GameAttach& game_;
    FeatureManager& features_;
    HWND hwnd_ = nullptr;
    WNDCLASSEXW wc_{};
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swap_ = nullptr;
    ID3D11RenderTargetView* rtv_ = nullptr;
    ImFont* fontBody_ = nullptr;
    ImFont* fontTitle_ = nullptr;
    ImFont* fontMono_ = nullptr;
    bool running_ = false;
    UINT resizeW_ = 0;
    UINT resizeH_ = 0;
    int uiLang_ = 0;
};

}  // namespace er
