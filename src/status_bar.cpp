#include <imgui.h>

#include "status_bar.hpp"
#include "status_checker.hpp"



int ShowStatusBar(StatusChecker::AppStatus app_status) {
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

    if (app_status == StatusChecker::AppStatus::_RUNNING) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImU32)ImColor(156, 51, 19));
    }

    bool show = true;
    ImGui::Begin("StatusBar", &show, window_flags);

    
    //ImGui::SameLine(ImGui::GetColumnWidth() - ImGui::CalcTextSize("Witam Pana 1234567890").x);
    

    ImGui::BeginTable("StatusBarTable", 4, ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingFixedFit);

    ImGui::TableSetupColumn("##COL0");
    ImGui::TableSetupColumn("##COL1");
    ImGui::TableSetupColumn("##COL2_strech", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##COL3");

    ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Status Bar");

        ImGui::TableSetColumnIndex(1);
            
            ImGui::Text("App Status: "); 
            ImGui::SameLine();

            switch (app_status) {
            case StatusChecker::AppStatus::_DISCONNECTED: ImGui::TextColored(ImColor(255,255,0), "Disconnected"); break;
            case StatusChecker::AppStatus::_UNNOWN:       ImGui::TextColored(ImColor(255,255,0), "Unnown"); break;
            case StatusChecker::AppStatus::_TIMEOUT:      ImGui::TextColored(ImColor(255,255,0), "Communication timeout"); break;
            case StatusChecker::AppStatus::_RUNNING:      ImGui::TextColored(ImColor(0,255,0), "Running"); break;
            case StatusChecker::AppStatus::_STOPPED:      ImGui::TextColored(ImColor(255,0,0), "Stoppped"); break;
            }
        ImGui::TableSetColumnIndex(3);
            ImGui::Text("Witam Pana 1234567890");

    ImGui::EndTable();


    // set window pos and size
    ImVec2 pos = viewport->WorkPos;
    ImVec2 size = viewport->WorkSize;
    pos.y += viewport->WorkSize.y - ImGui::GetWindowSize().y;
    size.y = 0; // heigth of window will adjust automatically (I guess)

    ImGui::SetWindowPos(pos);
    ImGui::SetWindowSize(size);

    int window_heigth = ImGui::GetWindowSize().y;

    ImGui::End();

    if (app_status == StatusChecker::AppStatus::_RUNNING) {
        ImGui::PopStyleColor();
    }

    return window_heigth;
}

