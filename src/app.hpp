#pragma once

#include "dockspace.hpp"
#include "status_bar.hpp"
#include <imgui.h>





class App{
public:
    
    bool show_demo_window = false;
    bool show_file_select_dialog = false;


    App(){
        for (int i = 0; i < file_path_max_len; i++)
            file_path[i] = '\0';
    };
    ~App(){};


    void update(){

        int status_bar_size = ShowStatusBar();
        ShowDockspace(status_bar_size);
        ShowMainMenu();


        ImGui::Begin("Window1");
        ImGui::Text("Witam Pana");
        ImGui::End();


        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        if (show_file_select_dialog)
            FileSelectDialog();
        
    }

private:

    void ShowMainMenu() {
        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("File")) {

            if (ImGui::MenuItem("Open Project")) show_file_select_dialog = true;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("DevOptions")) {

            if (ImGui::MenuItem("Show Demo Window", nullptr, show_demo_window)) show_demo_window = !show_demo_window;

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }



    static constexpr size_t file_path_max_len = 4096;
    char file_path[file_path_max_len];


    void FileSelectDialog() {

        // make this dialog fullscreen
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGuiWindowFlags window_flags 
            = ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoDecoration 
            | ImGuiWindowFlags_NoMove 
            | ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("Select project file", &show_file_select_dialog, window_flags)) {

            // make window always focused
            ImGui::SetWindowFocus();

            ImGui::Text("Enter project path");
            ImGui::InputText("Path", file_path, file_path_max_len);
       
            if(ImGui::Button("Cancel")) show_file_select_dialog = false;
            ImGui::Button("Open");
        }
        ImGui::End();
    }

    void LoadProj() {

    }






};