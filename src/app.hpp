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
#include "librarian.hpp"



class App{
public:
    
    bool show_demo_window = false;
    bool show_project_tree_window = true;

    bool show_open_project_dialog = false;
    bool show_save_project_dialog = false;
    bool show_PLC_connection_dialog = false;

    bool show_create_block_dialog = false;

    DebugLogger PLC_connection_log;

    DebugLogger event_log;
    
    Schematic mainSchematic;
    // std::list<std::shared_ptr<BlockData>> library;
    Librarian library1;


    std::list<BlockEditor> block_editors;
    SchematicEditor schematic_editor;

    const int argc;
    char** argv;
    std::vector<std::string> arg;


    App(int _argc, char** _argv) :
        argc(_argc),
        argv(_argv),
        PLC_connection_log("PLC Message Log"),
        event_log("Event Log"),
        schematic_editor("Schematic Editor")
    {

        for(int i = 0; i < argc; i++){
            arg.emplace_back(argv[i]);
            std::cout << argv[i] << "\n";
        }

        std::filesystem::path std_path = std::filesystem::path(arg[0]).parent_path() / "std_blocks";
        library1.SetProjectPath("");
        library1.SetStdLibPath(std_path.lexically_normal());
        library1.Scan();

        schematic_editor.SetSchematic(&mainSchematic);
        schematic_editor.SetLibrary(&library1);
        schematic_editor.Show(true);

        event_log.Show(true);
        PLC_connection_log.Show(true);

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

        //if (show_open_project_dialog)
        //    PLC_connection_log.PushBack(DebugLogger::Priority::WARNING, "XDDDDD");

        //if (show_PLC_connection_dialog)
        //    PLC_connection_log.PushBack(DebugLogger::Priority::ERROR, "XDDDDD");


        ImGui::Begin("Window1");
        ImGui::Text("Witam Pana");
        ImGui::End();


        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        if (show_open_project_dialog)
            OpenProjectDialog();

        if (show_create_block_dialog)
            BlockCreateDialog();

        if (show_PLC_connection_dialog)
            PLCConnectionDialog();

        if (show_project_tree_window)
            ProjectTreeWindow();

        if (show_save_project_dialog)
            SaveProjectAsDialog();

    }

private:

    void ShowMainMenu() {
        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Project")) show_open_project_dialog = true;
            if (ImGui::MenuItem("Save Project")){
                if(mainSchematic.Path().empty())    
                    show_save_project_dialog = true;
                else
                    SaveProj();
            } 

            if (ImGui::MenuItem("Save Project As")) show_save_project_dialog = true;
            
            ImGui::Separator();
            if (ImGui::MenuItem("New Block")) show_create_block_dialog = true;
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
    std::string open_project_file_path;


    void OpenProjectDialog() {

        ImGui::OpenPopup("Select project file##OPEN_PROJECT_WINDOW", ImGuiWindowFlags_AlwaysAutoResize);


        // this forces Popup to show on center
        if(show_open_project_dialog)
            ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));


