#pragma once
#include "dockspace.hpp"


class App{
public:
    
    bool show_demo_window = false;


    App(){};
    ~App(){};


    void update(){

        ShowDockspace();

        ImGui::BeginMainMenuBar();
        if(ImGui::BeginMenu("File")){
            
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("DevOptions")){
            if(ImGui::MenuItem("Show Demo Window")) show_demo_window = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();


        ImGui::Begin("Window1");
        ImGui::Text("Witam Pana");
        ImGui::End();


        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        
    }






};