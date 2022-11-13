#include <memory>
#include <vector>
#include <imgui.h>
#include <imnodes.h>
#include <misc/cpp/imgui_stdlib.h>
// #include <imgui_node_editor.h>
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

public:

    BlockEditor(std::shared_ptr<BlockData> _block): 
        WindowObject("block: " + _block->Name() + "##BLOCK_EDITOR"),
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

                std::string title = block_copy.Title();
                if(ImGui::InputText("Name", &title))no_saved = true;
                block_copy.SetTitle(title);


                if(ImGui::InputInt("Inputs count", &inputs_count, 1, 1)){
                    no_saved = true;
                    inputs_count = inputs_count > 0 ? inputs_count : 0;
                    inputs_count = inputs_count < io_count_limit ? inputs_count : io_count_limit;
                }

                { // Inputs 
                    // unnecessary copy 
                    // Fix in future
                    auto inputs = block_copy.Inputs();

                    if(inputs_count != inputs.size()) inputs.resize(inputs_count);

                    if(ImGui::TreeNode("Inputs")){
                        for(int i = 0; i < inputs.size(); i++){
                            ImGui::PushID(i);

                            if(ImGui::TreeNode(&i, "input %d", i)){
                                ImGui::InputText("Label", &inputs[i].label);
                                ImGui::InputText("Type", &inputs[i].type);
                                ImGui::TreePop();
                            }

                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }

                    // unnecessary copy (Again) 
                    block_copy.SetInputs(inputs);
                }

                if(ImGui::InputInt("Output count", &outputs_count, 1, 1)){
                    no_saved = true;
                    outputs_count = outputs_count > 0 ? outputs_count : 0;
                    outputs_count = outputs_count < io_count_limit ? outputs_count : io_count_limit;
                }

                { // Inputs 
                    // unnecessary copy 
                    // Fix in future
                    auto outputs = block_copy.Outputs();

                    if(outputs_count != outputs.size()) outputs.resize(outputs_count);

                    if(ImGui::TreeNode("Outputs")){
                        for(int i = 0; i < outputs.size(); i++){
                            ImGui::PushID(i);

                            if(ImGui::TreeNode(&i, "output %d", i)){
                                ImGui::InputText("Label", &outputs[i].label);
                                ImGui::InputText("Type", &outputs[i].type);
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
                {
                    if(ImGui::Button("Close", ImVec2(ImGui::GetWindowWidth()/3, 0))) show = false;

                    ImGui::SameLine();
                    if(ImGui::Button("Save", ImVec2(ImGui::GetWindowWidth()/3, 0))){
                        auto block_ptr = block.lock();
                        if(block_ptr){
                            *block_ptr = block_copy;
                            block_ptr->Save();
                            no_saved = false;    
                        } 
                    } 

                    ImGui::SameLine();
                    if(ImGui::Button("Delete", ImVec2(ImGui::GetWindowWidth()/3, 0))){
                        auto block_ptr = block.lock();
                        if(block_ptr){
                            show_delete_block_popup = true;
                            
                            delete_button_timeout = std::chrono::high_resolution_clock::now() + std::chrono::seconds(7);
                        }
                    } 
                }

                ImGui::EndChild();
            }

        }

        ImGui::End();

        if(show_delete_block_popup)
            DeleteBlockPopup();
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



