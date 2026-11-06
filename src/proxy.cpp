// proxy.cpp — forwards all d3d9.dll exports to the real one
#include <Windows.h>

static HMODULE g_RealD3D9 = nullptr;

// load real d3d9 from system32 before anything else runs
struct D3D9ProxyLoader {
    D3D9ProxyLoader() {
        char path[MAX_PATH];
        GetSystemDirectoryA(path, MAX_PATH);
        strcat_s(path, "\\d3d9.dll");
        g_RealD3D9 = LoadLibraryA(path);
    }
} g_loader;

// forward every export
#define PROXY(name) \
    extern "C" __declspec(dllexport) void name() { \
        static auto fn = GetProcAddress(g_RealD3D9, #name); \
        ((void(*)())fn)(); \
    }

extern "C" __declspec(dllexport)
void* Direct3DCreate9(UINT sdk) {
    static auto fn = (void*(*)(UINT))GetProcAddress(g_RealD3D9, "Direct3DCreate9");
    return fn(sdk);
}

extern "C" __declspec(dllexport)
HRESULT Direct3DCreate9Ex(UINT sdk, void** ppD3D) {
    static auto fn = (HRESULT(*)(UINT, void**))GetProcAddress(g_RealD3D9, "Direct3DCreate9Ex");
    return fn(sdk, ppD3D);
}

PROXY(D3DPERF_BeginEvent)
PROXY(D3DPERF_EndEvent)
PROXY(D3DPERF_GetStatus)
PROXY(D3DPERF_QueryRepeatFrame)
PROXY(D3DPERF_SetMarker)
PROXY(D3DPERF_SetOptions)
PROXY(D3DPERF_SetRegion)
PROXY(DebugSetLevel)
PROXY(DebugSetMute)
PROXY(Direct3D9EnableMaximizedWindowedModeShim)