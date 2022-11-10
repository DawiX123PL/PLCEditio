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

        if(ImGui::Begin(window_name.c_str(), &show)){

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

                std::string name = block_copy.Name();
                ImGui::InputText("Name", &name);
                block_copy.SetName(name);

                ImGui::InputInt("Inputs count", &inputs_count, 1, 1);
                inputs_count = inputs_count > 0 ? inputs_count : 0;
                inputs_count = inputs_count < io_count_limit ? inputs_count : io_count_limit;

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

                ImGui::InputInt("Output count", &outputs_count, 1, 1);
                { // Inputs 
                    // unnecessary copy 
                    // Fix in future
                    auto outputs = block_copy.Outputs();
                    outputs_count = outputs_count > 0 ? outputs_count : 0;
                    outputs_count = outputs_count < io_count_limit ? outputs_count : io_count_limit;

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

                ImGui::EndChild();
            }

        }
        ImGui::End();
    }



};



