#pragma once

#include <imnodes.h>
// #include <imnodes_internal.h>

#include "window_object.hpp"
#include "schematic.hpp"
#include "librarian.hpp"


class SchematicEditor: public WindowObject{

    ImNodesEditorContext* context_editor;
    ImNodesContext* context;

    Schematic* schematic;
    Librarian* library;

    bool init;

public:
    SchematicEditor(std::string name): WindowObject(name){
        init = false;
        schematic = nullptr;
        library = nullptr;
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

    void SetLibrary(Librarian* lib){
        library = lib;
        init = true;
    }


    void StoreBlocksPositions(){

        if(!schematic) return;

        ImNodes::SetCurrentContext(context);
        ImNodes::EditorContextSet(context_editor);

        for(auto& b: schematic->blocks){
            ImVec2 pos = ImNodes::GetNodeEditorSpacePos(BlockData::GetImnodeID(b->id));
            b->pos.x = pos.x;
            b->pos.y = pos.y;
        }

        ImNodes::EditorContextSet(nullptr);
        ImNodes::SetCurrentContext(nullptr);
    }


    std::vector<int> GetSelectedBlocksID(){

        // get list of selected blocks
        if(!schematic) return std::vector<int>{};

        ImNodes::SetCurrentContext(context);
        ImNodes::EditorContextSet(context_editor);

        std::vector<int> selected_blocks_id;

        const int count = ImNodes::NumSelectedNodes();
        std::unique_ptr<int[]> selected_imnodes_id = std::make_unique<int[]>(count);
        ImNodes::GetSelectedNodes(selected_imnodes_id.get());

        selected_blocks_id.resize(count);
        for(int i = 0; i < count; i++){
            selected_blocks_id[i] = BlockData::ImnodeToID(selected_imnodes_id[i]);
        }

        ImNodes::EditorContextSet(nullptr);
        ImNodes::SetCurrentContext(nullptr);

        return selected_blocks_id;

    }

    void SelectBlockWithID(std::vector<int> IDs){
        ImNodes::SetCurrentContext(context);
        ImNodes::EditorContextSet(context_editor);

        ImNodes::ClearNodeSelection();
        
        for(int i = 0; i < IDs.size(); i++){
            if(IDs[i] <= 0) continue;
            ImNodes::SelectNode(BlockData::GetImnodeID(IDs[i]));
        }


        ImNodes::EditorContextSet(nullptr);
        ImNodes::SetCurrentContext(nullptr);
    }


private:

    void GetConnectionColor(const BlockData::IO& io, ImColor* color ){

        if(io.type == "bool"){                      // bool - blue
            *color = ImColor(0, 69, 242);
        }else if(io.type == "int64_t"){             // int64_t - green/blue
            *color = ImColor(0, 255, 89);
        }else if(io.type == "double"){              // double - yellow
            *color = ImColor(166, 255, 0);
        }else if(io.type == "std::string"){         // std::string - purple
            *color = ImColor(222, 0, 242);        
        }else{
            *color = ImColor(150,150,150);          // other - grey
        }
    }


public:



    void Render() override{
        
        if (!show) return;

        auto& io = ImGui::GetIO();

        ImGui::SetNextWindowSizeConstraints(ImVec2(100,100), ImVec2(1e+20,1e+20)); // set minimum size to (100, 100)

        if (ImGui::Begin(window_name.c_str(), &show)) {


            ImNodes::SetCurrentContext(context);
            ImNodes::EditorContextSet(context_editor);
            ImNodes::BeginNodeEditor();

            ImNodes::MiniMap(.1, ImNodesMiniMapLocation_BottomRight);



            // right click PopUp
            if(ImGui::IsKeyPressed(ImGuiKey_MouseRight) && ImNodes::IsEditorHovered()){
                ImGui::OpenPopup("##SCHEMATIC_EDITOR_POPUP");
            }

            // render popup
            if(ImGui::BeginPopup("##SCHEMATIC_EDITOR_POPUP")){

                const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();
                
                if(ImGui::BeginMenu("Add")){
                    RenderAddPopup(library->GetLib(), click_pos);                 
                    ImGui::EndMenu();
                }

                ImGui::EndPopup();
            }


            // render blocks 
            if(schematic){
                int execution_number = 0;
                for(auto block: schematic->Blocks()){
                    auto block_data = block->lib_block.lock();
                    
                    if(block_data){
                        int id = block_data->Render(block->id, execution_number, block->parameters);
                    }
                    execution_number++;
                }
            }


            // Setup positions in editor
            if(schematic && init){
                for(auto block: schematic->Blocks()){
                    auto block_data = block->lib_block.lock();
                    if(block_data){
                        int id = BlockData::GetImnodeID(block->id);
                        ImVec2 pos = ImVec2(block->pos.x, block->pos.y);
                        ImNodes::SetNodeGridSpacePos(id, pos);
                    }
                }
            }
            
            // Render links
            if(schematic){
                for(auto& conn: schematic->Connetions()){

                    auto src = conn.src.lock();
                    auto dst = conn.dst.lock();

                    if(!src || !dst) continue;

                    auto src_lib = src->lib_block.lock();
                    if (!src_lib) continue;

                    int src_pin_imnodes = BlockData::GetImnodeOutputID(src->id, conn.src_pin);
                    int dst_pin_imnodes = BlockData::GetImnodeInputID(dst->id, conn.dst_pin);
                    
                    ImColor color;

                    if(src_lib){
                        const BlockData::IO& io = src_lib->Outputs()[conn.src_pin];
                        GetConnectionColor(io, &color );
                    }else{
                        color = ImColor(100,100,100);
                    }

                    ImNodes::PushColorStyle(ImNodesCol_Link, color);
                    ImNodes::Link(conn.id, src_pin_imnodes, dst_pin_imnodes);
                    ImNodes::PopColorStyle();
                }
            }



            // delete elements 
            if (schematic) {
                if ( ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImNodes::IsEditorHovered() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {

                    // delete blocks 
                    for (auto iter = schematic->blocks.begin(); iter != schematic->blocks.end(); /*none*/) {

                        // check if selected
                        auto block = *iter;
                        int id = BlockData::GetImnodeID(block->id);
                        if (ImNodes::IsNodeSelected(id)) {
                            // delete selected 
                            iter = schematic->blocks.erase(iter);
                        }
                        else {
                            iter++;
                        }
                    }

                    // delete connections
                    for (auto iter = schematic->connetions.begin(); iter != schematic->connetions.end(); /*none*/) {

                        // check if selected
                        auto conn = *iter;
                        if (ImNodes::IsLinkSelected(conn.id)) {
                            // delete selected 
                            iter = schematic->connetions.erase(iter);
                        }
                        else {
                            iter++;
                        }
                    }
                }
            }


            init = false;
            


            ImNodes::PushColorStyle(ImNodesCol_Link, IM_COL32(255,0,0,255));
            ImNodes::PushColorStyle(ImNodesCol_LinkHovered, IM_COL32(255,0,0,255));
            ImNodes::PushColorStyle(ImNodesCol_LinkSelected, IM_COL32(255,0,0,255));

            ImNodes::EndNodeEditor();

            {   // Create link 
                // This block must be outside BeginNodeEditor/EndNodeEditor

                int src_id;
                int dst_id;

                bool created_from_stap;

                if (schematic) {
                    if (ImNodes::IsLinkCreated(&src_id, &dst_id, &created_from_stap)) {
                        schematic->CreateConnection(
                            BlockData::ImnodeToID(src_id),
                            BlockData::ImnodeToOutputID(src_id),
                            BlockData::ImnodeToID(dst_id),
                            BlockData::ImnodeToInputID(dst_id)
                        );
                    }
                }
                // This migth be useful in checking if new link source and destination have same data type
                // or something
                // std::cout << context->HoveredPinIdx.HasValue() << "\n";
                
            }

            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();


            ImNodes::EditorContextSet(nullptr);
            ImNodes::SetCurrentContext(nullptr);

        }
        ImGui::End();
            
    }

private:
    void RenderAddPopup(Librarian::Library& lib, const ImVec2 click_pos){

        for(auto& sub_lib: lib.sub_libraries){
            
            if(ImGui::BeginMenu(sub_lib.name.c_str())){
                RenderAddPopup(sub_lib, click_pos);
                ImGui::EndMenu();
            }
        }

        for(auto& block: lib.blocks){

            if(ImGui::MenuItem(block->Name().c_str())){
                // ADD BLOCK to schematic     
                auto b = schematic->CreateBlock(block, 0, 0);
                ImNodes::SetNodeScreenSpacePos(BlockData::GetImnodeID(b->id), click_pos);
            }
        }


    } 



};




