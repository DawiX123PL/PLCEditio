#include <memory>
#include <vector>
#include <functional>
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
    int parameters_count;
    int inputs_count;
    int outputs_count;
    


    struct Block_IO{
        enum class Type                            { BOOL_T,  DOUBLE_T, INT64_T,   STRING_T,      USER_TYPE  };
        static constexpr char const* names[] =     { "Bool",  "Double", "Int64",   "String",      "-User Type-"};
        static constexpr char const* cpp_names[] = { "bool",  "double", "int64_t", "std::string", "bool"};
        static constexpr int names_count =  sizeof(names)/sizeof(names[0]);

        std::string label;
        Type type;
        std::string type_str;

        Block_IO() { 
            label = ""; 
            type = Type::BOOL_T;
            type_str = "bool";
        };
        Block_IO(BlockData::IO io){
            Block_IO();
            FromBlockIO(io);
        }

        void SetType(Type t){
            type = t;
            if(t != Type::USER_TYPE)
                type_str = cpp_names[(int)t];
        }

        void SetType(std::string s){
            type_str = s;
            type = Type::USER_TYPE;
        }

        BlockData::IO ToBlockIO(){
            if(type != Type::USER_TYPE)
                return BlockData::IO(label, cpp_names[(int)type]);
            else
                return BlockData::IO(label, type_str);
        }

        void FromBlockIO(BlockData::IO io){
            label = io.label;
            type_str = io.type;
            if     (io.type == "bool")        type = Type::BOOL_T;
            else if(io.type == "double")      type = Type::DOUBLE_T;
            else if(io.type == "int64_t")     type = Type::INT64_T;
            else if(io.type == "std::string") type = Type::STRING_T;
            else type = Type::USER_TYPE;
        }
    };

    std::vector<Block_IO> inputs_types;
    std::vector<Block_IO> parameters_types;
    std::vector<Block_IO> outputs_types;


    BlockData block_copy;

    std::vector<std::variant<std::monostate, bool, int64_t, double, std::string>> block_memory;

    bool show_prewiev;
    bool center_on_start;
    int block_id;

    bool no_saved = false;

    bool show_delete_block_popup = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> delete_button_timeout;

    static int next_id;

    bool is_std_block;

    std::function<void()> on_save_callback;

// code editor variables
    std::string code_editor_name;
    bool show_code_editor = false;
    
    bool is_code_loaded = false;



    struct BlockCode{
        struct Text{
            std::string str;
            float height;
            void CalcSize(){
                height = ImGui::CalcTextSize((str + "x").c_str()).y;
                // height = ImGui::CalcTextSize(str.c_str()).y + ImGui::CalcTextSize("x").y;
            }
            ImVec2 GetSize(){
                if(height <= 0) CalcSize();
                float style_height = ImGui::GetStyle().FramePadding.x;
                return ImVec2(ImGui::GetWindowWidth(), height + style_height * 2);
            }
        };

        Text user_include;
        Text r_class_prolog;
        Text user_functions;
        Text r_init_func_prolog;
        Text user_init_func_body;
        Text r_update_func_prolog;
        Text user_update_func_body;
        Text r_class_epilog;
    } block_code;


