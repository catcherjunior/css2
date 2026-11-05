// hooks.cpp — D3D9 EndScene/Reset + WndProc + CreateMove VMT hooks
#include "hooks.h"
#include "config.h"
#include "features.h"
#include "menu.h"
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// ── swap one vtable slot, return original ─────────────────────
static void* VtSwap(void** vt, int idx, void* fn) {
    DWORD old;
    VirtualProtect(&vt[idx], sizeof(void*), PAGE_READWRITE, &old);
    void* orig = vt[idx];
    vt[idx] = fn;
    VirtualProtect(&vt[idx], sizeof(void*), old, &old);
    return orig;
}

// ── EndScene ──────────────────────────────────────────────────
static HRESULT __stdcall hEndScene(IDirect3DDevice9* dev) {
    if (!Hooks::g_ImgInit) {
        Hooks::g_Device = dev;
        Hooks::g_Hwnd   = FindWindowA("Valve001", nullptr);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        // small style pass for a cleaner look
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding = s.FrameRounding = s.GrabRounding = s.TabRounding = 5.f;
        s.WindowBorderSize = 1.f;
        s.Colors[ImGuiCol_WindowBg]       = {0.08f, 0.08f, 0.09f, 0.94f};
        s.Colors[ImGuiCol_TitleBgActive]  = {0.15f, 0.15f, 0.17f, 1.00f};
        s.Colors[ImGuiCol_Header]         = {0.20f, 0.20f, 0.22f, 1.00f};
        s.Colors[ImGuiCol_SliderGrab]     = {0.40f, 0.60f, 1.00f, 1.00f};
        s.Colors[ImGuiCol_CheckMark]      = {0.40f, 0.80f, 0.40f, 1.00f};

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags  |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename   = nullptr;

        ImGui_ImplWin32_Init(Hooks::g_Hwnd);
        ImGui_ImplDX9_Init(dev);
        Hooks::g_ImgInit = true;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_cfg.esp_enabled && iface::engine && iface::engine->IsInGame())
        Features::DrawESP();

    if (g_cfg.menu_open)
        Menu::Draw();

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    return Hooks::oEndScene(dev);
}

// ── Reset (handle device lost) ────────────────────────────────
static HRESULT __stdcall hReset(IDirect3DDevice9* dev, D3DPRESENT_PARAMETERS* pp) {
    if (Hooks::g_ImgInit) ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = Hooks::oReset(dev, pp);
    if (Hooks::g_ImgInit && SUCCEEDED(hr)) ImGui_ImplDX9_CreateDeviceObjects();
    return hr;
}

// ── WndProc ───────────────────────────────────────────────────
static LRESULT CALLBACK hWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        g_cfg.menu_open = !g_cfg.menu_open;
        ShowCursor(g_cfg.menu_open);
        if (g_cfg.menu_open) ClipCursor(nullptr);
    }
    if (g_cfg.menu_open) {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        if (ImGui::GetIO().WantCaptureMouse) return 0;
    }
    return CallWindowProcA(Hooks::oWndProc, hWnd, msg, wParam, lParam);
}

// ── CreateMove ────────────────────────────────────────────────
// __fastcall: ecx=this, edx=ignored (thiscall shim), then args
static bool __fastcall hCreateMove(void* ecx, void*, float dt, CUserCmd* cmd) {
    bool r = Hooks::oCreateMove(ecx, dt, cmd);
    if (cmd) Features::Bhop(cmd);
    return r;
}

// ── Hooks::InitD3D9 ───────────────────────────────────────────
bool Hooks::InitD3D9() {
    // temp window so we can spin up a device and read its vtable
    HWND tmp = CreateWindowExA(0, "STATIC", "", WS_POPUP | WS_MINIMIZE,
                               0, 0, 2, 2, nullptr, nullptr,
                               GetModuleHandleA(nullptr), nullptr);
    if (!tmp) return false;

    HMODULE d3d9 = LoadLibraryA("d3d9.dll");
    if (!d3d9) { DestroyWindow(tmp); return false; }

    auto* D3DCreate = (IDirect3D9*(*)(UINT))GetProcAddress(d3d9, "Direct3DCreate9");
    IDirect3D9* d3d = D3DCreate(D3D_SDK_VERSION);
    if (!d3d) { DestroyWindow(tmp); return false; }

    D3DPRESENT_PARAMETERS pp{};
    pp.Windowed   = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = tmp;

    IDirect3DDevice9* dev = nullptr;
    HRESULT hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tmp,
                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
    d3d->Release();
    if (FAILED(hr) || !dev) { DestroyWindow(tmp); return false; }

    // All D3D9 device instances share the same vtable — patching it here
    // affects the real CS:S device too
    g_D3DVtable = *(void***)dev;
    oEndScene   = (EndScene_t)VtSwap(g_D3DVtable, 42, (void*)hEndScene);
    oReset      = (Reset_t)  VtSwap(g_D3DVtable, 16, (void*)hReset);
    dev->Release();
    DestroyWindow(tmp);

    // hook WndProc while we're here
    HWND game = FindWindowA("Valve001", nullptr);
    if (game)
        oWndProc = (WndProc_t)SetWindowLongPtrA(game, GWLP_WNDPROC, (LONG_PTR)hWndProc);

    return true;
}

// ── Hooks::InitCreateMove ─────────────────────────────────────
bool Hooks::InitCreateMove() {
    if (!iface::client_mode) return false;
    // IClientMode::CreateMove = vtable index 24 for CS:S OB
    // VERIFY: if bhop doesn't fire, check vtable[24] in IDA on client.dll
    auto* vt = *(void***)iface::client_mode;
    oCreateMove = (CreateMove_t)VtSwap(vt, 24, (void*)hCreateMove);
    return oCreateMove != nullptr;
}

// ── Hooks::Shutdown ───────────────────────────────────────────
void Hooks::Shutdown() {
    if (g_D3DVtable) {
        if (oEndScene)  VtSwap(g_D3DVtable, 42, (void*)oEndScene);
        if (oReset)     VtSwap(g_D3DVtable, 16, (void*)oReset);
    }
    if (oCreateMove && iface::client_mode)
        VtSwap(*(void***)iface::client_mode, 24, (void*)oCreateMove);
    if (g_Hwnd && oWndProc)
        SetWindowLongPtrA(g_Hwnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
    if (g_ImgInit) {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_ImgInit = false;
    }
}