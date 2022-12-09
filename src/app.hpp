#pragma once

#include <memory>
#include <imgui.h>
#include <boost/json.hpp>
#include <array>
#include "dockspace.hpp"
#include "status_bar.hpp"
#include "debug_console.hpp"
#include "schematic.hpp"
#include "schematic_editor.hpp"
#include "schematic_block.hpp"
#include "block_editor.hpp"
#include "librarian.hpp"
#include "tcp_client.hpp"
#include "code_uploader.hpp"
#include "status_checker.hpp"



class App{
public:

    PLCclient plc_client;
    CodeUploader code_uploader{&plc_client};
    StatusChecker status_checker{&plc_client};


    bool show_demo_window = false;
    bool show_project_tree_window = true;

    bool show_open_project_dialog = false;
    bool show_save_project_dialog = false;
    bool show_PLC_connection_dialog = false;

    bool show_create_block_dialog = false;

    bool show_compilation_errors_dialog = false;
    bool show_produced_cpp_code_dialog = false;
    bool show_compilation_flags_editor_dialog = false;


    DebugLogger PLC_connection_log;
    DebugLogger PLC_message_log;
    DebugLogger event_log;
    
    Schematic mainSchematic;
    // std::list<std::shared_ptr<BlockData>> library;
    Librarian library1;


    std::list<BlockEditor> block_editors;
    SchematicEditor schematic_editor;

    const int argc;
    char** argv;
    std::vector<std::string> arg;

    bool code_compilation_running = true;
    std::vector<CodeUploader::CompilationResult> code_compilation_result;
    int code_compilation_errors_count = 0;


    struct AppBuildConfig{
        std::vector<std::string> FilesConst()        { return {"file1.cpp"}; }
        std::vector<std::string> IncludesConst()     { return {}; }
        std::vector<std::string> C_CppFlagsConst() { return {"-Wall"}; }
        std::vector<std::string> LdFlagsConst()    { return {"-Wall"}; }

        std::vector<std::string> files;
        std::vector<std::string> includes;
        std::vector<std::string> c_cpp_flags;
        std::vector<std::string> ld_flags;

        std::string ToString(){
            boost::json::object obj;

            auto ToJsonArray = 
                [](std::vector<std::string>& flags, std::vector<std::string> const_flags)->boost::json::array{
                    boost::json::array arr;
                    for(std::string& str: const_flags)
                        arr.push_back(boost::json::string(str));
                    
                    for(std::string& str: flags)
                        arr.push_back(boost::json::string(str));

                    return arr;
                };
            
            obj["Files"] = ToJsonArray(files, FilesConst());
            obj["Includes"] = ToJsonArray(includes, IncludesConst());
            obj["CPP_flags"] = ToJsonArray(c_cpp_flags, C_CppFlagsConst());
            obj["C_flags"] = ToJsonArray(c_cpp_flags, C_CppFlagsConst());
            obj["LD_flags"] = ToJsonArray(ld_flags, LdFlagsConst());

            return boost::json::serialize(obj);
        }
    };

    AppBuildConfig app_build_config;
    std::string produced_cpp_code;
    float produced_cpp_code_viewsize_y;


    App(int _argc, char** _argv) :
        argc(_argc),
        argv(_argv),
        PLC_connection_log("PLC Connection Log"),
        PLC_message_log("PLC Message Log"),
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


