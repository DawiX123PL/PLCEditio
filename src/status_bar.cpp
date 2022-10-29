#include <imgui.h>

#include "status_bar.hpp"


int ShowStatusBar() {
    ImGuiWindowFlags window_flags
        = ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoNavFocus
        | ImGuiWindowFlags_NoDecoration;


    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(viewport->ID);


    bool show = true;
    ImGui::Begin("StatusBar", &show, window_flags);

    
    ImGui::Text("Status Bar");
    ImGui::SameLine(ImGui::GetColumnWidth() - ImGui::CalcTextSize("Witam Pana 1234567890").x);
    ImGui::Text("Witam Pana 1234567890");


    // set window pos and size
    ImVec2 pos = viewport->WorkPos;
    ImVec2 size = viewport->WorkSize;
    pos.y += viewport->WorkSize.y - ImGui::GetWindowSize().y;
    size.y = 0; // heigth of window will adjust automatically (I guess)

    ImGui::SetWindowPos(pos);
    ImGui::SetWindowSize(size);

    int window_heigth = ImGui::GetWindowSize().y;

    ImGui::End();

    return window_heigth;
}

