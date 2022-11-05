#include <memory>
#include <vector>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_node_editor.h>
#include "window_object.hpp"
#include "schematic_block.hpp"


// TODO :
// add "SAVE" button

class BlockEditor: public WindowObject{

    std::weak_ptr<BlockData> block;
    ax::NodeEditor::EditorContext* context;

    static constexpr int io_count_limit = 126;
    int inputs_count;
    int outputs_count;

    BlockData block_copy;

    bool show_prewiev;

public:

    BlockEditor(std::shared_ptr<BlockData> _block): 
        WindowObject("block: " + _block->Name() + "##BLOCK_EDITOR"),
        block(_block)
    {
        show = true;

        ax::NodeEditor::Config config;
        config.SettingsFile = nullptr;
        context = ax::NodeEditor::CreateEditor(&config);

        inputs_count = _block->Inputs().size();
        outputs_count = _block->Outputs().size();

        block_copy = *_block;
    }

    ~BlockEditor(){
        ax::NodeEditor::DestroyEditor(context);
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

        if(ImGui::Begin(window_name.c_str(), &show)){

            // block preview
            if(ImGui::Checkbox("Show Prewiev", &show_prewiev)) show_prewiev != show_prewiev;
            if(show_prewiev){ 

                ax::NodeEditor::SetCurrentEditor(context);
                ax::NodeEditor::Begin("##BLOCK_WINDOW", ImVec2(ImGui::GetWindowWidth(), 300));

                auto id = block_copy.Render(1);

                ax::NodeEditor::End();
                //if(ImGui::Button("Center block")) ax::NodeEditor::CenterNodeOnScreen(id);
                if(ImGui::Button("Center block")) ax::NodeEditor::NavigateToContent();
                ax::NodeEditor::SetCurrentEditor(nullptr);
            }
            ImGui::Separator();
            
            { // block edition
                ImGui::BeginChild(1);

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



