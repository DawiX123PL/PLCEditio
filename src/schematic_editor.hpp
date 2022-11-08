#pragma once

#include <imgui_node_editor.h>
#include "window_object.hpp"
#include "schematic.hpp"



class SchematicEditor: public WindowObject{

    ax::NodeEditor::EditorContext* context;

    Schematic* schematic;

public:
    SchematicEditor(std::string name): WindowObject(name){
        context =  ax::NodeEditor::CreateEditor();
    };

    ~SchematicEditor() {
        ax::NodeEditor::DestroyEditor(context);
    }

    void SetSchematic(Schematic* s){
        schematic = s;
    }


    void Render() override{
        
        if (!show) return;

        auto& io = ImGui::GetIO();


        if (ImGui::Begin(window_name.c_str(), &show)) {

            ax::NodeEditor::SetCurrentEditor(context);
            ax::NodeEditor::Begin("##NODE_EDITOR");

            if(schematic){
                // render blocks 
                for(auto block: schematic->Blocks()){
                    auto block_data = block->lib_block.lock();
                    
                    if(block_data){
                        block_data->Render(block->id);
                    }
                }

            }

            ax::NodeEditor::BeginNode((int)(10 << 8));
            ImGui::Text("Witam pana");
            ax::NodeEditor::EndNode();

            ax::NodeEditor::End();
            ax::NodeEditor::SetCurrentEditor(nullptr);


        }
        ImGui::End();
            
    }



};