        if (ImGui::BeginPopupModal("Select project file##OPEN_PROJECT_WINDOW", &show_open_project_dialog)) {

            // make window always focused
            ImGui::SetWindowFocus();

            ImGui::Text("Enter project path");
            ImGui::InputText("Path", &open_project_file_path);

            bool can_save = true;

            if(open_project_file_path.empty()){
                ImGui::TextColored(ImColor(255,0,0), "Path Cannot be empty");
                can_save = false;
            }
       
            ImGui::Separator();
            if(ImGui::Button("Cancel", ImVec2(ImGui::GetWindowSize().x*0.5f, 20))) show_open_project_dialog = false;
            
            ImGui::SameLine();
            ImGui::BeginDisabled(!can_save);
            if (ImGui::Button("Open", ImVec2(ImGui::GetWindowSize().x*0.5f, 20))) {
                LoadProj(open_project_file_path);
                show_open_project_dialog = false;
            }
            ImGui::EndDisabled();
            ImGui::EndPopup();
        }
    }


    bool save_libraries_in_same_location = false;
    

    void SaveProjectAsDialog() {

        ImGui::OpenPopup("Create new Project file##SAVE_PROJECT_WINDOW", ImGuiWindowFlags_AlwaysAutoResize);


        // this forces Popup to show on center
        if(show_save_project_dialog)
            ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));


        if (ImGui::BeginPopupModal("Create new Project file##SAVE_PROJECT_WINDOW", &show_save_project_dialog)) {

            // make window always focused
            ImGui::SetWindowFocus();

            ImGui::Text("Enter project path");
            ImGui::InputText("Path", &open_project_file_path);

            if(ImGui::Checkbox("Save Local library in specified location", &save_libraries_in_same_location))
                save_libraries_in_same_location != save_libraries_in_same_location;
       
            bool can_save = true;

            if(open_project_file_path.empty()){
                ImGui::TextColored(ImColor(255,0,0), "Path Cannot be empty");
                can_save = false;
            }

            ImGui::Separator();
            if(ImGui::Button("Cancel", ImVec2(ImGui::GetWindowSize().x*0.5f, 20))){
                show_save_project_dialog = false;
            }

            ImGui::SameLine();
            ImGui::BeginDisabled(!can_save);
            if (ImGui::Button("Save", ImVec2(ImGui::GetWindowSize().x*0.5f, 20))) {
                SaveProj(open_project_file_path, save_libraries_in_same_location);
                show_save_project_dialog = false;
            }
            ImGui::EndDisabled();

            ImGui::EndPopup();
        }
    }



    void SaveProj(const std::string& file_path = "", bool save_lib = false){

        schematic_editor.StoreBlocksPositions();
        Schematic::Error err;
        if(!file_path.empty())
            err = mainSchematic.Save(file_path);
        else
            err = mainSchematic.Save();

        std::string path_str;
        try { path_str = mainSchematic.Path().string(); }
        catch (...) { path_str = "????????????"; }

        if(err == Schematic::Error::OK)
            event_log.PushBack(DebugLogger::Priority::SUCCESS, "Saved project at: " + path_str );
        else
            event_log.PushBack(DebugLogger::Priority::ERROR, std::string("Cannot save project: ") + Schematic::ErrorToStr(err) );    
            

        if(save_lib && !file_path.empty()){

            library1.CopyLocalLibTo(file_path);

            // for(auto& block: library){
            //     auto proj_root = mainSchematic.Path().parent_path();
            //     auto block_name = block->Path().stem();
            //     block->Save(proj_root / block_name.concat(".block"));
            // }

        }    
    }



    void LoadProj(const std::string& file_path) {
        Schematic::Error err = mainSchematic.Read(file_path);

        if (err == Schematic::Error::OK){
            event_log.PushBack(DebugLogger::Priority::SUCCESS, "Project loaded succesfully");
        }
        else{
            event_log.PushBack(DebugLogger::Priority::ERROR, Schematic::ErrorToStr(err));
            return;
        }

        library1.SetProjectPath(mainSchematic.Path().parent_path());
        library1.ScanProject();

        // std::list<BlockData> proj_lib;
        // auto proj_lib_path = mainSchematic.Path().parent_path();
        // BlockData::LoadProjectLibrary(&proj_lib, proj_lib_path);
        // library.clear();
        // for (auto& block_from_lib : proj_lib) {
        //    library.push_back(std::make_shared<BlockData>(block_from_lib));
        // }

        mainSchematic.LinkWithLibrary(&library1);
        mainSchematic.RemoveInvalidElements();
        schematic_editor.SetSchematic(&mainSchematic);
        schematic_editor.SetLibrary(&library1);
    }


    std::string block_create_new_name;
    std::string block_create_new_path;
    bool block_create_insert_path = false;
    bool block_create_editor_after_creation = true;

    void BlockCreateDialog(){

        ImGui::OpenPopup("Create New Block");

        ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if(ImGui::BeginPopupModal("Create New Block", &show_create_block_dialog)){
            bool can_create = true;

            if(ImGui::Checkbox("Different block path", &block_create_insert_path)) block_create_insert_path != block_create_insert_path;

            ImGui::BeginDisabled(block_create_insert_path);
            ImGui::InputText("Name", &block_create_new_name);
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!block_create_insert_path);
            ImGui::InputText("Path", &block_create_new_path );
            ImGui::EndDisabled();


            std::filesystem::path path;
            
            if(!block_create_insert_path){
                path = mainSchematic.Path().parent_path() / (block_create_new_name + ".block");
                try{ block_create_new_path = path.string(); } catch(...){}
            }else{
                path = block_create_new_path;
                try{ block_create_new_name = path.stem().string(); } catch(...){}
            }

            bool is_exists = std::filesystem::exists(path);
            
            
            if(is_exists){ 
                ImGui::TextColored(ImColor(255,0,0,255), "Block with that name already exists");
                can_create = false;
            }

            if(block_create_new_name.empty()){
                ImGui::TextColored(ImColor(255,0,0,255), "Name cannot be empty");
                can_create = false;
            }
            if(block_create_new_path.empty()){
                ImGui::TextColored(ImColor(255,0,0,255), "Path cannot be empty");
                can_create = false;
            }




            if(ImGui::Checkbox("Open editor after creation", &block_create_editor_after_creation)) block_create_editor_after_creation != block_create_editor_after_creation;


            ImGui::Separator();
            if(ImGui::Button("Cancel", ImVec2(ImGui::GetWindowSize().x*0.5f, 20))) show_create_block_dialog = false;
           
            ImGui::SameLine();

            ImGui::BeginDisabled(!can_create);
            if (ImGui::Button("Create", ImVec2(ImGui::GetWindowSize().x*0.5f, 20))) {

                // create new block and save on hard drive
                BlockData block;
                BlockData::Error err = block.Save(path);
                //block.SetLibraryRoot(mainSchematic.Path().parent_path());

                if(err == BlockData::Error::OK){
                    library1.AddBlock(block);
                    mainSchematic.LinkWithLibrary(&library1);

                    // auto block_ptr = std::make_shared<BlockData>(block);
                    // block_ptr->SetDemoBlockData();

                    // library.push_back(block_ptr);

                    // if(block_create_editor_after_creation)
                    //     block_editors.emplace_back(block_ptr); // open editor after creation 

                    event_log.PushBack(DebugLogger::Priority::SUCCESS, "Created new block");
                }else{
                    std::string msg = std::string("Cannot create new block: ") + BlockData::ErrorToStr(err);
                    event_log.PushBack(DebugLogger::Priority::ERROR, msg);
                }


                show_create_block_dialog = false;
            }
            ImGui::EndDisabled();

            ImGui::EndPopup();
        }


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

                    ShowProjectTreeBlockLib(library1.GetLib());
                    
                    
                    // for (const auto& block : library) {

                    //     std::string name = block->Name() + "##block";
                    //     ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                    //     // open editor window
                    //     if(ImGui::IsItemClicked()){

                    //         // check if editor is opened
                    //         bool isOpen = false;
                    //         for (auto& ed : block_editors) {
                    //             if (ed.IsSameBlockAs(block)) {
                    //                 isOpen = true;
                    //                 ed.Show(true);
                    //                 break;
                    //             }
                    //         }
                    //         // open new editor if needed
                    //         if(!isOpen)
                    //             block_editors.emplace_back(block);
                    //     }

                    // }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }


    void ShowProjectTreeBlockLib(Librarian::Library& lib){
        
        for(auto& sub_lib: lib.sub_libraries){
            if (ImGui::TreeNode(sub_lib.name.c_str())) {
                ShowProjectTreeBlockLib(sub_lib);
                ImGui::TreePop();
            }
        }

        for(auto& block: lib.blocks){
            ImGui::TreeNodeEx(block->Name().c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);

            if (ImGui::IsItemClicked()) {
                bool isOpen = false;
                for (auto &editor : block_editors)                {
                    if (editor.IsSameBlockAs(block)){
                        isOpen = true;
                        editor.Show(true);
                        break;
                    }
                }
                // open new editor if needed
                if (!isOpen)
                    block_editors.emplace_back(block);
            }
        }
    }



};