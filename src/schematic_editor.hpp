#pragma once

#include <imgui_node_editor.h>
#include "window_object.hpp"



class SchematicEditor: public WindowObject{

    ax::NodeEditor::EditorContext* context;

public:
    SchematicEditor(std::string name): WindowObject(name){
        context =  ax::NodeEditor::CreateEditor();
    };

    ~SchematicEditor() {
        ax::NodeEditor::DestroyEditor(context);
    }


    void Render() override{
        if (!show) return;

        auto& io = ImGui::GetIO();


        if (ImGui::Begin(window_name.c_str(), &show)) {

            

            ax::NodeEditor::SetCurrentEditor(context);
            ax::NodeEditor::Begin("##NODE_EDITOR");
                namespace ed = ax::NodeEditor;

                int uniqueId = 1;
                // Start drawing nodes.
                ed::BeginNode(uniqueId++);
                    ImGui::Text("Node A");
                    ed::BeginPin(uniqueId++, ed::PinKind::Input);
                        ImGui::Text("-> In");
                    ed::EndPin();

                    ImGui::SameLine();
                    ed::BeginPin(uniqueId++, ed::PinKind::Output);
                        ImGui::Text("Out ->");
                    ed::EndPin();
                ed::EndNode();

                auto a = ed::GetNodePosition(1);

            ax::NodeEditor::End();
            ax::NodeEditor::SetCurrentEditor(nullptr);

            ImGui::Text(" [%f,%f] ", a.x, a.y);


        }
        ImGui::End();
            
    }



};