public:

    BlockEditor(std::shared_ptr<BlockData> _block): 
        WindowObject("block: " + _block->FullName() + "##BLOCK_EDITOR"),
        code_editor_name("block: " + _block->FullName() + "##BLOCK_EDITOR_CODE"),
        block(_block)
    {
        on_save_callback = nullptr;
        show = true;
        center_on_start = true;
        block_id = 0;


        context = ImNodes::CreateContext();
        context_editor = ImNodes::EditorContextCreate();

        inputs_count = _block->Inputs().size();
        outputs_count = _block->Outputs().size();
        parameters_count = _block->Parameters().size();

        block_copy = *_block;

        BlockIOsToInternal(block_copy);
        
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

private:

    void BlockIOsToInternal(BlockData b){
        std::vector<BlockData::IO> b_inputs = b.Inputs();
        std::vector<BlockData::IO> b_parameters = b.Parameters();
        std::vector<BlockData::IO> b_outputs = b.Outputs();
        
        inputs_types.clear();
        parameters_types.clear();
        outputs_types.clear();

        for(BlockData::IO in: b_inputs) inputs_types.emplace_back(in);
        for(BlockData::IO param: b_parameters) parameters_types.emplace_back(param);
        for(BlockData::IO out: b_outputs) outputs_types.emplace_back(out);
    }

    void InternalIOsToBlock(BlockData* b){
        std::vector<BlockData::IO> b_inputs;
        std::vector<BlockData::IO> b_parameters;
        std::vector<BlockData::IO> b_outputs;

        for(Block_IO in: inputs_types) b_inputs.emplace_back(in.ToBlockIO());
        for(Block_IO param: parameters_types) b_parameters.emplace_back(param.ToBlockIO());
        for(Block_IO out: outputs_types) b_outputs.emplace_back(out.ToBlockIO());

        b->SetInputs(b_inputs);
        b->SetParameters(b_parameters);
        b->SetOutputs(b_outputs);
    }

public:

    bool IsNoSaved(){return no_saved;}


    bool IsSameBlockAs(std::shared_ptr<BlockData> b){
        try{
            return b == block.lock();
        }catch(...){
            return false;
        }
    }


    void SetOnSaveCallback( std::function<void()> func ){
        on_save_callback = func;
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
                block_id = block_copy.Render(1, 1, block_memory);

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
                    if(inputs_count != inputs_types.size()) inputs_types.resize(inputs_count);

                    if(ImGui::TreeNode("Inputs")){
                        for(int i = 0; i < inputs_types.size(); i++){
                            ImGui::PushID(i);

                            if(ImGui::TreeNode(&i, "input %d", i)){
                                ImGui::BeginDisabled(is_std_block);
                                    ImGui::InputText("Label", &inputs_types[i].label);

                                    int type = (int)inputs_types[i].type;
                                    if(ImGui::Combo("Type", &type,Block_IO::names, Block_IO::names_count)){
                                        inputs_types[i].SetType((Block_IO::Type)type);
                                    }

                                    if(inputs_types[i].type == Block_IO::Type::USER_TYPE){
                                        ImGui::InputText("##UserType", &inputs_types[i].type_str, ImGuiInputTextFlags_CharsNoBlank);
                                    }

                                ImGui::EndDisabled();
                                ImGui::TreePop();
                            }

                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }
                } // End Inputs 


                ImGui::BeginDisabled(is_std_block);
                if(ImGui::InputInt("Parameters count", &parameters_count, 1, 1)){
                    no_saved = true;
                    parameters_count = parameters_count > 0 ? parameters_count : 0;
                    parameters_count = parameters_count < io_count_limit ? parameters_count : io_count_limit;
                }
                ImGui::EndDisabled();


                { // Parameters 
                    if(parameters_count != parameters_types.size()) parameters_types.resize(parameters_count);

                    if(ImGui::TreeNode("parameters")){
                        for(int i = 0; i < parameters_types.size(); i++){
                            ImGui::PushID(i);

                            if(ImGui::TreeNode(&i, "input %d", i)){
                                ImGui::BeginDisabled(is_std_block);
                                    ImGui::InputText("Label", &parameters_types[i].label);

                                    int type = (int)parameters_types[i].type;
                                    if(ImGui::Combo("Type", &type,Block_IO::names, Block_IO::names_count)){
                                        parameters_types[i].SetType((Block_IO::Type)type);
                                    }

                                    if(parameters_types[i].type == Block_IO::Type::USER_TYPE){
                                        ImGui::InputText("##UserType", &parameters_types[i].type_str, ImGuiInputTextFlags_CharsNoBlank);
                                    }

                                ImGui::EndDisabled();
                                ImGui::TreePop();
                            }

                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }
                } // End Parameters 


                ImGui::BeginDisabled(is_std_block);
                if(ImGui::InputInt("Output count", &outputs_count, 1, 1)){
                    no_saved = true;
                    outputs_count = outputs_count > 0 ? outputs_count : 0;
                    outputs_count = outputs_count < io_count_limit ? outputs_count : io_count_limit;
                }
                ImGui::EndDisabled();


                { // Outputs 
                    if(outputs_count != outputs_types.size()) outputs_types.resize(outputs_count);

                    if(ImGui::TreeNode("Outputs")){
                        for(int i = 0; i < outputs_types.size(); i++){
                            ImGui::PushID(i);

                            if(ImGui::TreeNode(&i, "output %d", i)){
                                ImGui::BeginDisabled(is_std_block);
                                    ImGui::InputText("Label", &outputs_types[i].label);
                                    
                                    int type = (int)outputs_types[i].type;
                                    if(ImGui::Combo("Type", &type,Block_IO::names, Block_IO::names_count)){
                                        outputs_types[i].SetType((Block_IO::Type)type);
                                    }

                                    if(outputs_types[i].type == Block_IO::Type::USER_TYPE){
                                        ImGui::InputText("##UserType", &outputs_types[i].type_str, ImGuiInputTextFlags_CharsNoBlank);
                                    }


                                ImGui::EndDisabled();
                                ImGui::TreePop();
                            }

                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }
                } // End Outputs

                InternalIOsToBlock(&block_copy);

                ImGui::Separator();

                if(ImGui::Button("Edit Code", ImVec2(ImGui::GetWindowWidth(), 0))){ 

                    std::string code;
                    BlockData::Error err = block_copy.ReadCode(&code);

                    if(err == BlockData::Error::OK){
                        // convert '\r' -> '\n' for simplicity
                        boost::replace_all(code, "\r", "\n");
                        PreprocessUserCode(code);
                    }else{
                        PreprocessUserCode("");
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
            
            std::string code = MergeCode();
            block_ptr->Save();
            block_ptr->SaveCode(code);

            no_saved = false;
        }
        if(on_save_callback){
            on_save_callback();
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

                
                auto DisplayCode = 
                    [this](const char* id_str, bool read_only, BlockCode::Text& text)
                    {
                        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
                        if(read_only)
                        {
                            flags |= ImGuiInputTextFlags_ReadOnly;
                            ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0, 0, 0, 0));
                        }
                        if(ImGui::InputTextMultiline(id_str, &text.str, text.GetSize(), flags)){
                            text.CalcSize();
                            no_saved = true;   
                        }
                        if(read_only){
                            ImGui::PopStyleColor();
                        }
                    };



                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 0));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

                DisplayCode("##user_include",          false, block_code.user_include);
                DisplayCode("##r_class_prolog",        true,  block_code.r_class_prolog);
                DisplayCode("##user_functions",        false, block_code.user_functions);
                DisplayCode("##r_init_func_prolog",    true,  block_code.r_init_func_prolog);
                DisplayCode("##user_init_func_body",   false, block_code.user_init_func_body);
                DisplayCode("##r_update_func_prolog",  true,  block_code.r_update_func_prolog);
                DisplayCode("##user_update_func_body", false, block_code.user_update_func_body);
                DisplayCode("##r_class_epilog",        true,  block_code.r_class_epilog);

                ImGui::PopStyleVar(2);


                ImGui::EndDisabled();

            ImGui::EndChild();
            ImGui::PopStyleColor();

        }
        ImGui::End();

    }


    void PreprocessUserCode(const std::string& code){

        CodeExtractSection(code, &block_code.user_include, "includes");
        CodeExtractSection(code, &block_code.user_functions, "functions");
        CodeExtractSection(code, &block_code.user_init_func_body, "init");
        CodeExtractSection(code, &block_code.user_update_func_body, "update");

    }



    void PreprocessComputedCode(){


        // EXAMPLE BLOCK CODE

        //          //////****** begin includes ******//////
        //          
        //          //////****** end includes ******//////
        //          namespace __LOCAL{ 
        //          
        //              class test_block{ 
        //              public: 
        //                  double* input0;
        //                  double* input1;
        //                  double  output0;
        //          
        //          //////****** begin functions ******//////
        //          
        //          //////****** end functions ******//////
        //          
        //                  void init(){
        //          //////****** begin init ******//////
        //          
        //          //////****** begin init ******//////
        //                  }
        //          
        //          		void update(){
        //          //////****** begin update ******//////
        //          			output0 = *input0 + *input1;
        //          //////****** begin update ******//////
        //          		}
        //          
        //              };
        //          };


        {
            block_code.r_class_prolog.str = 
                "class " + block_copy.Name() + "_block{ \n"
                "public: \n";
            
            auto inputs = block_copy.Inputs();
            for(int i = 0; i < inputs.size(); i++){
                block_code.r_class_prolog.str +=
                "    " + inputs[i].type + "* input" + std::to_string(i) + ";\n";
            }

            auto parameters = block_copy.Parameters();
            for(int i = 0; i < parameters.size(); i++){
                block_code.r_class_prolog.str +=
                "    " + parameters[i].type + "  parameter" + std::to_string(i) + ";\n";
            }

            auto outputs = block_copy.Outputs();
            for(int i = 0; i < outputs.size(); i++){
                block_code.r_class_prolog.str +=
                "    " + outputs[i].type + "  output" + std::to_string(i) + ";\n";
            }

            block_code.r_class_prolog.CalcSize();
        }

        {
            block_code.r_init_func_prolog.str =
                "\n" 
                "    void init(){";

            block_code.r_init_func_prolog.CalcSize();
        }

        {
            block_code.r_update_func_prolog.str = 
                "    }\n"
                "\n"        
                "    void update(){";
            block_code.r_update_func_prolog.CalcSize();           
        }

        {
            block_code.r_class_epilog.str = 
                    "    }\n"
                    "};\n";
            block_code.r_class_epilog.CalcSize();
        }

    }


    std::string MergeCode(){
        return
              "\n//////****** begin includes ******//////\n"
            + block_code.user_include.str
            + "\n//////****** end includes ******//////\n"
            + block_code.r_class_prolog.str
            + "\n//////****** begin functions ******//////\n"
            + block_code.user_functions.str
            + "\n//////****** end functions ******//////\n"
            + block_code.r_init_func_prolog.str
            + "\n//////****** begin init ******//////\n"
            + block_code.user_init_func_body.str
            + "\n//////****** end init ******//////\n"
            + block_code.r_update_func_prolog.str
            + "\n//////****** begin update ******//////\n"
            + block_code.user_update_func_body.str
            + "\n//////****** end update ******//////\n"
            + block_code.r_class_epilog.str;
    }




    bool CodeExtractSection(const std::string& code, BlockCode::Text* result, std::string marker){

        std::string begin_marker = "\n//////****** begin " + marker + " ******//////\n";
        std::string end_marker = "\n//////****** end " + marker + " ******//////\n";

        size_t start = code.find(begin_marker);
        size_t end = code.find(end_marker);
        
        if(start == std::string::npos) return false;
        if(end == std::string::npos) return false;

        size_t begin_pos = start + begin_marker.size();
        size_t section_len = end - begin_pos;

        if(section_len <= 0) return false;

        result->str = code.substr(begin_pos, section_len); 
        result->CalcSize();

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



