#include "features.h"
#include "config.h"
#include "netvars.h"
#include <imgui.h>
#include <cstdio>
#include <cstring>

// ── World-to-screen ───────────────────────────────────────────
bool Features::W2S(const Vector& w, float& sx, float& sy) {
    if (!iface::engine) return false;
    int sw, sh;
    iface::engine->GetScreenSize(sw, sh);
    const VMatrix& m = iface::engine->WorldToScreenMatrix();

    float clip_w = m.m[3][0]*w.x + m.m[3][1]*w.y + m.m[3][2]*w.z + m.m[3][3];
    if (clip_w < 0.001f) return false;

    float nx = (m.m[0][0]*w.x + m.m[0][1]*w.y + m.m[0][2]*w.z + m.m[0][3]) / clip_w;
    float ny = (m.m[1][0]*w.x + m.m[1][1]*w.y + m.m[1][2]*w.z + m.m[1][3]) / clip_w;

    sx = sw * 0.5f + nx * (sw * 0.5f);
    sy = sh * 0.5f - ny * (sh * 0.5f);
    return true;
}

// ── Bhop ──────────────────────────────────────────────────────
void Features::Bhop(CUserCmd* cmd) {
    if (!g_cfg.bhop_enabled)        return;
    if (!(cmd->buttons & IN_JUMP))  return;
    if (!iface::engine || !iface::entlist) return;
    if (g_off.m_fFlags == 0)        return;

    auto* local = iface::entlist->GetClientEntity(iface::engine->GetLocalPlayer());
    if (!local) return;

    int flags = local->At<int>(g_off.m_fFlags);
    if (!(flags & FL_ONGROUND)) {
        // mid-air: apply % regulator so it looks human
        static std::mt19937 rng{ std::random_device{}() };
        static std::uniform_int_distribution<int> dist(1, 100);
        if (dist(rng) > g_cfg.bhop_chance)
            cmd->buttons &= ~IN_JUMP;
    }
}

// ── ESP ───────────────────────────────────────────────────────
void Features::DrawESP() {
    if (!iface::entlist || !iface::engine) return;
    // bail if critical offsets missing
    if (!g_off.m_iHealth || !g_off.m_lifeState || !g_off.m_vecOrigin) return;

    ImDrawList* dl  = ImGui::GetBackgroundDrawList();
    int local_idx   = iface::engine->GetLocalPlayer();
    auto* local_ent = iface::entlist->GetClientEntity(local_idx);
    int   local_team = (local_ent && g_off.m_iTeamNum)
                       ? local_ent->At<int>(g_off.m_iTeamNum) : 0;
    int max_ent = iface::entlist->GetHighestEntityIndex();

    for (int i = 1; i <= max_ent; ++i) {
        if (i == local_idx) continue;
        auto* ent = iface::entlist->GetClientEntity(i);
        if (!ent) continue;

        ClientClass* cc = ent->GetClientClass();
        if (!cc || !cc->m_pNetworkName) continue;
        if (!strstr(cc->m_pNetworkName, "CSPlayer")) continue; // players only

        // alive check
        if (ent->At<uint8_t>(g_off.m_lifeState) != LIFE_ALIVE) continue;
        int hp = ent->At<int>(g_off.m_iHealth);
        if (hp <= 0) continue;

        // positions
        Vector feet = ent->At<Vector>(g_off.m_vecOrigin);
        Vector head = { feet.x, feet.y, feet.z + 72.f }; // standing height approx

        float fx, fy, hx, hy;
        if (!W2S(feet, fx, fy) || !W2S(head, hx, hy)) continue;

        float height = fy - hy;
        if (height < 2.f) continue;
        float width = height * 0.4f;
        float left  = hx - width * 0.5f, right = hx + width * 0.5f;
        float top   = hy, bottom = fy;

        int team = g_off.m_iTeamNum ? ent->At<int>(g_off.m_iTeamNum) : 0;
        bool enemy = (team != local_team);
        ImU32 col = enemy ? IM_COL32(220, 50,  50,  230)
                          : IM_COL32(50,  210, 50,  230);

        // ── box ──────────────────────────────────────────────
        if (g_cfg.esp_boxes) {
            dl->AddRect({left-1,top-1},{right+1,bottom+1}, IM_COL32(0,0,0,160)); // shadow
            dl->AddRect({left,  top  },{right,  bottom  }, col);
        }

        // ── health bar (left of box) ──────────────────────────
        if (g_cfg.esp_health) {
            float ratio = (float)hp / 100.f;
            if (ratio > 1.f) ratio = 1.f;
            float bx  = left - 5.f;
            float bar_top = bottom - height * ratio;

            dl->AddRectFilled({bx-1,top},{bx+3,bottom},  IM_COL32(0,0,0,160));
            ImU32 hcol = IM_COL32(
                (uint8_t)((1.f-ratio)*255.f),
                (uint8_t)(ratio*210.f),
                0, 230
            );
            dl->AddRectFilled({bx-1,bar_top},{bx+3,bottom}, hcol);

            // hp number below box
            char buf[8]; snprintf(buf, sizeof(buf), "%d", hp);
            dl->AddText({left, bottom+2.f}, IM_COL32(220,220,220,220), buf);
        }

        // ── weapon name ───────────────────────────────────────
        if (g_cfg.esp_weapons && g_off.m_hActiveWeapon) {
            int handle = ent->At<int>(g_off.m_hActiveWeapon);
            int widx   = handle & 0xFFF; // EHandle: low 12 bits = entity index
            if (widx > 0 && widx <= max_ent) {
                auto* w = iface::entlist->GetClientEntity(widx);
                if (w) {
                    ClientClass* wcc = w->GetClientClass();
                    if (wcc && wcc->m_pNetworkName) {
                        const char* wn = wcc->m_pNetworkName;
                        if (!strncmp(wn, "CWeapon", 7))  wn += 7;
                        dl->AddText({left, bottom+13.f}, IM_COL32(190,190,190,210), wn);
                    }
                }
            }
        }
    }
}