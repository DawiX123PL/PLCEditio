#pragma once

#include <imgui_node_editor.h>
#include "window_object.hpp"
#include "schematic.hpp"



class SchematicEditor: public WindowObject{

    ImNodesEditorContext* context_editor;
    ImNodesContext* context;

    Schematic* schematic;

    bool init;

public:
    SchematicEditor(std::string name): WindowObject(name){
        init = false;
        schematic = nullptr;
        context = ImNodes::CreateContext();
        context_editor = ImNodes::EditorContextCreate();
    };

    ~SchematicEditor() {
        ImNodes::EditorContextFree(context_editor);
        ImNodes::DestroyContext(context);
    }

    void SetSchematic(Schematic* s){
        schematic = s;
        init = true;
    }


    void Render() override{
        
        if (!show) return;

        auto& io = ImGui::GetIO();


        if (ImGui::Begin(window_name.c_str(), &show)) {

            ImNodes::SetCurrentContext(context);
            ImNodes::EditorContextSet(context_editor);
            ImNodes::BeginNodeEditor();


            if(schematic){
                // render blocks 
                for(auto block: schematic->Blocks()){
                    auto block_data = block->lib_block.lock();
                    
                    if(block_data){
                        int id = block_data->Render(block->id);
                    }
                }
            }

            // Setup positions in editor
            if(schematic && init){
                for(auto block: schematic->Blocks()){
                    auto block_data = block->lib_block.lock();
                    if(block_data){
                        int id = block_data->GetImnodeID(block->id);
                        ImVec2 pos = ImVec2(block->pos.x, block->pos.y);
                        ImNodes::SetNodeGridSpacePos(id, pos);
                    }
                }
            }
            
            // Render links

            if(schematic){
                for(auto& conn: schematic->Connetions()){

                    // TODO - FINISH THIS
                  //  conn.

                }
            }


            init = false;
            
            ImNodes::EndNodeEditor();
            ImNodes::EditorContextSet(nullptr);
            ImNodes::SetCurrentContext(nullptr);

        }
        ImGui::End();
            
    }



};




