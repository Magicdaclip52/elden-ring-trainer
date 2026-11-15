#include "ui/UiRenderer.h"

#include "core/Config.h"
#include "core/Logger.h"
#include "core/Utf8.h"
#include "features/FeatureCatalog.h"
#include "features/FeatureManager.h"
#include "features/IFeature.h"
#include "hooks/GameHooks.h"
#include "memory/ProcessMemory.h"

#include "resource.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <dwmapi.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace er {
namespace {

constexpr wchar_t kClassName[] = L"ERTrainerWnd";
constexpr int kDefaultW = 1180;
constexpr int kDefaultH = 760;
constexpr int kMinW = 960;
constexpr int kMinH = 600;
constexpr float kHeaderH = 58.0f;
constexpr float kFooterH = 34.0f;
constexpr float kRowH = 44.0f;

constexpr ImU32 kBg = IM_COL32(12, 14, 16, 255);
constexpr ImU32 kHeader = IM_COL32(16, 19, 22, 255);
constexpr ImU32 kRow = IM_COL32(22, 26, 30, 255);
constexpr ImU32 kRowHover = IM_COL32(30, 36, 40, 255);
constexpr ImU32 kRowOn = IM_COL32(24, 36, 34, 255);
constexpr ImU32 kLine = IM_COL32(44, 52, 56, 255);
constexpr ImU32 kAccent = IM_COL32(122, 158, 138, 255);
constexpr ImU32 kAccentHi = IM_COL32(176, 204, 188, 255);
constexpr ImU32 kAccentDim = IM_COL32(70, 96, 84, 255);
constexpr ImU32 kBone = IM_COL32(228, 223, 214, 255);
constexpr ImU32 kMuted = IM_COL32(138, 144, 140, 255);
constexpr ImU32 kDanger = IM_COL32(201, 112, 112, 255);
constexpr ImU32 kWarn = IM_COL32(196, 154, 90, 255);
constexpr ImU32 kChip = IM_COL32(28, 36, 38, 255);

ImVec4 v4(ImU32 c) {
    return ImVec4(((c >> 0) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, ((c >> 16) & 255) / 255.0f,
                  ((c >> 24) & 255) / 255.0f);
}

bool toggleSwitch(const char* id, bool on, bool disabled) {
    ImGui::PushID(id);
    const float w = 40.0f;
    const float h = 22.0f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##sw", ImVec2(w, h));
    const bool clicked = !disabled && ImGui::IsItemClicked();
    const bool hover = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const     ImU32 track = disabled ? IM_COL32(32, 36, 38, 255)
                           : (on ? kAccent : (hover ? IM_COL32(48, 56, 58, 255) : IM_COL32(38, 44, 46, 255)));
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), track, h * 0.5f);
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), disabled ? kLine : (on ? kAccentHi : kLine), h * 0.5f, 0, 1.0f);
    const float knob = h - 6.0f;
    const float kx = on ? (p.x + w - 3.0f - knob) : (p.x + 3.0f);
    dl->AddCircleFilled(ImVec2(kx + knob * 0.5f, p.y + h * 0.5f), knob * 0.5f,
                        disabled ? IM_COL32(90, 94, 92, 255) : (on ? kBone : IM_COL32(180, 184, 180, 255)), 20);
    ImGui::PopID();
    return clicked;
}

bool pillButton(const char* id, const char* label, bool disabled) {
    ImGui::PushID(id);
    const ImVec2 ts = ImGui::CalcTextSize(label);
    const ImVec2 size(ts.x + 22.0f, 26.0f);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##pb", size);
    const bool hover = ImGui::IsItemHovered();
    const bool down = ImGui::IsItemActive();
    const bool clicked = !disabled && ImGui::IsItemClicked();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 fill = hover ? IM_COL32(40, 56, 50, 255) : IM_COL32(28, 36, 34, 255);
    if (down) {
        fill = kAccentDim;
    }
    if (disabled) {
        fill = IM_COL32(24, 26, 28, 255);
    }
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), fill, 4.0f);
    dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), disabled ? kLine : kAccent, 4.0f, 0, 1.0f);
    const ImU32 tc = disabled ? kMuted : kBone;
    dl->AddText(ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f), tc, label);
    ImGui::PopID();
    return clicked;
}

