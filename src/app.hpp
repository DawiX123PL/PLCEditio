#pragma once

#include <memory>
#include <imgui.h>
#include "dockspace.hpp"
#include "status_bar.hpp"
#include "debug_console.hpp"
#include "schematic.hpp"
#include "schematic_editor.hpp"
#include "schematic_block.hpp"
#include "block_editor.hpp"


class App{
public:
    
    bool show_demo_window = false;
    bool show_project_tree_window = true;

    bool show_file_select_dialog = false;
    bool show_PLC_connection_dialog = false;

    DebugLogger PLC_connection_log;

    DebugLogger event_log;
    
    Schematic mainSchematic;
    std::list<std::shared_ptr<BlockData>> library;

    std::list<BlockEditor> block_editors;
    SchematicEditor schematic_editor;

  

    App() :
        PLC_connection_log("PLC Message Log"),
        event_log("Event Log"),
        schematic_editor("Schematic Editor")
    {
        schematic_editor.Show(true);

        event_log.Show(true);
        PLC_connection_log.Show(true);

        for (int i = 0; i < file_path_max_len; i++)
            file_path[i] = '\0';
    };
    ~App(){};


    void update(){

        int status_bar_size = ShowStatusBar();
        ShowDockspace(status_bar_size);
        ShowMainMenu();

        PLC_connection_log.Render();
        event_log.Render();
        schematic_editor.Render();

        for(auto& editor: block_editors) editor.Render();

        //if (show_demo_window) 
        //    PLC_connection_log.PushBack(DebugLogger::Priority::INFO ,"XDDDDD");

        //if (show_file_select_dialog)
        //    PLC_connection_log.PushBack(DebugLogger::Priority::WARNING, "XDDDDD");

        //if (show_PLC_connection_dialog)
        //    PLC_connection_log.PushBack(DebugLogger::Priority::ERROR, "XDDDDD");


        ImGui::Begin("Window1");
        ImGui::Text("Witam Pana");
        ImGui::End();


        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        if (show_file_select_dialog)
            FileSelectDialog();

        if (show_PLC_connection_dialog)
            PLCConnectionDialog();

        if (show_project_tree_window)
            ProjectTreeWindow();

    }

private:

    void ShowMainMenu() {
        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Project")) show_file_select_dialog = true;
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("PLC")) {
            if (ImGui::MenuItem("Connection", nullptr, show_PLC_connection_dialog)) show_PLC_connection_dialog = !show_PLC_connection_dialog;
            if (ImGui::MenuItem("Message Log", nullptr, PLC_connection_log.IsShown())) PLC_connection_log.Show(!PLC_connection_log.IsShown());
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("Event Log", nullptr, event_log.IsShown())) event_log.Show(!event_log.IsShown());
            ImGui::Separator();
            if (ImGui::MenuItem("Project Tree", nullptr, show_project_tree_window)) show_project_tree_window = !show_project_tree_window;
            ImGui::Separator();
            if (ImGui::MenuItem("Schematic", nullptr, schematic_editor.IsShown())) schematic_editor.Show(!schematic_editor.IsShown());         
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

        ImGui::OpenPopup("Select project file##PROJECT_FILE_SELECTOR_WINDOW", ImGuiWindowFlags_AlwaysAutoResize);


        // this forces Popup to show on center
        if(show_file_select_dialog)
            ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));


        if (ImGui::BeginPopupModal("Select project file##PROJECT_FILE_SELECTOR_WINDOW", &show_file_select_dialog)) {

            // make window always focused
            ImGui::SetWindowFocus();

            ImGui::Text("Enter project path");
            ImGui::InputText("Path", file_path, file_path_max_len);
       
            if(ImGui::Button("Cancel")) show_file_select_dialog = false;
            
            ImGui::SameLine();

            if (ImGui::Button("Open")) {
                LoadProj(file_path);
                show_file_select_dialog = false;
            }

            ImGui::EndPopup();
        }
    }


    void LoadProj(const char* file_path) {
        Schematic::Error err = mainSchematic.LoadFromJsonFile(file_path);

        if (err == Schematic::Error::OK) 
            event_log.PushBack(DebugLogger::Priority::SUCCESS, "Project loaded succesfully");
        else 
            event_log.PushBack(DebugLogger::Priority::ERROR, Schematic::ErrorToStr(err));


        std::list<BlockData> proj_lib;
        auto proj_lib_path = mainSchematic.Path().parent_path();
        BlockData::LoadProjectLibrary(&proj_lib, proj_lib_path);

        library.clear();

        for (auto& block_from_lib : proj_lib) {
            library.push_back(std::make_shared<BlockData>(block_from_lib));
        }

        mainSchematic.LinkWithLibrary(&library);
        schematic_editor.SetSchematic(&mainSchematic);
        schematic_editor.SetLibrary(&library);
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


    void ProjectTreeWindow() {

        if (ImGui::Begin("Project Tree", &show_project_tree_window)) {

            std::string tree_name;
            try {
                if (mainSchematic.Path().empty()) {
                    tree_name = "Untitled##PROJECT_TREE";
                }
                else { 
                    tree_name = mainSchematic.Path().stem().string() + "##PROJECT_TREE";
                }
            }catch(...){
                tree_name = "????????????????";
            }
            
            if (ImGui::TreeNode(tree_name.c_str())) {

                if(ImGui::TreeNodeEx("Schematic", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen)){
                    if(ImGui::IsItemClicked()) schematic_editor.Show(true);
                }


                if (ImGui::TreeNode("Blocks")) {
                    for (const auto& block : library) {

                        std::string name = block->Name() + "##block";
                        ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                        // open editor window
                        if(ImGui::IsItemClicked()){

                            // check if editor is opened
                            bool isOpen = false;
                            for (auto& ed : block_editors) {
                                if (ed.IsSameBlockAs(block)) {
                                    isOpen = true;
                                    ed.Show(true);
                                    break;
                                }
                            }
                            // open new editor if needed
                            if(!isOpen)
                                block_editors.emplace_back(block);
                        }

                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
                


        }
        ImGui::End();

    }



};