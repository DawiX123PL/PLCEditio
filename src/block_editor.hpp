#include <memory>
#include <vector>
#include <imgui.h>
#include <imnodes.h>
#include <misc/cpp/imgui_stdlib.h>
#include <boost/algorithm/string.hpp>
#include "window_object.hpp"
#include "schematic_block.hpp"


// TODO :
// add "SAVE" button

class BlockEditor: public WindowObject{

    std::weak_ptr<BlockData> block;

    ImNodesEditorContext* context_editor;
    ImNodesContext* context;

    static constexpr int io_count_limit = 126;
    int inputs_count;
    int outputs_count;

    BlockData block_copy;

    bool show_prewiev;
    bool center_on_start;
    int block_id;

    bool no_saved = false;

    bool show_delete_block_popup = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> delete_button_timeout;

    static int next_id;

    bool is_std_block;

// code editor variables
    std::string code_editor_name;
    bool show_code_editor = false;
    
    bool is_code_loaded = false;

    std::string block_code;

    std::string block_code_user_include;
    std::string block_code_class_prolog;
    std::string block_code_user_class_body;
    std::string block_code_class_epilog;

    int block_code_size_user_include;
    int block_code_size_class_prolog;
    int block_code_size_user_class_body;
    int block_code_size_class_epilog;



public:

    BlockEditor(std::shared_ptr<BlockData> _block): 
        WindowObject("block: " + _block->FullName() + "##BLOCK_EDITOR"),
        code_editor_name("block: " + _block->FullName() + "##BLOCK_EDITOR_CODE"),
        block(_block)
    {
        show = true;
        center_on_start = true;
        block_id = 0;


        context = ImNodes::CreateContext();
        context_editor = ImNodes::EditorContextCreate();

        inputs_count = _block->Inputs().size();
        outputs_count = _block->Outputs().size();

        block_copy = *_block;
        
        // check if block is from STD
        std::string full_name = _block->FullName();
        const char std[] = "\\STD\\";
        if(full_name.compare(0, sizeof(std)-1, std) == 0){
            // fullname is == "\STD\......";
            is_std_block = true;
        }else{
            is_std_block = false;
        }


    }

    ~BlockEditor(){
        ImNodes::EditorContextFree(context_editor);
        ImNodes::DestroyContext(context);
    }


    bool IsNoSaved(){return no_saved;}


    bool IsSameBlockAs(std::shared_ptr<BlockData> b){
        try{
            return b == block.lock();
        }catch(...){
            return false;
        }
    }


    void Render() override {
        if(!show) return;
        if(block.expired()) return;

        std::shared_ptr<BlockData> block_ptr = block.lock();

        // Creating multiple instances of window with same name 
        // can cause crashing in function ax::NodeEditor::Begin(...).
        // TODO:
        // fix this issue.

        // this prevents ax::NodeEditor::Begin(...) function from crashing (SOMEHOW and only sometimes)
        ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(10000, 10000));


