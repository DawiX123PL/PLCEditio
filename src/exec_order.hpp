#pragma once

#include <imgui.h>
#include <list>
#include <functional>
#include "window_object.hpp"
#include "schematic.hpp"


class ExecutionOrderWindow: public WindowObject
{
private:
    
    Schematic* schematic;

    bool automatic_sort_selected = true;
    enum class DragAndDropBehaviour{Swap, Move} drag_and_drop_behaviour = DragAndDropBehaviour::Swap;

    std::list<std::shared_ptr<Schematic::Block>>::iterator drag_and_drop_source;
    std::list<std::shared_ptr<Schematic::Block>>::iterator drag_and_drop_target;

    std::vector<int> selected_blocks_id;
    std::function<void(std::vector<int>)> on_select_callback;

public:
    ExecutionOrderWindow(const std::string& name, Schematic* s): WindowObject(name), schematic(s){}
    ~ExecutionOrderWindow(){}


    void Render(){
        if(!IsShown()) return;

        if(ImGui::Begin(window_name.c_str(), &show)){
            WindowContent();
        }

        ImGui::End();
    }


    void SetSelectedBlocksID(std::vector<int> selected){
        selected_blocks_id = selected;
    }

    void OnSelectBlock(std::function<void(std::vector<int>)> callback){
        on_select_callback = callback;
    }



private:

    int selected_index = -1;

    void WindowContent(){

        ImGui::Text("Set execution order:");
        if(ImGui::RadioButton("Automaticaly", automatic_sort_selected)) automatic_sort_selected = true;
        if(ImGui::RadioButton("Manualy", !automatic_sort_selected)) automatic_sort_selected = false;

        ImGui::Separator();

        ImGui::BeginDisabled(automatic_sort_selected);
        if(ImGui::RadioButton("Swap", drag_and_drop_behaviour == DragAndDropBehaviour::Swap)) drag_and_drop_behaviour = DragAndDropBehaviour::Swap;
        if(ImGui::RadioButton("Move", drag_and_drop_behaviour == DragAndDropBehaviour::Move)) drag_and_drop_behaviour = DragAndDropBehaviour::Move;
        ImGui::EndDisabled();

        // columns:
        // 0 - execution order number
        // 1 - block name

        
        ImGuiTableFlags table_flags = ImGuiTableFlags_BordersV 
                                    | ImGuiTableFlags_BordersOuter 
                                    | ImGuiTableFlags_SizingStretchProp;

        ImGui::BeginTable("##ExecOrder", 3, table_flags);
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableHeadersRow();


        bool is_any_selected = false;

        bool do_drag_and_drop = false;
        int index = 0;


        for(auto iter = schematic->blocks.begin(); iter != schematic->blocks.end(); iter++){

            ImGui::PushID(index);
            ImGui::TableNextRow();

            auto block = *iter;
            if(block == nullptr) continue;

            // selectable object with execution number
            ImGui::TableSetColumnIndex(0);
            ImGuiSelectableFlags sel_flags = ImGuiSelectableFlags_SelectOnClick 
                                           | ImGuiSelectableFlags_SpanAllColumns
                                           | ImGuiSelectableFlags_AllowItemOverlap;

            bool is_selected = IsIDselected(block->id);
            bool is_selected_old = is_selected;

            if(ImGui::Selectable(std::to_string(index).c_str(), &is_selected, sel_flags)){
                if(is_selected != is_selected_old){
                    is_any_selected = true;
                    
                    if (ImGui::GetIO().KeyCtrl)
                    {
                        if(is_selected_old) UnselectID(block->id);
                        else SelectID(block->id);
                    }else{
                        ClearID();
                        SelectID(block->id);
                    }
                    
                    
                }
            }

            // drag and drop source
            if(!automatic_sort_selected && ImGui::BeginDragDropSource()){
                ImGui::SetDragDropPayload("ExecOrderChange", nullptr, 0);

                drag_and_drop_source = iter;
                const char* src_name = (block != nullptr) ? (block->full_name.c_str()): "---";

                if(drag_and_drop_behaviour == DragAndDropBehaviour::Swap){
                    ImGui::Text("Swap: #%d: %s", 
                        index, 
                        block->full_name.c_str()
                        );
                }
                else if(drag_and_drop_behaviour == DragAndDropBehaviour::Move){
                        ImGui::Text("Move: #%d: %s", 
                        index, 
                        block->full_name.c_str()
                        );
                }
                ImGui::EndDragDropSource();
            }

            // drag and drop target
            if(!automatic_sort_selected && ImGui::BeginDragDropTarget()){
                
                if(ImGui::AcceptDragDropPayload("ExecOrderChange")){
                    drag_and_drop_target = iter;
                    do_drag_and_drop = true;
                }
                ImGui::EndDragDropTarget();
            }
            
            // block ID
            ImGui::TableSetColumnIndex(1);
            if(block != nullptr){
                ImGui::Text("%d", block->id);
            } 

            // block name
            ImGui::TableSetColumnIndex(2);
            if(block != nullptr){
                ImGui::Text(block->full_name.c_str());
            } 

            ImGui::PopID();
            index++;
        }
        
        
        ImGui::EndTable();


        if(do_drag_and_drop){
            if(drag_and_drop_behaviour == DragAndDropBehaviour::Swap){
                SwapBlocks(drag_and_drop_source, drag_and_drop_target);
            }
            if(drag_and_drop_behaviour == DragAndDropBehaviour::Move){
                MoveBlocks(drag_and_drop_source, drag_and_drop_target);
            }
        }

        if(is_any_selected){
            on_select_callback(selected_blocks_id);
        }

    }

private:

    void SwapBlocks(std::list<std::shared_ptr<Schematic::Block>>::iterator iter1, std::list<std::shared_ptr<Schematic::Block>>::iterator iter2){
        std::shared_ptr<Schematic::Block> temp_block1 = *iter1;
        std::shared_ptr<Schematic::Block> temp_block2 = *iter2;
        *iter1 = temp_block2;
        *iter2 = temp_block1;
    }

    void MoveBlocks(std::list<std::shared_ptr<Schematic::Block>>::iterator from, std::list<std::shared_ptr<Schematic::Block>>::iterator to){
        // check which element is first
        bool source_first;
        for(auto iter = schematic->blocks.begin(); iter != schematic->blocks.end(); iter++ ){
            if(from == iter){
                source_first = true; 
                break;
            }
            if(to == iter){
                source_first = false; 
                break;
            }
        }

        std::shared_ptr<Schematic::Block> temp_block = *from;
        schematic->blocks.erase(from);

        if(source_first){
            to++;
            schematic->blocks.insert(to, temp_block);
        }else{
            schematic->blocks.insert(to, temp_block);
        }

    }

    bool IsIDselected(int id){
        for(int i = 0; i < selected_blocks_id.size(); i++){
            if(selected_blocks_id[i] == id) return true;
        }
        return false;
    }


    void UnselectID(int id){
        for(int i = 0; i < selected_blocks_id.size(); i++)
            if(selected_blocks_id[i] == id)
                selected_blocks_id[i] = -1;
    }

    void SelectID(int id){
        selected_blocks_id.push_back(id);
    }

    void ClearID(){
        selected_blocks_id.clear();
    }


    static void HelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }       
    }

};




