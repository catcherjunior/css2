// main.cpp — CS:S v34 internal cheat via Source VDF client plugin
// Build : 32-bit DLL, Windows, MSVC/C++20
// Deploy: cstrike/addons/clientmod.dll + cstrike/addons/clientmod.vdf
#include "sdk.h"
#include "config.h"
#include "netvars.h"
#include "hooks.h"

// ── find g_pClientMode via scan of client.dll ─────────────────
// sig: mov ecx,[g_pClientMode] ; call [ecx+N]
// VERIFY: if bhop never inits, open client.dll in IDA,
//         search for "ClientModeCSNormal", find the global ref
static void* FindClientMode() {
    HMODULE h = GetModuleHandleA("client.dll");
    if (!h) return nullptr;
    MODULEINFO mi{};
    GetModuleInformation(GetCurrentProcess(), h, &mi, sizeof(mi));
    auto* base = (uint8_t*)h;
    auto* end  = base + mi.SizeOfImage;

    for (auto* p = base; p < end - 9; ++p) {
        // 8B 0D [4-byte addr] FF 51 [byte]
        if (p[0] != 0x8B || p[1] != 0x0D) continue;
        if (p[6] != 0xFF || p[7] != 0x51) continue;

        // The 4 bytes at p+2 are an absolute address of the global pointer
        uintptr_t ptr_addr = *(uintptr_t*)(p + 2);
        // must be inside client.dll's data
        if (ptr_addr < (uintptr_t)h ||
            ptr_addr >= (uintptr_t)h + mi.SizeOfImage) continue;

        void* mode = *(void**)ptr_addr;
        if (!mode) continue;
        // sanity: mode must have a readable vtable
        if (IsBadReadPtr(mode, sizeof(void*))) continue;
        void* vt = *(void**)mode;
        if (IsBadReadPtr(vt, sizeof(void*))) continue;
        return mode;
    }
    return nullptr;
}

// ── plugin class ──────────────────────────────────────────────
class CClientModPlugin : public IServerPluginCallbacks {
public:
    bool Load(CreateInterfaceFn factory, CreateInterfaceFn) override {
        iface::engine = (IVEngineClient*)factory("VEngineClient013", nullptr);
        if (!iface::engine) return false;

        HMODULE cdll = GetModuleHandleA("client.dll");
        if (!cdll) return false;
        auto clientFac = (CreateInterfaceFn)GetProcAddress(cdll, "CreateInterface");
        if (!clientFac) return false;

        iface::client  = (IBaseClientDLL*)    clientFac("VClient017",           nullptr);
        iface::entlist = (IClientEntityList*) clientFac("VClientEntityList003", nullptr);
        if (!iface::client || !iface::entlist) return false;

        InitNetvars(iface::client);

        // D3D9 hook in background — device may not exist yet at Load()
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            while (!GetModuleHandleA("shaderapidx9.dll")) Sleep(100);
            Sleep(500);
            Hooks::InitD3D9();
            return 0;
        }, nullptr, 0, nullptr);

        return true;
    }

    // LevelInit fires after the game loads a map → clientmode is initialized by then
    void LevelInit(char const*) override {
        if (!iface::client_mode) {
            iface::client_mode = FindClientMode();
            if (iface::client_mode) Hooks::InitCreateMove();
        }
    }

    void Unload() override { Hooks::Shutdown(); }

    // stubs — not needed for a client-side cheat
    void Pause()   override {}
    void UnPause() override {}
    const char* GetPluginDescription() override { return "clientmod"; }
    void ServerActivate(edict_t*, int, int)  override {}
    void GameFrame(bool)                     override {}
    void LevelShutdown()                     override {}
    void ClientFullyConnect(edict_t*)        override {}
    void ClientDisconnect(edict_t*)          override {}
    void ClientPutInServer(edict_t*, const char*) override {}
    void SetCommandClient(int)               override {}
    void ClientSettingsChanged(edict_t*)     override {}
    PLUGIN_RESULT ClientConnect(bool*, edict_t*, const char*,
                                const char*, char*, int)   override { return PLUGIN_CONTINUE; }
    PLUGIN_RESULT ClientCommand(edict_t*, const CCommand&) override { return PLUGIN_CONTINUE; }
    PLUGIN_RESULT NetworkIDValidated(const char*, const char*) override { return PLUGIN_CONTINUE; }
    void OnQueryCvarValueFinished(QueryCvarCookie_t, edict_t*,
                                  EQueryCvarValueStatus,
                                  const char*, const char*) override {}
};

static CClientModPlugin g_plugin;

// ── DLL export — engine looks for this by name ────────────────
extern "C" __declspec(dllexport)
void* CreateInterface(const char* name, int* ret) {
    if (!strcmp(name, "ISERVERPLUGINCALLBACKS002")) {
        if (ret) *ret = 0;
        return static_cast<IServerPluginCallbacks*>(&g_plugin);
    }
    if (ret) *ret = 1;
    return nullptr;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_DETACH && Hooks::oEndScene)
        Hooks::Shutdown();
    return TRUE;
}