std::string windowsFontPath(const char* file) {
    wchar_t dir[MAX_PATH]{};
    if (GetWindowsDirectoryW(dir, MAX_PATH) == 0) {
        return std::string("C:\\Windows\\Fonts\\") + file;
    }
    return wideToUtf8(std::wstring(dir) + L"\\Fonts\\") + file;
}

bool fontFileExists(const std::string& path) {
    const std::wstring w = utf8ToWide(path);
    const DWORD attr = GetFileAttributesW(w.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

ImFont* tryFont(ImGuiIO& io, const char* const* names, int count, float size, const ImWchar* ranges) {
    for (int i = 0; i < count; ++i) {
        const std::string path = windowsFontPath(names[i]);
        if (!fontFileExists(path)) {
            continue;
        }
        if (ImFont* f = io.Fonts->AddFontFromFileTTF(path.c_str(), size, nullptr, ranges)) {
            Logger::instance().info(std::string("Шрифт UI: ") + path);
            return f;
        }
    }
    return nullptr;
}

void clampWindowRect(int& x, int& y, int& w, int& h) {
    RECT wa{};
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0)) {
        wa.left = 0;
        wa.top = 0;
        wa.right = GetSystemMetrics(SM_CXSCREEN);
        wa.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    const int availW = std::max(kMinW, static_cast<int>(wa.right - wa.left));
    const int availH = std::max(kMinH, static_cast<int>(wa.bottom - wa.top));
    if (w <= 0) {
        w = kDefaultW;
    }
    if (h <= 0) {
        h = kDefaultH;
    }
    w = std::clamp(w, kMinW, availW);
    h = std::clamp(h, kMinH, availH);
    if (x < wa.left || y < wa.top || x + 80 > wa.right || y + 40 > wa.bottom) {
        x = wa.left + (availW - w) / 2;
        y = wa.top + (availH - h) / 2;
    }
}

bool captionBtn(const char* id, const char* glyph, ImU32 hoverBg) {
    ImGui::PushID(id);
    const ImVec2 sz(32.0f, 24.0f);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##cap", sz);
    const bool hover = ImGui::IsItemHovered();
    const bool click = ImGui::IsItemClicked();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (hover) {
        dl->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), hoverBg, 3.0f);
    }
    const ImVec2 ts = ImGui::CalcTextSize(glyph);
    dl->AddText(ImVec2(p.x + (sz.x - ts.x) * 0.5f, p.y + (sz.y - ts.y) * 0.5f), hover ? kBone : kMuted, glyph);
    ImGui::PopID();
    return click;
}

bool langChip(const char* label, bool selected) {
    const ImVec2 ts = ImGui::CalcTextSize(label);
    const ImVec2 size(ts.x + 16.0f, 24.0f);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(label, size);
    const bool clicked = ImGui::IsItemClicked();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (selected) {
        dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), kAccentDim, 3.0f);
    }
    dl->AddText(ImVec2(p.x + 8.0f, p.y + (size.y - ts.y) * 0.5f), selected ? kBone : kMuted, label);
    return clicked;
}

}  // namespace

UiRenderer::UiRenderer(GameAttach& game, FeatureManager& features)
    : game_(game), features_(features) {}

UiRenderer::~UiRenderer() {
    if (hwnd_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        destroyDevice();
        DestroyWindow(hwnd_);
        UnregisterClassW(kClassName, GetModuleHandleW(nullptr));
        hwnd_ = nullptr;
    }
}

bool UiRenderer::create(int x, int y, int w, int h) {
    ImGui_ImplWin32_EnableDpiAwareness();
    clampWindowRect(x, y, w, h);

    wc_ = {};
    wc_.cbSize = sizeof(wc_);
    wc_.style = CS_CLASSDC;
    wc_.lpfnWndProc = wndProc;
    wc_.hInstance = GetModuleHandleW(nullptr);
    wc_.lpszClassName = kClassName;
    wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc_.hIcon = LoadIconW(wc_.hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc_.hIconSm = static_cast<HICON>(LoadImageW(
        wc_.hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    RegisterClassExW(&wc_);

    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, kClassName, L"ERTrainer",
                            WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX, x, y, w, h, nullptr, nullptr, wc_.hInstance,
                            this);
    if (!hwnd_) {
        Logger::instance().error("CreateWindowEx failed: " + lastWinError());
        return false;
    }
    if (wc_.hIcon) {
        SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(wc_.hIcon));
    }
    if (wc_.hIconSm) {
        SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(wc_.hIconSm));
    }

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd_, 20, &dark, sizeof(dark)); // Win10 1809+; на старых просто no-op

    if (!createDevice()) {
        Logger::instance().error("Direct3D 11 недоступен");
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    applyTheme();
    if (!loadFonts()) {
        Logger::instance().warn("Шрифт с кириллицей не загрузился");
    }

    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX11_Init(device_, context_);

    ShowWindow(hwnd_, SW_SHOWDEFAULT);
    UpdateWindow(hwnd_);
    running_ = true;
    return true;
}