        ImGuiWindowFlags flags = no_saved ? ImGuiWindowFlags_UnsavedDocument : 0;
        if(ImGui::Begin(window_name.c_str(), &show, flags)){

            // block preview
            if(ImGui::Checkbox("Show Preview", &show_prewiev)) show_prewiev != show_prewiev;
            if(show_prewiev){ 

                ImGui::BeginChild(1, ImVec2(ImGui::GetWindowWidth(), 200));
                ImNodes::SetCurrentContext(context);
                ImNodes::EditorContextSet(context_editor);

                if(ImGui::Button("Center block") || center_on_start){
                    if(block_id > 0){
                        ImVec2 block_pos = ImNodes::GetNodeGridSpacePos(block_id);
                        ImVec2 block_size = ImNodes::GetNodeDimensions(block_id);
                        ImVec2 editor_size = ImGui::GetWindowSize();
                        ImVec2 pos = ImVec2(
                            - block_pos.x - block_size.x / 2 + editor_size.x / 2,
                            - block_pos.y - block_size.y / 2 + editor_size.y / 2
                        );
                        ImNodes::EditorContextResetPanning(pos);
                        center_on_start = false;
                    }
                    
                } 

                ImNodes::BeginNodeEditor();

                block_id = block_copy.Render(1);

                ImNodes::EndNodeEditor();

                ImNodes::EditorContextSet(nullptr);
                ImNodes::SetCurrentContext(nullptr);
                ImGui::EndChild();
            }
            ImGui::Separator();
            
            { // block edition
                ImGui::BeginChild(2);

                if(is_std_block)
                    ImGui::TextColored(ImColor(255,255,0), "STD block cannot be modified");

                std::string title = block_copy.Title();

                ImGui::BeginDisabled(is_std_block);
                    if(ImGui::InputText("Name", &title))no_saved = true;
                    block_copy.SetTitle(title);
                ImGui::EndDisabled();


                ImGui::BeginDisabled(is_std_block);
                if(ImGui::InputInt("Inputs count", &inputs_count, 1, 1)){
                    no_saved = true;
                    inputs_count = inputs_count > 0 ? inputs_count : 0;
                    inputs_count = inputs_count < io_count_limit ? inputs_count : io_count_limit;
                }
                ImGui::EndDisabled();


                { // Inputs 
                    // unnecessary copy 
                    // Fix in future
                    auto inputs = block_copy.Inputs();

                    if(inputs_count != inputs.size()) inputs.resize(inputs_count);

                    if(ImGui::TreeNode("Inputs")){
                        for(int i = 0; i < inputs.size(); i++){
                            ImGui::PushID(i);

                            if(ImGui::TreeNode(&i, "input %d", i)){
                                ImGui::BeginDisabled(is_std_block);
                                    ImGui::InputText("Label", &inputs[i].label);
                                    ImGui::InputText("Type", &inputs[i].type);
                                ImGui::EndDisabled();
                                ImGui::TreePop();
                            }

                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }

                    // unnecessary copy (Again) 
                    block_copy.SetInputs(inputs);
                }


                ImGui::BeginDisabled(is_std_block);
                if(ImGui::InputInt("Output count", &outputs_count, 1, 1)){
                    no_saved = true;
                    outputs_count = outputs_count > 0 ? outputs_count : 0;
                    outputs_count = outputs_count < io_count_limit ? outputs_count : io_count_limit;
                }
                ImGui::EndDisabled();


                { // Inputs 
                    // unnecessary copy 
                    // Fix in future
                    auto outputs = block_copy.Outputs();

                    if(outputs_count != outputs.size()) outputs.resize(outputs_count);

                    if(ImGui::TreeNode("Outputs")){
                        for(int i = 0; i < outputs.size(); i++){
                            ImGui::PushID(i);

                            if(ImGui::TreeNode(&i, "output %d", i)){
                                ImGui::BeginDisabled(is_std_block);
                                    ImGui::InputText("Label", &outputs[i].label);
                                    ImGui::InputText("Type", &outputs[i].type);
                                ImGui::EndDisabled();
                                ImGui::TreePop();
                            }

                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }

                    // unnecessary copy (Again) 
                    block_copy.SetOutputs(outputs);
                }

                ImGui::Separator();

                if(ImGui::Button("Edit Code", ImVec2(ImGui::GetWindowWidth(), 0))){ 
                    BlockData::Error err = block_copy.ReadCode(&block_code);

                    // convert '\r' -> '\n' for simplicity
                    boost::replace_all(block_code, "\r", "\n");

                    if(err == BlockData::Error::OK){
                        PreprocessUserCode();
                    }else{
                        InitUserCode();
                    }
                    is_code_loaded = true;
                    show_code_editor = true;
                }

                ImGui::Separator();
                {
                    if(ImGui::Button("Close", ImVec2(ImGui::GetWindowWidth()/3, 0))) show = false;
                    
                    ImGui::BeginDisabled(is_std_block);
                    ImGui::SameLine();
                    if(ImGui::Button("Save", ImVec2(ImGui::GetWindowWidth()/3, 0))){
                        SaveBlock();
                    } 

                    ImGui::SameLine();
                    if(ImGui::Button("Delete", ImVec2(ImGui::GetWindowWidth()/3, 0))){
                        auto block_ptr = block.lock();
                        if(block_ptr){
                            show_delete_block_popup = true;
                            
                            delete_button_timeout = std::chrono::high_resolution_clock::now() + std::chrono::seconds(7);
                        }
                    } 
                    ImGui::EndDisabled();
                }

                ImGui::EndChild();
            }

        }

        ImGui::End();

        if(show_delete_block_popup)
            DeleteBlockPopup();

        if(show_code_editor)
            RenderCodeEditor();
    }

private:

    void SaveBlock(){
        auto block_ptr = block.lock();
        if(block_ptr && !is_std_block){
            *block_ptr = block_copy;
            block_ptr->Save();
            block_ptr->SaveCode(block_code);
            no_saved = false;    
            if(is_code_loaded){
                MergeCode();
                block_ptr->SaveCode(block_code);
            }
        } 
    }

    void RenderCodeEditor(){
        if(!show_code_editor) return;

        PreprocessComputedCode();


        ImGuiWindowFlags flags = 0;
        if(no_saved) flags += ImGuiWindowFlags_UnsavedDocument;

        if(ImGui::Begin(code_editor_name.c_str(), &show_code_editor, flags)){


            ImVec2 button_size = ImVec2(ImGui::GetWindowWidth()/2, 0);
            if(ImGui::Button("Close", button_size)) show = false;

            ImGui::SameLine();

            ImGui::BeginDisabled(is_std_block);
            if(ImGui::Button("Save", button_size)){
                SaveBlock();
            }
            ImGui::EndDisabled();

            if(is_std_block){
                ImGui::TextColored(ImColor(255,255,0), "STD Block cannot be modified");
            }


            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
            

            ImGui::BeginChild("##CODE");
                ImGui::BeginDisabled(is_std_block);

                // ImVec2 size = ImGui::CalcTextSize((block_code + "\n\n\n\nX").c_str());
                // size.x = ImGui::GetWindowWidth();

                // if(ImGui::InputTextMultiline("##code", &block_code, size, ImGuiInputTextFlags_AllowTabInput)){
                //     no_saved = true;
                // }


                block_code_size_user_include = ImGui::CalcTextSize((block_code_user_include + "\nX").c_str()).y;
                block_code_size_user_class_body = ImGui::CalcTextSize((block_code_user_class_body + "\nX").c_str()).y;

                ImVec2 size_include = ImVec2(ImGui::GetWindowWidth(), block_code_size_user_include);
                ImVec2 size_prolog  = ImVec2(ImGui::GetWindowWidth(), block_code_size_class_prolog);
                ImVec2 size_body    = ImVec2(ImGui::GetWindowWidth(), block_code_size_user_class_body);
                ImVec2 size_epilog  = ImVec2(ImGui::GetWindowWidth(), block_code_size_class_epilog);


                if(ImGui::InputTextMultiline("##code_includes", &block_code_user_include,    size_include,ImGuiInputTextFlags_AllowTabInput ))
                    no_saved = true;

                ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0, 0, 0, 0));
                if(ImGui::InputTextMultiline("##code_prolog",   &block_code_class_prolog,    size_prolog, ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly))
                    no_saved = true;
                ImGui::PopStyleColor();

                if(ImGui::InputTextMultiline("##code_body",     &block_code_user_class_body, size_body,   ImGuiInputTextFlags_AllowTabInput ))
                    no_saved = true;

                ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0,0,0,0));
                if(ImGui::InputTextMultiline("##code_epilog",   &block_code_class_epilog,    size_epilog, ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly))
                    no_saved = true;
                ImGui::PopStyleColor();


                ImGui::EndDisabled();

            ImGui::EndChild();
            ImGui::PopStyleColor();

        }
        ImGui::End();

    }


    void PreprocessUserCode(){
        CodeExtractSection(block_code, &block_code_user_include, "includes");
        CodeExtractSection(block_code, &block_code_user_class_body, "functions");

        block_code_size_user_include = ImGui::CalcTextSize((block_code_user_include + "\nX").c_str()).y;
        block_code_size_user_class_body = ImGui::CalcTextSize((block_code_user_class_body + "\nX").c_str()).y;
    }

    void InitUserCode(){
        block_code_user_include = "\n";
        block_code_user_class_body = 
            "\n"
            "        void Init(){\n"
            "\n"
            "        }\n"
            "\n"
            "        void Update(){\n"
            "            \n"
            "        }\n"
            "\n";
    }


    void PreprocessComputedCode(){

        std::string block_namespace = block_copy.GetNamePrefix();
        boost::replace_all(block_namespace, "\\", "__");

        block_code_class_prolog = 
            "namespace " + block_namespace + "{ \n"
            " \n"
            "    class " + block_copy.Name() + "_block{ \n"
            "    public: \n";
            // "        bool input0; \n"
            // "        bool input1; \n"
            // "        bool output0; \n"
            // "        bool output1; \n";
        
        auto inputs = block_copy.Inputs();
        for(int i = 0; i < inputs.size(); i++){
            block_code_class_prolog +=
            "        " + inputs[i].type + "* input" + std::to_string(i) + ";\n";
        }

        auto outputs = block_copy.Outputs();
        for(int i = 0; i < outputs.size(); i++){
            block_code_class_prolog +=
            "        " + outputs[i].type + "  output" + std::to_string(i) + ";\n";
        }

        block_code_class_epilog = 
            "    };\n"
            "};\n";

        block_code_size_class_prolog = ImGui::CalcTextSize((block_code_class_prolog + "\nX").c_str()).y;
        block_code_size_class_epilog = ImGui::CalcTextSize((block_code_class_epilog + "\nX").c_str()).y;
    }


    void MergeCode(){
        block_code =
            + "\n//////****** begin includes ******//////\n"
            + block_code_user_include
            + "\n//////****** end includes ******//////\n"
            + block_code_class_prolog
            + "\n//////****** begin functions ******//////\n"
            + block_code_user_class_body
            + "\n//////****** end functions ******//////\n"
            + block_code_class_epilog;
    }




    bool CodeExtractSection(const std::string& code, std::string* result, std::string marker){

        std::string begin_marker = "\n//////****** begin " + marker + " ******//////\n";
        std::string end_marker = "\n//////****** end " + marker + " ******//////\n";

        size_t start = code.find(begin_marker);
        size_t end = code.find(end_marker);
        
        if(start == std::string::npos) return false;
        if(end == std::string::npos) return false;

        size_t begin_pos = start + begin_marker.size();
        size_t section_len = end - begin_pos;

        if(section_len <= 0) return false;

        *result = code.substr(begin_pos, section_len); 
        return true;
    }




