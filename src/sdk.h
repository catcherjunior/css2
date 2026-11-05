// sdk.h — stripped Source SDK, CS:S OB v34, 32-bit Windows
#pragma once
#include <windows.h>
#include <d3d9.h>
#include <psapi.h>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>

// ── Math ─────────────────────────────────────────────────────
struct Vector {
    float x{}, y{}, z{};
    constexpr Vector() = default;
    constexpr Vector(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
    Vector operator+(const Vector& o) const noexcept { return {x+o.x, y+o.y, z+o.z}; }
};

struct VMatrix { float m[4][4]; };
struct QAngle  { float pitch, yaw, roll; };

// ── Source constants ──────────────────────────────────────────
inline constexpr int FL_ONGROUND = 1 << 0;
inline constexpr int IN_JUMP     = 1 << 1;
inline constexpr int LIFE_ALIVE  = 0;

// ── CUserCmd (CS:S OB layout) ─────────────────────────────────
struct CUserCmd {
    void*  vtable;
    int    command_number;
    int    tick_count;
    QAngle viewangles;
    float  forwardmove;
    float  sidemove;
    float  upmove;
    int    buttons;
};

// ── RecvTable / ClientClass ───────────────────────────────────
struct RecvTable;
struct RecvProp {
    char*       m_pVarName;
    int         m_RecvType;
    int         m_Flags;
    int         m_StringBufferSize;
    bool        m_bInsideArray;
    const void* m_pExtraData;
    RecvProp*   m_pArrayProp;
    void*       m_ArrayLengthProxy;
    void*       m_ProxyFn;
    void*       m_DataTableProxyFn;
    RecvTable*  m_pDataTable;
    int         m_Offset;
    int         m_ElementStride;
    int         m_nElements;
    const char* m_pParentArrayPropName;
};
struct RecvTable {
    RecvProp* m_pProps;
    int       m_nProps;
    void*     m_pDecoder;
    char*     m_pNetTableName;
};
struct ClientClass {
    void*        m_pCreateFn;
    void*        m_pCreateEventFn;
    char*        m_pNetworkName;
    RecvTable*   m_pRecvTable;
    ClientClass* m_pNext;
    int          m_ClassID;
};

// ── vtable call helper ────────────────────────────────────────
template<typename Ret, typename This, typename... Args>
inline Ret vcall(This* th, int idx, Args... args) noexcept {
    using fn_t = Ret(__thiscall*)(This*, Args...);
    return (*(fn_t**)th)[idx](th, args...);
}

// ── IVEngineClient013 ─────────────────────────────────────────
// VERIFY INDICES in IDA on engine.dll if something acts wrong
namespace EngIdx {
    inline constexpr int GetScreenSize       = 5;
    inline constexpr int GetLocalPlayer      = 12;
    inline constexpr int IsInGame            = 26;
    inline constexpr int WorldToScreenMatrix = 37;
}
struct IVEngineClient {
    void GetScreenSize(int& w, int& h) noexcept {
        using fn_t = void(__thiscall*)(IVEngineClient*, int&, int&);
        (*(fn_t**)this)[EngIdx::GetScreenSize](this, w, h);
    }
    int  GetLocalPlayer() noexcept { return vcall<int> (this, EngIdx::GetLocalPlayer); }
    bool IsInGame()       noexcept { return vcall<bool>(this, EngIdx::IsInGame); }
    const VMatrix& WorldToScreenMatrix() noexcept {
        using fn_t = const VMatrix&(__thiscall*)(IVEngineClient*);
        return (*(fn_t**)this)[EngIdx::WorldToScreenMatrix](this);
    }
};

// ── IClientEntityList003 ──────────────────────────────────────
struct IClientEntity;
struct IClientEntityList {
    // index 3 = GetClientEntity, index 6 = GetHighestEntityIndex
    IClientEntity* GetClientEntity(int idx) noexcept {
        return vcall<IClientEntity*>(this, 3, idx);
    }
    int GetHighestEntityIndex() noexcept { return vcall<int>(this, 6); }
};

// ── IBaseClientDLL017 ─────────────────────────────────────────
struct IBaseClientDLL {
    // index 8 = GetAllClasses
    ClientClass* GetAllClasses() noexcept { return vcall<ClientClass*>(this, 8); }
};

// ── IClientEntity ─────────────────────────────────────────────
struct IClientEntity {
    template<typename T>
    T& At(int offset) noexcept { return *(T*)((uint8_t*)this + offset); }

    // IClientNetworkable sub-object sits at offset +8 (3rd vfptr in OB layout)
    // Its vtable[2] = GetClientClass
    // VERIFY offset 8 if GetClientClass crashes (try 4 or 12)
    ClientClass* GetClientClass() noexcept {
        void*  net = (void*)((uint8_t*)this + 8);
        void** vft = *(void***)net;
        return ((ClientClass*(__thiscall*)(void*))vft[2])(net);
    }
};

// ── Interface factory + global pointers ──────────────────────
using CreateInterfaceFn = void*(*)(const char*, int*);
namespace iface {
    inline IVEngineClient*    engine      = nullptr;
    inline IClientEntityList* entlist     = nullptr;
    inline IBaseClientDLL*    client      = nullptr;
    inline void*              client_mode = nullptr;
}

// ── IServerPluginCallbacks002 (minimal, for VDF loading) ─────
struct edict_t;
struct CCommand;
enum  PLUGIN_RESULT { PLUGIN_CONTINUE = 0, PLUGIN_OVERRIDE, PLUGIN_STOP };
using QueryCvarCookie_t     = int;
using EQueryCvarValueStatus = int;

struct IServerPluginCallbacks {
    virtual bool Load(CreateInterfaceFn, CreateInterfaceFn) = 0;
    virtual void Unload() = 0;
    virtual void Pause() = 0;
    virtual void UnPause() = 0;
    virtual const char* GetPluginDescription() = 0;
    virtual void LevelInit(char const*) = 0;
    virtual void ServerActivate(edict_t*, int, int) = 0;
    virtual void GameFrame(bool) = 0;
    virtual void LevelShutdown() = 0;
    virtual void ClientFullyConnect(edict_t*) = 0;
    virtual void ClientDisconnect(edict_t*) = 0;
    virtual void ClientPutInServer(edict_t*, const char*) = 0;
    virtual void SetCommandClient(int) = 0;
    virtual void ClientSettingsChanged(edict_t*) = 0;
    virtual PLUGIN_RESULT ClientConnect(bool*, edict_t*, const char*,
                                        const char*, char*, int) = 0;
    virtual PLUGIN_RESULT ClientCommand(edict_t*, const CCommand&) = 0;
    virtual PLUGIN_RESULT NetworkIDValidated(const char*, const char*) = 0;
    virtual void OnQueryCvarValueFinished(QueryCvarCookie_t, edict_t*,
                                          EQueryCvarValueStatus,
                                          const char*, const char*) = 0;
};