bool UiRenderer::loadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    const char* body[] = {"segoeui.ttf", "tahoma.ttf", "arial.ttf", "calibri.ttf", "micross.ttf", "times.ttf"};
    const char* title[] = {"seguisb.ttf", "segoeuib.ttf", "tahomabd.ttf", "arialbd.ttf", "segoeui.ttf", "tahoma.ttf"};
    const char* mono[] = {"consola.ttf", "cour.ttf", "lucon.ttf", "consolab.ttf"};

    fontBody_ = tryFont(io, body, IM_ARRAYSIZE(body), 15.5f, io.Fonts->GetGlyphRangesCyrillic());
    if (!fontBody_) {
        fontBody_ = io.Fonts->AddFontDefault();
        Logger::instance().warn("Системный шрифт с кириллицей не найден, берём встроенный");
    }
    fontTitle_ = tryFont(io, title, IM_ARRAYSIZE(title), 20.0f, io.Fonts->GetGlyphRangesCyrillic());
    fontMono_ = tryFont(io, mono, IM_ARRAYSIZE(mono), 13.5f, io.Fonts->GetGlyphRangesDefault());
    if (!fontMono_) {
        fontMono_ = fontBody_;
    }
    io.FontDefault = fontBody_;
    return fontBody_ != nullptr;
}

void UiRenderer::applyTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0.0f;
    s.ChildRounding = 6.0f;
    s.FrameRounding = 4.0f;
    s.GrabRounding = 3.0f;
    s.PopupRounding = 4.0f;
    s.ScrollbarRounding = 6.0f;
    s.ScrollbarSize = 10.0f;
    s.WindowPadding = ImVec2(0, 0);
    s.FramePadding = ImVec2(8, 5);
    s.ItemSpacing = ImVec2(8, 6);
    s.ItemInnerSpacing = ImVec2(6, 4);
    s.FrameBorderSize = 0.0f;
    s.ChildBorderSize = 0.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = v4(kBg);
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.12f, 0.13f, 0.98f);
    c[ImGuiCol_Border] = v4(kLine);
    c[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.12f, 0.13f, 1);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.20f, 0.19f, 1);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.24f, 0.22f, 1);
    c[ImGuiCol_Button] = ImVec4(0.16f, 0.22f, 0.20f, 1);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.34f, 0.30f, 1);
    c[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.42f, 0.36f, 1);
    c[ImGuiCol_Header] = ImVec4(0.16f, 0.22f, 0.20f, 1);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.30f, 0.27f, 1);
    c[ImGuiCol_Text] = v4(kBone);
    c[ImGuiCol_TextDisabled] = v4(kMuted);
    c[ImGuiCol_Separator] = v4(kLine);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.07f, 0.08f, 1);
    c[ImGuiCol_ScrollbarGrab] = v4(kAccentDim);
    c[ImGuiCol_ScrollbarGrabHovered] = v4(kAccent);
    c[ImGuiCol_CheckMark] = v4(kAccentHi);
    c[ImGuiCol_SliderGrab] = v4(kAccent);
    c[ImGuiCol_SliderGrabActive] = v4(kAccentHi);
    c[ImGuiCol_NavHighlight] = v4(kAccent);
}

bool UiRenderer::createDevice() {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd_;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
    const D3D_DRIVER_TYPE drivers[] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_REFERENCE};
    const DXGI_SWAP_EFFECT effects[] = {DXGI_SWAP_EFFECT_DISCARD, DXGI_SWAP_EFFECT_SEQUENTIAL};

    D3D_FEATURE_LEVEL got{};
    for (const D3D_DRIVER_TYPE drv : drivers) {
        for (const DXGI_SWAP_EFFECT fx : effects) {
            sd.SwapEffect = fx;
            swap_ = nullptr;
            device_ = nullptr;
            context_ = nullptr;
            if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, drv, nullptr, 0, levels, 3, D3D11_SDK_VERSION, &sd,
                                                        &swap_, &device_, &got, &context_))) {
                Logger::instance().info(std::string("D3D11: driver=") +
                                        (drv == D3D_DRIVER_TYPE_HARDWARE ? "hardware"
                                         : drv == D3D_DRIVER_TYPE_WARP   ? "WARP"
                                                                         : "reference"));
                createRenderTarget();
                return rtv_ != nullptr;
            }
        }
    }
    return false;
}