private:

    void DeleteBlockPopup(){
        
        ImGui::OpenPopup("Delete block ?");

        if(show_delete_block_popup)
            ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        // delete block popup;
        if(ImGui::BeginPopupModal("Delete block ?", &show_delete_block_popup)){

            auto block_ptr = block.lock();
            if(!block_ptr) show_delete_block_popup = false;

            std::string msg = "Are you sure you want to delete block: " 
                            + block_ptr->Name() 
                            + "? \n\nName = " 
                            + block_ptr->Name()
                            + "\nPath = ";

            try{msg += block_ptr->Path().string();}catch(...){}

            ImGui::TextWrapped(msg.c_str());
            ImGui::TextColored(ImColor(255,0,0,255), "This operation cannot be undone !!!");

            namespace ch = std::chrono;
            int seconds =  ch::duration_cast<ch::seconds>(delete_button_timeout - ch::high_resolution_clock::now()).count();

            std::string delete_button_text = "Delete";
            if(seconds > 0) delete_button_text += " " + std::to_string(seconds);


            ImGui::Separator();
            ImGui::TextColored(ImColor(255,187,0), "This operation is not yet implemented");


            ImGui::Separator();
            if(ImGui::Button("Cancel", ImVec2(ImGui::GetWindowWidth()/2, 0))) show_delete_block_popup = false;
            
            ImGui::SameLine();
            ImGui::BeginDisabled(seconds > 0);
            if(ImGui::Button(delete_button_text.c_str(), ImVec2(ImGui::GetWindowWidth() / 2, 0))){
                // TODO: Handle block deletion
                show_delete_block_popup = false;
                show = false;
            }
            ImGui::EndDisabled();


            ImGui::EndPopup();
        }
    }



};