        // app_build_config.clear();
        // app_build_config["Files"] = boost::json::array{ "file1.cpp" };
        // app_build_config["Includes"] = boost::json::array();
        // app_build_config["CPP_flags"] = boost::json::array({"-Wall", "-pass-exit-codes"});
        // app_build_config["C_flags"] = boost::json::array({ "-Wall", "-pass-exit-codes"});
        // app_build_config["LD_flags"] = boost::json::array({ "-Wall", "-pass-exit-codes" });
    };

    ~App(){
        code_uploader.Stop();
        code_uploader.Join();
        plc_client.Stop();
        plc_client.Stop();
    };


    void update(){

        int status_bar_size = ShowStatusBar(status_checker.GetAppStatus());
        ShowDockspace(status_bar_size);
        ShowMainMenu();

        PLC_connection_log.Render();
        PLC_message_log.Render();
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


        // get events from PLC client
        {
            std::queue<PLCclient::Event> events = plc_client.PullEvent();
            while(!events.empty()){
                PLCclient::Event e = events.front();
                events.pop();
                PLC_connection_log.PushBack(e.GetPriority(), e.ToStr());
            }
        }

        { // get all messages received and sent
            std::queue<std::string> rx_messages = plc_client.GetRxMessages();
            std::queue<std::string> tx_messages = plc_client.GetTxMessages();

            while(!rx_messages.empty()){
                std::string& msg = rx_messages.front();
                PLC_message_log.PushBack(DebugLogger::Priority::_WARNING, "RX: " + msg);
                rx_messages.pop();
            }

            while(!tx_messages.empty()){
                std::string& msg = tx_messages.front();
                PLC_message_log.PushBack(DebugLogger::Priority::_INFO, "TX: " + msg);
                tx_messages.pop();
            }

        }

        if(!code_uploader.IsRunning() && code_compilation_running){
            code_compilation_result = code_uploader.GetCompilationResult();
            code_compilation_running = false;
            code_compilation_errors_count = 0;

            for(auto& result: code_compilation_result)
                if(result.exit_code != 0) code_compilation_errors_count ++;
            
        }

        if (code_uploader.IsRunning() && !code_compilation_running) {
            code_compilation_running = true;
        }

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
            if (ImGui::MenuItem("Connection Log", nullptr, PLC_connection_log.IsShown())) PLC_connection_log.Show(!PLC_connection_log.IsShown());
            if (ImGui::MenuItem("Message Log", nullptr, PLC_message_log.IsShown())) PLC_message_log.Show(!PLC_message_log.IsShown());
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
            event_log.PushBack(DebugLogger::Priority::_SUCCESS, "Saved project at: " + path_str );
        else
            event_log.PushBack(DebugLogger::Priority::_ERROR, std::string("Cannot save project: ") + Schematic::ErrorToStr(err) );    
            

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
            event_log.PushBack(DebugLogger::Priority::_SUCCESS, "Project loaded succesfully");
        }
        else{
            event_log.PushBack(DebugLogger::Priority::_ERROR, Schematic::ErrorToStr(err));
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

            std::error_code err;
            bool is_exists = std::filesystem::exists(path, err);
            
            
            if(!err && is_exists){
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

                    event_log.PushBack(DebugLogger::Priority::_SUCCESS, "Created new block");
                }else{
                    std::string msg = std::string("Cannot create new block: ") + BlockData::ErrorToStr(err);
                    event_log.PushBack(DebugLogger::Priority::_ERROR, msg);
                }


                show_create_block_dialog = false;
            }
            ImGui::EndDisabled();

            ImGui::EndPopup();
        }


    }



    TCPclient::IPaddress plc_ip;



    void PLCConnectionDialog() {
        ImGui::Begin("PLC Connection", &show_PLC_connection_dialog);

        ImGui::InputScalarN("IP Adress", ImGuiDataType_U8, plc_ip.addr, 4);
        ImGui::InputScalar("Port", ImGuiDataType_U16, &plc_ip.port);


        TCPclient::Status plc_client_status = plc_client.GetStatus();

        { // Connection indicator
            switch(plc_client_status){
            case TCPclient::Status::CONNECTED:     
                if(plc_client.IsResponding()) ImGui::TextColored(ImColor(0,255,0),   "Connected");    
                else ImGui::TextColored(ImColor(255,255,0), "Connected - Not Responding");
                break;
            case TCPclient::Status::CONNECTING:    ImGui::TextColored(ImColor(255,255,0), "Connecting");   break;
            case TCPclient::Status::DISCONNECTED:  ImGui::TextColored(ImColor(255,0,0),   "Disconnected"); break;
            };
        }


        { // Connect/Disconnect buttons
            ImVec2 button_size = ImVec2(ImGui::GetWindowWidth()/2, 0);
            ImGui::BeginDisabled(plc_client_status != TCPclient::Status::DISCONNECTED);
            if (ImGui::Button("Connect", button_size)){
                plc_client.SetIp(plc_ip);
                plc_client.Connect();
                code_uploader.ClearFlags();
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(plc_client_status != TCPclient::Status::CONNECTED);
            if (ImGui::Button("Disconnect", button_size)){
                plc_client.Disconnect();
                code_uploader.ClearFlags();
            }
            ImGui::EndDisabled();
        }

        
        bool uploading_code = code_uploader.IsRunning();

        ImGui::Separator();

        { // upload and compile button


            ImGui::BeginDisabled(uploading_code);

            ImVec2 button_size = ImVec2(ImGui::GetWindowWidth(), 0);
            if (ImGui::Button("Upload and Compile", button_size)){
                produced_cpp_code = mainSchematic.BuildToCPP();

                code_uploader.ClearFlags();
                code_uploader.UploadAndBuild(produced_cpp_code, app_build_config.ToString());

                produced_cpp_code_viewsize_y = ImGui::CalcTextSize( (produced_cpp_code+"\nX\nX").c_str() ).y;
            }

            ImGui::EndDisabled();




            if (ImGui::Button("Edit compilation flags")) {
                show_compilation_flags_editor_dialog = !show_compilation_flags_editor_dialog;
            }

            if (ImGui::Button("Show produced cpp code")) {
                show_produced_cpp_code_dialog = !show_produced_cpp_code_dialog;
            }

            if (ImGui::Button("Show compilation errors")) {
                show_compilation_errors_dialog = !show_compilation_errors_dialog;
            }


            auto ShowStepStatus = 
                [](CodeUploader::Status flag, std::string msg, std::string step_name)
                {
                    // CodeUploader::Status flag = code_uploader.GetFlagCodeUpload();
                    // std::string msg = code_uploader.GetMsgCodeUpload();
                    // std::string step_name = "Upload code: ";

                    if(flag == CodeUploader::Status::_NONE){
                        std::string text = step_name + ":";
                        ImGui::Text(text.c_str());
                    }else if (flag == CodeUploader::Status::_WAIT) {
                        std::string text = step_name + ": ...";
                        ImGui::TextColored(ImColor(255, 255, 0), text.c_str());
                    }else if(flag == CodeUploader::Status::_OK){
                        std::string text = step_name + ": OK";
                        ImGui::TextColored(ImColor(0, 255, 0), text.c_str());
                    }else if(flag == CodeUploader::Status::_ERROR){
                        std::string text = step_name + ": Error = \"" + msg + "\"";
                        ImGui::TextColored(ImColor(255, 0, 0), text.c_str());
                    }else if(flag == CodeUploader::Status::_TIMEOUT){
                        std::string text = step_name + ": Timeout";
                        ImGui::TextColored(ImColor(255, 0, 0), text.c_str());
                    }else if(flag == CodeUploader::Status::_DISCONNECTED){
                        std::string text = step_name + ": Disconnected";
                        ImGui::TextColored(ImColor(255, 0, 0), text.c_str());
                    }
                };

            ImGui::Text("Status:");
            ImGui::Indent();
            ShowStepStatus(code_uploader.GetFlagStopApp(), code_uploader.GetMsgAppStop(), "Stop App");
            ShowStepStatus(code_uploader.GetFlagCodeUpload(), code_uploader.GetMsgCodeUpload(), "Upload code");
            ShowStepStatus(code_uploader.GetFlagConfigUpload(), code_uploader.GetMsgConfigUpload(), "Upload config");
            ShowStepStatus(code_uploader.GetFlagCodeCompilation(), code_uploader.GetMsgCodeCompilation(), "Compile");
            ImGui::Indent();
        
            if(code_uploader.GetFlagCodeCompilation() == CodeUploader::Status::_OK){
                if(code_compilation_errors_count != 0)
                    ImGui::TextColored(ImColor(255, 0, 0),"Compilation Errors: %d", code_compilation_errors_count);
                else
                    ImGui::TextColored(ImColor(0, 255, 0), "Compilation Errors: %d", code_compilation_errors_count);
                
            }else{
                ImGui::Text("Compilation Errors: ---");
            }


            ImGui::Unindent();

            ImGui::Unindent();

            if(show_compilation_flags_editor_dialog){
                CompilationFlagsEditorWindow();
            }


            // show compilation errors in separate window
            if (show_compilation_errors_dialog) {
                CompilationErrorWindow();
            }

            
            // show show produced cpp code in separate window
            if(show_produced_cpp_code_dialog){
                CppCodeDisplayWindow();
            }

        }

        ImGui::Separator();

        { // Run/Stop Buttons

            ImGui::BeginDisabled(uploading_code);

            ImVec2 button_size = ImVec2(ImGui::GetWindowWidth()/2, 0);
            if (ImGui::Button("Run", button_size)){
                plc_client.AppStart();
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop", button_size)){
                plc_client.AppStop();
            }

            ImGui::EndDisabled();

            
            StatusChecker::AppStatus status = status_checker.GetAppStatus();

            switch(status){
            case StatusChecker::AppStatus::_DISCONNECTED : ImGui::Text("Disconnected"); break;
            case StatusChecker::AppStatus::_UNNOWN :       ImGui::TextColored(ImColor(255,255,0),"Unnown"); break;
            case StatusChecker::AppStatus::_TIMEOUT :      ImGui::TextColored(ImColor(255,255,0),"Communication Timeout"); break;
            case StatusChecker::AppStatus::_RUNNING :      ImGui::TextColored(ImColor(0,255,0),"Running"); break;
            case StatusChecker::AppStatus::_STOPPED :      ImGui::TextColored(ImColor(255,0,0),"Stopped"); break;
            }


        }



        ImGui::End();
    }


    void CppCodeDisplayWindow(){
    
        if (ImGui::Begin("CPP code", &show_produced_cpp_code_dialog)) {

            if(ImGui::Button("Rebuild code")){
                produced_cpp_code = mainSchematic.BuildToCPP();
                produced_cpp_code_viewsize_y = ImGui::CalcTextSize( (produced_cpp_code+"\nX\nX").c_str() ).y;
            }

            ImGui::TextColored(ImColor(255,255,0), "Code is read only");

            ImGui::BeginChild("##CODE", ImVec2(0,0));

            ImVec2 size;
            size.x = ImGui::GetWindowWidth();
            size.y = produced_cpp_code_viewsize_y;

            ImGui::InputTextMultiline("##CODE_TEXT", &produced_cpp_code, size, ImGuiInputTextFlags_ReadOnly);

            ImGui::EndChild();
        }
        ImGui::End();
    
    }


    void CompilationErrorWindow(){

        if (ImGui::Begin("Compilation Errors", &show_compilation_errors_dialog)) {

            if(code_compilation_result.empty()){
                ImGui::TextColored(ImColor(255,255,0), "Compile code to show errors");
            }

            for(int i = 0; i < code_compilation_result.size(); i++){
                CodeUploader::CompilationResult& result = code_compilation_result[i];
                ImGui::PushID(i);

                if(result.exit_code != 0){
                    ImGui::TextColored(ImColor(255, 0, 0), result.file.c_str());
                }else if(!result.error.empty()){
                    ImGui::TextColored(ImColor(255, 255, 0), result.file.c_str());
                }else{
                    ImGui::TextColored(ImColor(0, 255, 0), result.file.c_str());
                }

                ImGui::Indent();

                if(result.exit_code != 0){
                    ImGui::TextColored(ImColor(255, 0, 0), "compilation failed");
                }else if(!result.error.empty()){
                    ImGui::TextColored(ImColor(255, 255, 0), "compiled with warnings");
                }else{
                    ImGui::TextColored(ImColor(0, 255, 0),  "compiled without warnings");
                }

                ImGui::Text(("Exit code: " + std::to_string(result.exit_code )).c_str());
                ImGui::Unindent();

                ImVec2 error_msg_size;
                error_msg_size.x =  ImGui::GetWindowWidth();
                error_msg_size.y = ImGui::CalcTextSize(result.error.c_str()).y + ImGui::CalcTextSize("\n\n\nx").y;

                ImGui::InputTextMultiline("##ErrMsg", &result.error, error_msg_size, ImGuiInputTextFlags_ReadOnly);

                ImGui::PopID();
            }

        }
        ImGui::End();
    }



    void CompilationFlagsEditorWindow(){
        if(ImGui::Begin("Compilation Flags", &show_compilation_flags_editor_dialog)){

            auto FlagsEdit =
                [](std::vector<std::string>& flags, std::vector<std::string> const_flags) {

                int count = flags.size() + const_flags.size();

                ImGui::InputInt("count", &count, 1, 1);
                count = count > const_flags.size() ? count : const_flags.size();

                int editable_count = count - const_flags.size();
                if (editable_count != flags.size())
                    flags.resize(editable_count);

                int i = 0;

                ImGui::BeginDisabled();
                for (std::string& flag : const_flags){
                    ImGui::PushID(i++);
                    ImGui::InputText("##f", &flag);
                    ImGui::PopID();
                }
                ImGui::EndDisabled();

                for (std::string& flag : flags) {
                    ImGui::PushID(i++);
                    ImGui::InputText("##f", &flag);
                    ImGui::PopID();
                }
            };

            if(ImGui::TreeNode("CPP Files")){
                FlagsEdit(app_build_config.files, app_build_config.FilesConst());
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Included Files")){
                FlagsEdit(app_build_config.includes, app_build_config.IncludesConst());
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("c/c++ flags")){
                FlagsEdit(app_build_config.c_cpp_flags, app_build_config.C_CppFlagsConst());
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("Linker flags")){
                FlagsEdit(app_build_config.ld_flags, app_build_config.LdFlagsConst());
                ImGui::TreePop();
            }

            // if(ImGui::TreeNode("Files")){

            //     std::vector<const char*> files_const = app_build_config.FilesConst();
            //     int count = app_build_config.files.size() + files_const.size();
            //     count = count < files_const.size();

            //     ImGui::InputInt("files count", count, 1, 1);

                
            //     int i = 0;
            //     for(std::string& file: app_build_config.files){
            //         ImGui::PushID(i++);
            //         ImGui::InputText("##f",file);
            //         ImGui::PopID()
            //     }

            //     for(std::string& file: app_build_config.files){
            //         ImGui::PushID(i++);
            //         ImGui::InputText("##f",file);
            //         ImGui::PopID()
            //     }
            // }

        }
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
                    block_editors.back().SetOnSaveCallback(
                        [this]()
                        {
                            mainSchematic.RemoveInvalidElements();
                        }
                        );
            }
        }
    }



};