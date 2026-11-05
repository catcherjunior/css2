#include "menu.h"
#include "config.h"
#include <imgui.h>

void Menu::Draw() {
    ImGui::SetNextWindowSize({300, 240}, ImGuiCond_Once);
    ImGui::SetNextWindowPos({40, 40},   ImGuiCond_Once);
    ImGui::Begin("clientmod  |  INSERT = toggle", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    if (ImGui::BeginTabBar("##tabs")) {

        if (ImGui::BeginTabItem("Bhop")) {
            ImGui::Spacing();
            ImGui::Checkbox("Enabled##bh", &g_cfg.bhop_enabled);
            ImGui::Separator();
            ImGui::BeginDisabled(!g_cfg.bhop_enabled);
            ImGui::SliderInt("Chance %", &g_cfg.bhop_chance, 1, 100);
            ImGui::TextDisabled("100 = every frame lands  |  ~65 = human-ish");
            ImGui::EndDisabled();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ESP")) {
            ImGui::Spacing();
            ImGui::Checkbox("Enable ESP",   &g_cfg.esp_enabled);
            ImGui::Separator();
            ImGui::BeginDisabled(!g_cfg.esp_enabled);
            ImGui::Checkbox("Boxes",        &g_cfg.esp_boxes);
            ImGui::Checkbox("Health bars",  &g_cfg.esp_health);
            ImGui::Checkbox("Weapon names", &g_cfg.esp_weapons);
            ImGui::EndDisabled();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}