#pragma once
struct Config {
    bool bhop_enabled = true;
    int  bhop_chance  = 65;   // %: how often jump connects mid-air (1–100)

    bool esp_enabled  = true;
    bool esp_boxes    = true;
    bool esp_health   = true;
    bool esp_weapons  = true;

    bool menu_open    = false;
};
inline Config g_cfg;