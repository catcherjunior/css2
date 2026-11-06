// hooks.cpp — Present hook via minhook + shaderapidx9 device pointer
#include "hooks.h"
#include "config.h"
#include "features.h"
#include "menu.h"
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <MinHook.h>

extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace Hooks {
    IDirect3DDevice9*   g_Device   = nullptr;
    HWND                g_Hwnd     = nullptr;
    bool                g_ImgInit  = false;
    IDirect3DStateBlock9* g_StateBlock = nullptr;

    using PresentFn = HRESULT(D3DAPI*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
    PresentFn oPresent = nullptr;

    using CreateMoveFn = bool(__thiscall*)(void*, float, CUserCmd*);
    CreateMoveFn oCreateMove = nullptr;

    void* iface::client_mode = nullptr;
}

static HRESULT D3DAPI hPresent(IDirect3DDevice9* device,
    const RECT* src, const RECT* dst, HWND hwnd_override, const RGNDATA* dirty)
{
    if (!Hooks::g_ImgInit) {
        Hooks::g_Device = device;

        D3DDEVICE_CREATION_PARAMETERS params{};
        device->GetCreationParameters(&params);
        Hooks::g_Hwnd = params.hFocusWindow;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding = s.FrameRounding = s.GrabRounding = s.TabRounding = 5.f;
        s.WindowBorderSize = 1.f;
        s.Colors[ImGuiCol_WindowBg]      = {0.08f, 0.08f, 0.09f, 0.94f};
        s.Colors[ImGuiCol_TitleBgActive] = {0.15f, 0.15f, 0.17f, 1.00f};
        s.Colors[ImGuiCol_Header]        = {0.20f, 0.20f, 0.22f, 1.00f};
        s.Colors[ImGuiCol_SliderGrab]    = {0.40f, 0.60f, 1.00f, 1.00f};
        s.Colors[ImGuiCol_CheckMark]     = {0.40f, 0.80f, 0.40f, 1.00f};

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename  = nullptr;

        ImGui_ImplWin32_Init(Hooks::g_Hwnd);
        ImGui_ImplDX9_Init(device);

        device->CreateStateBlock(D3DSBT_ALL, &Hooks::g_StateBlock);

        Hooks::g_ImgInit = true;
    }

    if (GetAsyncKeyState(VK_INSERT) & 1) {
        g_cfg.menu_open = !g_cfg.menu_open;
        ShowCursor(g_cfg.menu_open);
        if (g_cfg.menu_open) ClipCursor(nullptr);
    }

    Hooks::g_StateBlock->Capture();

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_cfg.esp_enabled && iface::engine && iface::engine->IsInGame())
        Features::DrawESP();

    if (g_cfg.menu_open)
        Menu::Draw();

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    Hooks::g_StateBlock->Apply();

    return Hooks::oPresent(device, src, dst, hwnd_override, dirty);
}

static bool __fastcall hCreateMove(void* ecx, void*, float dt, CUserCmd* cmd) {
    bool r = Hooks::oCreateMove(ecx, dt, cmd);
    if (cmd) Features::Bhop(cmd);
    return r;
}

bool Hooks::InitD3D9() {
    // wait for shaderapidx9
    HMODULE shader = nullptr;
    while (!shader) {
        shader = GetModuleHandleA("shaderapidx9.dll");
        Sleep(100);
    }
    Sleep(500);

    // get encrypted device pointer — same offset as deathcore's cheat
    // VERIFY: if this fails, scan shaderapidx9.dll for the device ptr in IDA
    const auto device_encrypted = *(uintptr_t*)((uintptr_t)shader + 0x002360D8);
    if (!device_encrypted) return false;

    g_Device = (IDirect3DDevice9*)(device_encrypted ^ 0xAA05351F);
    if (!g_Device) return false;

    auto vmt = *(void***)g_Device;
    if (!vmt) return false;

    // hook Present = vtable[17]
    if (MH_CreateHook(vmt[17], &hPresent, (void**)&oPresent) != MH_OK) return false;
    if (MH_EnableHook(vmt[17]) != MH_OK) return false;

    return true;
}

bool Hooks::InitCreateMove() {
    if (!iface::client_mode) return false;
    auto* vt = *(void***)iface::client_mode;

    if (MH_CreateHook(vt[24], &hCreateMove, (void**)&oCreateMove) != MH_OK) return false;
    if (MH_EnableHook(vt[24]) != MH_OK) return false;

    return oCreateMove != nullptr;
}

void Hooks::Shutdown() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    if (g_Hwnd && oWndProc)
        SetWindowLongPtrA(g_Hwnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);

    if (g_ImgInit) {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_ImgInit = false;
    }

    if (g_StateBlock) {
        g_StateBlock->Release();
        g_StateBlock = nullptr;
    }
}
