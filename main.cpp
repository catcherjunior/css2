// main.cpp — injectable debug DLL for CS:S clientmod
// inject with Extreme Injector -> CMLauncher.exe (486MB) -> Manual Map
#include "sdk.h"
#include "config.h"
#include "netvars.h"
#include "hooks.h"
#include <cstdio>

static DWORD WINAPI MainThread(LPVOID) {
    // open console immediately so we see output
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleTitleA("clientmod debug");

    printf("[+] DLL injected\n");

    // wait for shaderapidx9
    printf("[*] waiting for shaderapidx9.dll...\n");
    while (!GetModuleHandleA("shaderapidx9.dll")) Sleep(100);
    printf("[+] shaderapidx9.dll found\n");
    Sleep(500);

    // wait for client.dll
    printf("[*] waiting for client.dll...\n");
    while (!GetModuleHandleA("client.dll")) Sleep(100);
    printf("[+] client.dll found\n");

    // init interfaces
    HMODULE engine_dll = GetModuleHandleA("engine.dll");
    if (!engine_dll) { printf("[-] engine.dll not found\n"); return 0; }
    printf("[+] engine.dll found\n");

    auto engineFac = (CreateInterfaceFn)GetProcAddress(engine_dll, "CreateInterface");
    if (!engineFac) { printf("[-] CreateInterface not found in engine.dll\n"); return 0; }

    iface::engine = (IVEngineClient*)engineFac("VEngineClient013", nullptr);
    if (!iface::engine) { printf("[-] VEngineClient013 failed\n"); return 0; }
    printf("[+] engine interface OK\n");

    HMODULE cdll = GetModuleHandleA("client.dll");
    auto clientFac = (CreateInterfaceFn)GetProcAddress(cdll, "CreateInterface");

    iface::client  = (IBaseClientDLL*)    clientFac("VClient017",           nullptr);
    iface::entlist = (IClientEntityList*) clientFac("VClientEntityList003", nullptr);

    if (!iface::client)  { printf("[-] VClient017 failed\n");           return 0; }
    if (!iface::entlist) { printf("[-] VClientEntityList003 failed\n"); return 0; }
    printf("[+] client interfaces OK\n");

    InitNetvars(iface::client);
    printf("[+] netvars OK\n");

    if (Hooks::InitD3D9()) printf("[+] D3D9 hooked\n");
    else                   printf("[-] D3D9 hook FAILED\n");

    printf("[+] ready — get in a server\n");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    if (reason == DLL_PROCESS_DETACH && Hooks::oEndScene)
        Hooks::Shutdown();
    return TRUE;
}
