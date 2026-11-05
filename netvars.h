#pragma once
#include "sdk.h"

// All entity offsets we need, populated at Load() time
struct Offsets {
    int m_iHealth        = 0;
    int m_iTeamNum       = 0;
    int m_lifeState      = 0;
    int m_fFlags         = 0;
    int m_vecOrigin      = 0;
    int m_hActiveWeapon  = 0;
};
inline Offsets g_off;

inline void ScanRecvTable(RecvTable* tbl, int base,
                           std::unordered_map<std::string, int>& map) {
    if (!tbl || !tbl->m_pProps) return;
    for (int i = 0; i < tbl->m_nProps; ++i) {
        RecvProp& p = tbl->m_pProps[i];
        if (!p.m_pVarName) continue;
        char key[256];
        snprintf(key, sizeof(key), "%s::%s",
                 tbl->m_pNetTableName ? tbl->m_pNetTableName : "?",
                 p.m_pVarName);
        int off = base + p.m_Offset;
        if (!map.count(key)) map[key] = off;
        if (p.m_pDataTable) ScanRecvTable(p.m_pDataTable, off, map);
    }
}

inline void InitNetvars(IBaseClientDLL* cl) {
    std::unordered_map<std::string, int> map;
    for (auto* cc = cl->GetAllClasses(); cc; cc = cc->m_pNext)
        ScanRecvTable(cc->m_pRecvTable, 0, map);

    auto get = [&](std::initializer_list<const char*> keys) {
        for (auto* k : keys) {
            auto it = map.find(k);
            if (it != map.end() && it->second > 0) return it->second;
        }
        return 0;
    };

    g_off.m_iHealth = get({
        "DT_BasePlayer::m_iHealth",
        "DT_LocalPlayerExclusive::m_iHealth"
    });
    g_off.m_iTeamNum = get({
        "DT_BaseEntity::m_iTeamNum",
        "DT_CSPlayer::m_iTeamNum"
    });
    g_off.m_lifeState = get({
        "DT_BasePlayer::m_lifeState"
    });
    // m_fFlags: sent only to local player in OB — bhop still works (we only need local)
    g_off.m_fFlags = get({
        "DT_LocalPlayerExclusive::m_fFlags",
        "DT_BasePlayer::m_fFlags"
    });
    g_off.m_vecOrigin = get({
        "DT_BaseEntity::m_vecOrigin[0]",
        "DT_BaseEntity::m_vecOrigin"
    });
    g_off.m_hActiveWeapon = get({
        "DT_BaseCombatCharacter::m_hActiveWeapon"
    });
}