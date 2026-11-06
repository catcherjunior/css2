// dllmain.cpp — d3d9 proxy boot | CS:S OB v34 | 32-bit DLL | MSVC C++20
// Rename compiled output to d3d9.dll
// Drop it next to CMLauncher.exe
// No injector needed — Windows loads it before the real d3d9 (exe-folder takes priority)

#include "sdk.h"
#include "config.h"
#include "netvars.h"
#include "hooks.h"

// ── real d3d9 ─────────────────────────────────────────────────
static HMODULE g_RealD3D9 = nullptr;

static void LoadRealD3D9() {
    char path[MAX_PATH];
    GetSystemDirectoryA(path, MAX_PATH); // resolves to SysWOW64 on 32-bit process
    strcat_s(path, "\\d3d9.dll");
    g_RealD3D9 = LoadLibraryA(path);
}

// ── export forwards ───────────────────────────────────────────
extern "C" {
    __declspec(dllexport) void* __stdcall Direct3DCreate9(UINT sdk) {
        static auto fn = (void*(__stdcall*)(UINT))
            GetProcAddress(g_RealD3D9, "Direct3DCreate9");
        return fn(sdk);
    }
    __declspec(dllexport) HRESULT __stdcall Direct3DCreate9Ex(UINT sdk, void** pp) {
        static auto fn = (HRESULT(__stdcall*)(UINT, void**))
            GetProcAddress(g_RealD3D9, "Direct3DCreate9Ex");
        return fn ? fn(sdk, pp) : E_NOTIMPL;
    }
    __declspec(dllexport) int   __stdcall D3DPERF_BeginEvent(DWORD c, LPCWSTR n)        { return 0; }
    __declspec(dllexport) int   __stdcall D3DPERF_EndEvent()                             { return 0; }
    __declspec(dllexport) DWORD __stdcall D3DPERF_GetStatus()                            { return 0; }
    __declspec(dllexport) BOOL  __stdcall D3DPERF_QueryRepeatFrame()                     { return FALSE; }
    __declspec(dllexport) void  __stdcall D3DPERF_SetMarker(DWORD c, LPCWSTR n)         {}
    __declspec(dllexport) void  __stdcall D3DPERF_SetOptions(DWORD o)                   {}
    __declspec(dllexport) void  __stdcall D3DPERF_SetRegion(DWORD c, LPCWSTR n)         {}
    __declspec(dllexport) void  __stdcall DebugSetLevel(DWORD l)                        {}
    __declspec(dllexport) void  __stdcall DebugSetMute(BOOL m)                          {}
    __declspec(dllexport) void  __stdcall Direct3D9EnableMaximizedWindowedModeShim(BOOL e) {}
}

// ── clientmode scan ───────────────────────────────────────────
static void* FindClientMode() {
    HMODULE h = GetModuleHandleA("client.dll");
    if (!h) return nullptr;
    MODULEINFO mi{};
    GetModuleInformation(GetCurrentProcess(), h, &mi, sizeof(mi));
    auto* base = (uint8_t*)h;
    auto* end  = base + mi.SizeOfImage;
    for (auto* p = base; p < end - 9; ++p) {
        if (p[0] != 0x8B || p[1] != 0x0D) continue;
        if (p[6] != 0xFF || p[7] != 0x51) continue;
        uintptr_t ptr_addr = *(uintptr_t*)(p + 2);
        if (ptr_addr < (uintptr_t)h ||
            ptr_addr >= (uintptr_t)h + mi.SizeOfImage) continue;
        void* mode = *(void**)ptr_addr;
        if (!mode) continue;
        if (IsBadReadPtr(mode, sizeof(void*))) continue;
        void* vt = *(void**)mode;
        if (IsBadReadPtr(vt, sizeof(void*))) continue;
        return mode;
    }
    return nullptr;
}

// ── cheat thread ──────────────────────────────────────────────
static DWORD WINAPI CheatThread(LPVOID) {
    // wait for engine + client dlls to be mapped
    while (!GetModuleHandleA("engine.dll") ||
           !GetModuleHandleA("client.dll"))
        Sleep(200);
    Sleep(400);

    // grab engine interfaces
    HMODULE eng    = GetModuleHandleA("engine.dll");
    auto    engFac = (CreateInterfaceFn)GetProcAddress(eng, "CreateInterface");
    if (!engFac) return 1;

    iface::engine = (IVEngineClient*)engFac("VEngineClient013", nullptr);
    if (!iface::engine) return 1;

    // grab client interfaces
    HMODULE cdll      = GetModuleHandleA("client.dll");
    auto    clientFac = (CreateInterfaceFn)GetProcAddress(cdll, "CreateInterface");
    if (!clientFac) return 1;

    iface::client  = (IBaseClientDLL*)    clientFac("VClient017",           nullptr);
    iface::entlist = (IClientEntityList*) clientFac("VClientEntityList003", nullptr);
    if (!iface::client || !iface::entlist) return 1;

    // scan netvars now that client.dll is ready
    InitNetvars(iface::client);

    // wait for d3d to be alive before hooking
    while (!GetModuleHandleA("shaderapidx9.dll")) Sleep(100);
    Sleep(400);
    Hooks::InitD3D9();

    // poll for clientmode — only exists after a map starts loading
    while (!iface::client_mode) {
        iface::client_mode = FindClientMode();
        if (iface::client_mode)
            Hooks::InitCreateMove();
        else
            Sleep(500);
    }

    return 0;
}

// ── DllMain ───────────────────────────────────────────────────
BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        LoadRealD3D9();
        if (!g_RealD3D9) return FALSE; // real d3d9 missing, abort
        CreateThread(nullptr, 0, CheatThread, nullptr, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        Hooks::Shutdown();
        if (g_RealD3D9) {
            FreeLibrary(g_RealD3D9);
            g_RealD3D9 = nullptr;
        }
    }
    return TRUE;
}