void UiRenderer::createRenderTarget() {
    ID3D11Texture2D* back = nullptr;
    swap_->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) {
        device_->CreateRenderTargetView(back, nullptr, &rtv_);
        back->Release();
    }
}

void UiRenderer::destroyRenderTarget() {
    if (rtv_) {
        rtv_->Release();
        rtv_ = nullptr;
    }
}

void UiRenderer::destroyDevice() {
    destroyRenderTarget();
    if (swap_) {
        swap_->Release();
        swap_ = nullptr;
    }
    if (context_) {
        context_->Release();
        context_ = nullptr;
    }
    if (device_) {
        device_->Release();
        device_ = nullptr;
    }
}

void UiRenderer::requestClose() {
    running_ = false;
}

void UiRenderer::drawBrandMark(float x, float y, float size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 origin(x, y);
    const ImVec2 c(origin.x + size * 0.5f, origin.y + size * 0.5f);
    const float r = size * 0.42f;
    dl->AddCircle(c, r, kAccent, 40, 1.6f);
    dl->AddCircle(c, r * 0.62f, kAccentDim, 32, 1.2f);
    dl->AddLine(ImVec2(c.x, origin.y + 3.0f), ImVec2(c.x, origin.y + size - 3.0f), kAccentHi, 1.4f);
    dl->AddCircleFilled(c, 2.4f, kAccentHi, 12);
}

