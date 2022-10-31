#pragma once

#include "dockspace.hpp"
#include "status_bar.hpp"
#include "debug_console.hpp"
#include <imgui.h>





class App{
public:
    
    bool show_demo_window = false;
    bool show_file_select_dialog = false;
    bool show_PLC_connection_dialog = false;

    DebugConsole console;


    App():
        console("PLC Message Log") 
    {
        for (int i = 0; i < file_path_max_len; i++)
            file_path[i] = '\0';
    };
    ~App(){};


    void update(){

        int status_bar_size = ShowStatusBar();
        ShowDockspace(status_bar_size);
        ShowMainMenu();

        console.Render();


        if (show_demo_window) 
            console.PushBack(DebugConsole::Priority::INFO ,"XDDDDD");

        if (show_file_select_dialog)
            console.PushBack(DebugConsole::Priority::WARNING, "XDDDDD");

        if (show_PLC_connection_dialog)
            console.PushBack(DebugConsole::Priority::ERROR, "XDDDDD");


        ImGui::Begin("Window1");
        ImGui::Text("Witam Pana");
        ImGui::End();


        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        if (show_file_select_dialog)
            FileSelectDialog();

        if (show_PLC_connection_dialog)
            PLCConnectionDialog();

    }

private:

    void ShowMainMenu() {
        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("File")) {

            if (ImGui::MenuItem("Open Project")) show_file_select_dialog = true;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("PLC")) {

            if (ImGui::MenuItem("Connection")) show_PLC_connection_dialog = true;
            if (ImGui::MenuItem("Message Log")) console.Show(true);

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


    struct PLC_IP {
        //  ip1.ip2.ip3.ip4:port
        char ip1[4];
        char ip2[4];
        char ip3[4];
        char ip4[4];
        char port[6];
        PLC_IP() {
            for (int i = 0; i < 4; i++) 
                ip1[i] = ip2[i] = ip3[i] = ip4[i] = 0;
            for (int i = 0; i < 6; i++) 
                port[i] = 0;
        }
    }PLC_ip;



    void PLCConnectionDialog() {
        ImGui::Begin("PLC Connection", &show_PLC_connection_dialog);

        
        // IP Adress
        auto dot = []() {ImGui::SameLine(); ImGui::Text("."); ImGui::SameLine(); };

        ImGui::PushItemWidth(ImGui::CalcTextSize("123").x * 2);

        ImGui::InputText("##IP1", PLC_ip.ip1, sizeof(PLC_ip.ip1) / sizeof(PLC_ip.ip1[0]), ImGuiInputTextFlags_CharsDecimal); dot();
        ImGui::InputText("##IP2", PLC_ip.ip2, sizeof(PLC_ip.ip2) / sizeof(PLC_ip.ip2[0]), ImGuiInputTextFlags_CharsDecimal); dot();
        ImGui::InputText("##IP3", PLC_ip.ip3, sizeof(PLC_ip.ip3) / sizeof(PLC_ip.ip3[0]), ImGuiInputTextFlags_CharsDecimal); dot();
        ImGui::InputText("##IP4", PLC_ip.ip4, sizeof(PLC_ip.ip4) / sizeof(PLC_ip.ip4[0]), ImGuiInputTextFlags_CharsDecimal);
        
        ImGui::SameLine();
        ImGui::Text("IP Adress");

        ImGui::PopItemWidth();

        ImGui::InputText("Port", PLC_ip.port, sizeof(PLC_ip.port) / sizeof(PLC_ip.port[0]), ImGuiInputTextFlags_CharsDecimal);


        


        ImGui::End();
    }





};