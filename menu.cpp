#include "menu.h"
#include "config.h"
#include <imgui.h>

void Menu::Draw() {
    ImGui::SetNextWindowSize({220, 80}, ImGuiCond_Always);
    ImGui::SetNextWindowPos({10, 10},   ImGuiCond_Always);
    ImGui::Begin("##debug", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove);
    ImGui::TextColored({0.4f, 1.f, 0.4f, 1.f}, "HOOK ALIVE");
    ImGui::Text("frame: %d", ImGui::GetFrameCount());
    ImGui::End();
}