void UiRenderer::drawHeader(const AttachSnapshot& snap) {
    ImGui::BeginChild("##hdr", ImVec2(0, kHeaderH), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetWindowPos();
    const ImVec2 p1(p0.x + ImGui::GetWindowWidth(), p0.y + kHeaderH);
    dl->AddRectFilled(p0, p1, kHeader);
    dl->AddLine(ImVec2(p0.x, p1.y - 1.0f), ImVec2(p1.x, p1.y - 1.0f), kLine, 1.0f);

    ImGui::SetCursorPos(ImVec2(16.0f, 11.0f));
    const ImVec2 mark = ImGui::GetCursorScreenPos();
    drawBrandMark(mark.x, mark.y, 36.0f);
    ImGui::Dummy(ImVec2(36.0f, 36.0f));
    ImGui::SameLine(0, 12.0f);

    ImGui::BeginGroup();
    if (fontTitle_) {
        ImGui::PushFont(fontTitle_);
    }
    ImGui::SetCursorPosY(10.0f);
    ImGui::TextColored(v4(kBone), "ERTrainer");
    if (fontTitle_) {
        ImGui::PopFont();
    }
    ImGui::SetCursorPosY(32.0f);
    ImGui::TextColored(v4(kMuted), uiLang_ == 0 ? "Elden Ring  1.17.2" : "Elden Ring  1.17.2");
    ImGui::EndGroup();

    const float right = ImGui::GetWindowWidth() - 8.0f;
    float x = right;

    auto place = [&](float w) {
        x -= w;
        ImGui::SetCursorPos(ImVec2(x, 17.0f));
        return x;
    };

    place(32.0f);
    if (captionBtn("cls", "x", IM_COL32(110, 42, 42, 255))) {
        PostMessageW(hwnd_, WM_CLOSE, 0, 0);
    }
    place(32.0f);
    if (captionBtn("min", "-", IM_COL32(42, 50, 52, 255))) {
        ShowWindow(hwnd_, SW_MINIMIZE);
    }

    place(36.0f + 40.0f);
    if (langChip("RU", uiLang_ == 0)) {
        uiLang_ = 0;
    }
    ImGui::SameLine(0, 4.0f);
    if (langChip("EN", uiLang_ == 1)) {
        uiLang_ = 1;
    }

    const bool attached = snap.attached;
    const char* st = attached
                         ? (snap.eacLoaded ? (uiLang_ == 0 ? "EAC" : "EAC")
                                           : (uiLang_ == 0 ? "в игре" : "in-game"))
                         : (uiLang_ == 0 ? "ожидание" : "waiting");
    const ImVec2 sts = ImGui::CalcTextSize(st);
    const float lampW = 12.0f + sts.x + 18.0f;
    place(lampW + 16.0f);
    {
        const ImVec2 lp = ImGui::GetCursorScreenPos();
        const float pulse = attached && !snap.eacLoaded
                                ? 1.0f
                                : (0.45f + 0.55f * (0.5f + 0.5f * std::sinf(static_cast<float>(ImGui::GetTime()) * 3.0f)));
        ImU32 lamp = snap.eacLoaded ? kDanger : (attached ? kAccent : kWarn);
        lamp = (lamp & 0x00FFFFFF) | (static_cast<ImU32>(pulse * 255.0f) << 24);
        dl->AddCircleFilled(ImVec2(lp.x + 6.0f, lp.y + 12.0f), 4.2f, lamp, 16);
        dl->AddText(ImVec2(lp.x + 16.0f, lp.y + 4.0f), attached ? kBone : kMuted, st);
        ImGui::Dummy(ImVec2(lampW, 24.0f));
    }

    // тащим за шапку, кнопки справа не трогаем
    if (ImGui::IsMouseHoveringRect(p0, ImVec2(p1.x - 210.0f, p1.y)) && ImGui::IsMouseClicked(0)) {
        ReleaseCapture();
        SendMessageW(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }

    ImGui::EndChild();
}

void UiRenderer::drawFooter(const AttachSnapshot& snap) {
    ImGui::BeginChild("##ftr", ImVec2(0, kFooterH), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetWindowPos();
    const ImVec2 p1(p0.x + ImGui::GetWindowWidth(), p0.y + kFooterH);
    dl->AddRectFilled(p0, p1, kHeader);
    dl->AddLine(ImVec2(p0.x, p0.y), ImVec2(p1.x, p0.y), kLine, 1.0f);

    int on = 0;
    int ready = 0;
    for (const auto& f : features_.features()) {
        if (f->status() == FeatureStatus::Enabled) {
            ++on;
        } else if (f->status() == FeatureStatus::Ready) {
            ++ready;
        }
    }

    ImGui::SetCursorPos(ImVec2(16.0f, 8.0f));
    if (snap.attached) {
        ImGui::TextColored(v4(kMuted), "PID %u    %s    %s    %s %d/%d", snap.pid, toHex(snap.base).c_str(),
                           snap.version.displayVersion.c_str(), uiLang_ == 0 ? "вкл" : "on", on, on + ready);
    } else {
        ImGui::TextColored(v4(kMuted), "%s",
                           uiLang_ == 0 ? "Запусти eldenring.exe офлайн, без EAC"
                                        : "Start eldenring.exe offline, without EAC");
    }
    ImGui::EndChild();
}

void UiRenderer::drawFeatureRow(const FeatureInfo& info) {
    IFeature* feat = features_.find(info.id);
    const bool disabled = !feat || feat->status() == FeatureStatus::Unavailable;
    const bool enabled = feat && feat->status() == FeatureStatus::Enabled;
    auto& cfg = Config::instance();
    Hotkey hk = info.defaultHotkey;
    if (const auto it = cfg.hotkeys.find(info.id); it != cfg.hotkeys.end()) {
        hk = it->second;
    }

    ImGui::PushID(info.id.c_str());
    const float w = ImGui::GetContentRegionAvail().x;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool hover = ImGui::IsWindowHovered() &&
                       ImGui::IsMouseHoveringRect(p, ImVec2(p.x + w, p.y + kRowH - 3.0f), false);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 bg = enabled ? kRowOn : (hover ? kRowHover : kRow);
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + kRowH - 3.0f), bg, 5.0f);
    if (enabled) {
        dl->AddRectFilled(p, ImVec2(p.x + 3.0f, p.y + kRowH - 3.0f), kAccent, 2.0f);
    }

    float cx = p.x + 12.0f;
    const float cy = p.y + (kRowH - 3.0f) * 0.5f;

    const std::string hkStr = hk.toString();
    if (fontMono_) {
        ImGui::PushFont(fontMono_);
    }
    const ImVec2 hks = ImGui::CalcTextSize(hkStr.c_str());
    const ImVec2 chip0(cx, cy - 11.0f);
    const ImVec2 chip1(cx + hks.x + 14.0f, cy + 11.0f);
    dl->AddRectFilled(chip0, chip1, kChip, 3.0f);
    dl->AddRect(chip0, chip1, kLine, 3.0f, 0, 1.0f);
    dl->AddText(ImVec2(cx + 7.0f, cy - hks.y * 0.5f), kAccentHi, hkStr.c_str());
    if (fontMono_) {
        ImGui::PopFont();
    }
    cx = chip1.x + 14.0f;

    const char* label = uiLang_ == 0 ? info.nameRu.c_str() : info.nameEn.c_str();
    const ImVec2 ls = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(cx, cy - ls.y * 0.5f), disabled ? kMuted : kBone, label);

    if (hover) {
        std::string tip = std::string(uiLang_ == 0 ? info.nameRu : info.nameEn);
        tip += "\n";
        tip += feat ? std::string(feat->reason()) : (uiLang_ == 0 ? "нет сигнатуры" : "no signature");
        ImGui::SetTooltip("%s", tip.c_str());
    }

    const bool hasInt = info.kind == FeatureKind::ApplyValue || info.defaultInt != 0;
    const bool hasFloat = info.defaultFloat != 0.0f &&
                          (info.kind == FeatureKind::Toggle || info.id.find("multiplier") != std::string::npos ||
                           info.id == "game_speed");
    const bool hasHold = info.kind == FeatureKind::Hold;
    const bool hasToggle = info.kind == FeatureKind::Toggle;

    float controls = 0.0f;
    if (hasToggle) {
        controls += 48.0f;
    }
    if (hasInt) {
        controls += 188.0f;
    }
    if (hasFloat) {
        controls += 118.0f;
    }
    if (hasHold) {
        controls += 72.0f;
    }

    const float controlsX = p.x + w - controls - 12.0f;
    ImGui::SetCursorScreenPos(ImVec2(controlsX, p.y + (kRowH - 26.0f) * 0.5f - 1.0f));

    if (disabled) {
        ImGui::BeginDisabled();
    }

    if (hasInt) {
        int val = info.defaultInt;
        if (const auto it = cfg.featureInt.find(info.id); it != cfg.featureInt.end()) {
            val = it->second;
        }
        ImGui::SetNextItemWidth(108.0f);
        ImGui::InputInt("##v", &val, 0, 0);
        cfg.featureInt[info.id] = val;
        ImGui::SameLine(0, 6.0f);
        if (pillButton("ap", uiLang_ == 0 ? "Задать" : "Set", disabled)) {
            features_.requestApply(info.id);
        }
    } else if (hasHold) {
        if (pillButton("hold", uiLang_ == 0 ? "Жми" : "Hold", disabled)) {
            features_.requestApply(info.id);
        }
    }

    if (hasFloat) {
        float val = info.defaultFloat;
        if (const auto it = cfg.featureFloat.find(info.id); it != cfg.featureFloat.end()) {
            val = it->second;
        }
        ImGui::SetNextItemWidth(88.0f);
        ImGui::InputFloat("##f", &val, 0.1f, 1.0f, "%.1f");
        cfg.featureFloat[info.id] = val;
        if (hasToggle) {
            ImGui::SameLine(0, 8.0f);
        }
    }

    if (hasToggle) {
        if (toggleSwitch("tog", enabled, disabled) && feat) {
            features_.requestEnable(info.id, !enabled);
        }
    }

    if (disabled) {
        ImGui::EndDisabled();
    }

    if (!disabled && hasToggle && feat && hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        ImGui::GetIO().MousePos.x < controlsX) {
        features_.requestEnable(info.id, !enabled);
    }

    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(w, kRowH));
    ImGui::PopID();
}

