#pragma once

#include <imgui.h>



void ShowDockspace(int distance_from_bottom){

    
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags  
        = ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoTitleBar 
        | ImGuiWindowFlags_NoCollapse 
        | ImGuiWindowFlags_NoResize 
        | ImGuiWindowFlags_NoMove 
        | ImGuiWindowFlags_NoBringToFrontOnFocus 
        | ImGuiWindowFlags_NoNavFocus
        | ImGuiWindowFlags_NoDecoration;


    ImVec2 size = viewport->WorkSize;
    size.y -= distance_from_bottom;

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowViewport(viewport->ID);


    bool open = true;
    ImGui::Begin("Dockspace", &open, window_flags);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();

}




