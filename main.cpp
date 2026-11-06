// main.cpp — injectable DLL, no VDF needed
#include "sdk.h"
#include "config.h"
#include "netvars.h"
#include "hooks.h"

static DWORD WINAPI MainThread(LPVOID) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleTitleA("clientmod");

    printf("[*] waiting for modules...\n");
    while (!GetModuleHandleA("client.dll") ||
           !GetModuleHandleA("engine.dll")  ||
           !GetModuleHandleA("shaderapidx9.dll")) Sleep(100);
    printf("[+] modules found\n");

    MH_Initialize();

    HMODULE engine_dll = GetModuleHandleA("engine.dll");
    auto engineFac = (CreateInterfaceFn)GetProcAddress(engine_dll, "CreateInterface");
    iface::engine = (IVEngineClient*)engineFac("VEngineClient013", nullptr);
    printf(iface::engine ? "[+] engine OK\n" : "[-] engine FAIL\n");

    HMODULE cdll = GetModuleHandleA("client.dll");
    auto clientFac = (CreateInterfaceFn)GetProcAddress(cdll, "CreateInterface");
    iface::client  = (IBaseClientDLL*)    clientFac("VClient017",           nullptr);
    iface::entlist = (IClientEntityList*) clientFac("VClientEntityList003", nullptr);
    printf(iface::client  ? "[+] client OK\n"  : "[-] client FAIL\n");
    printf(iface::entlist ? "[+] entlist OK\n" : "[-] entlist FAIL\n");

    if (iface::client) InitNetvars(iface::client);

    if (Hooks::InitD3D9()) printf("[+] Present hooked\n");
    else                   printf("[-] Present hook FAILED\n");

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    if (reason == DLL_PROCESS_DETACH)
        Hooks::Shutdown();
    return TRUE;
}