void UiRenderer::drawFeatureList() {
    ImGui::BeginChild("##feats", ImVec2(0, 0), ImGuiChildFlags_None);

    const auto snap = game_.snapshot();
    if (snap.version.warning) {
        ImGui::SetCursorPos(ImVec2(18.0f, 8.0f));
        ImGui::TextColored(v4(kWarn), "%s", snap.version.warningText.c_str());
    }
    if (snap.eacLoaded) {
        ImGui::SetCursorPosX(18.0f);
        ImGui::TextColored(v4(kDanger),
                           "%s", uiLang_ == 0 ? "EasyAntiCheat загружен. Играй офлайн, без EAC."
                                              : "EasyAntiCheat is loaded. Play offline, without EAC.");
    }

    const FeatureGroup groups[] = {FeatureGroup::Survival, FeatureGroup::Combat, FeatureGroup::World,
                                   FeatureGroup::Items, FeatureGroup::Stats};

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
    for (const FeatureGroup g : groups) {
        ImGui::SetCursorPosX(18.0f);
        const char* title = uiLang_ == 0 ? groupTitleRu(g) : groupTitleEn(g);
        ImGui::TextColored(v4(kAccentHi), "%s", title);
        const ImVec2 lp = ImGui::GetItemRectMax();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddLine(ImVec2(lp.x + 10.0f, lp.y - 8.0f),
                    ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - 18.0f, lp.y - 8.0f), kLine, 1.0f);

        ImGui::SetCursorPosX(14.0f);
        ImGui::BeginGroup();
        for (const auto& f : features_.features()) {
            if (f->info().group != g) {
                continue;
            }
            drawFeatureRow(f->info());
        }
        ImGui::EndGroup();
        ImGui::Dummy(ImVec2(0, 10.0f));
    }

    ImGui::Dummy(ImVec2(0, 12.0f));
    ImGui::EndChild();
}

void UiRenderer::draw() {
    const auto snap = game_.snapshot();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##root", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    drawHeader(snap);

    {
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        const ImVec2 ds = io.DisplaySize;
        fg->AddRect(ImVec2(0.5f, 0.5f), ImVec2(ds.x - 0.5f, ds.y - 0.5f), kLine, 0.0f, 0, 1.0f);
    }

    ImGui::BeginChild("##body", ImVec2(0, -(kFooterH)), ImGuiChildFlags_None);
    drawFeatureList();
    ImGui::EndChild();

    drawFooter(snap);
    ImGui::End();
}

int UiRenderer::run() {
    while (running_) {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                running_ = false;
            }
        }
        if (!running_) {
            break;
        }

        if (resizeW_ && resizeH_) {
            destroyRenderTarget();
            swap_->ResizeBuffers(0, resizeW_, resizeH_, DXGI_FORMAT_UNKNOWN, 0);
            resizeW_ = resizeH_ = 0;
            createRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        draw();
        ImGui::Render();

        const float clear[4] = {12.0f / 255.0f, 14.0f / 255.0f, 16.0f / 255.0f, 1};
        context_->OMSetRenderTargets(1, &rtv_, nullptr);
        context_->ClearRenderTargetView(rtv_, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swap_->Present(1, 0);
    }

    RECT rc{};
    if (hwnd_ && GetWindowRect(hwnd_, &rc)) {
        auto& w = Config::instance().window;
        w.x = rc.left;
        w.y = rc.top;
        w.w = rc.right - rc.left;
        w.h = rc.bottom - rc.top;
        Config::instance().save();
    }
    return 0;
}

LRESULT CALLBACK UiRenderer::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
        return true;
    }
    UiRenderer* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<UiRenderer*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<UiRenderer*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    switch (msg) {
        case WM_NCHITTEST: {
            POINT pt{static_cast<LONG>(static_cast<short>(LOWORD(lParam))),
                     static_cast<LONG>(static_cast<short>(HIWORD(lParam)))};
            RECT wr{};
            GetWindowRect(hwnd, &wr);
            const int b = 6;
            const bool left = pt.x < wr.left + b;
            const bool right = pt.x >= wr.right - b;
            const bool top = pt.y < wr.top + b;
            const bool bottom = pt.y >= wr.bottom - b;
            if (top && left) {
                return HTTOPLEFT;
            }
            if (top && right) {
                return HTTOPRIGHT;
            }
            if (bottom && left) {
                return HTBOTTOMLEFT;
            }
            if (bottom && right) {
                return HTBOTTOMRIGHT;
            }
            if (left) {
                return HTLEFT;
            }
            if (right) {
                return HTRIGHT;
            }
            if (top) {
                return HTTOP;
            }
            if (bottom) {
                return HTBOTTOM;
            }
            return HTCLIENT;
        }
        case WM_GETMINMAXINFO:
            if (lParam) {
                auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
                mmi->ptMinTrackSize.x = kMinW;
                mmi->ptMinTrackSize.y = kMinH;
            }
            return 0;
        case WM_SIZE:
            if (self && wParam != SIZE_MINIMIZED) {
                self->resizeW_ = LOWORD(lParam);
                self->resizeH_ = HIWORD(lParam);
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

}  // namespace er
