#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <boost/json.hpp>
#include <imgui.h>
#include <imgui_node_editor.h>
// #include <utilities/widgets.h>


class Block_Element{

    struct IO{
        int id;
        std::string label;
        std::string type;
        IO(int _id, const std::string& _label, const std::string& _type)
            : id(_id), label(_label), type(_type) {}; 
    };

    std::string name;
    std::string path;

    std::vector<IO> inputs;
    std::vector<IO> outputs;


    inline int max(int a, int b){
        return a > b ? a : b; 
    }

    static constexpr float pin_icon_size = 24; 

public:


    // TODO: CHANGE INDEXING LATER
    // 
    // id - current object id
    // node_id = id << 8;
    // input_id  = id << 8 + input_number; 
    // input_id  = id << 8 + 127 + output_number;
    //

    void Render(int id){

        int node_id = id << 8;
        int input_id = id << 8;
        int output_id = id << 8 + 127;


        ax::NodeEditor::BeginNode(node_id);

        for(const auto& i_pin: inputs){
            ax::NodeEditor::BeginPin(input_id++, ax::NodeEditor::PinKind::Input);

            const ImVec2 size(pin_icon_size, pin_icon_size);
            // ax::Widgets::Icon(size, ax::Drawing::IconType::Circle, false);
            ImGui::Text(i_pin.label.c_str());

            ax::NodeEditor::EndPin();
        }

        for(const auto& o_pin: outputs){
            ax::NodeEditor::BeginPin(output_id++, ax::NodeEditor::PinKind::Output);

            const ImVec2 size(pin_icon_size, pin_icon_size);
            // ax::Widgets::Icon(size, ax::Drawing::IconType::Circle, false);
            ImGui::Text(o_pin.label.c_str());

            ax::NodeEditor::EndPin();
        }


        ax::NodeEditor::EndNode();
    }



    enum class Error{
        OK,
        CANNOT_OPEN_FILE,
        JSON_PARSING_ERROR,
        JSON_NOT_AN_OBJECT,

        JSON_NAME_NOT_A_STRING,
        JSON_MISSING_NAME_FIELD,

        JSON_INPUTS_EL_MISSING_LABEL,
        JSON_INPUTS_EL_MISSING_TYPE,
        JSON_INPUTS_EL_NOT_AN_OBJECT,
        JSON_INPUTS_NOT_AN_ARRAY,
        JSON_MISSING_INPUTS_FIELD,

        JSON_OUTPUTS_EL_MISSING_LABEL,
        JSON_OUTPUTS_EL_MISSING_TYPE,
        JSON_OUTPUTS_EL_NOT_AN_OBJECT,
        JSON_OUTPUTS_NOT_AN_ARRAY,
        JSON_MISSING_OUTPUTS_FIELD,
    };

    Error FromJsonFile(const std::string& _path);

private:
    Error LoadFile(const std::string& path, std::string* result);    
    Error ParseJson(const std::string& str);
    static const char* ErrorToStr(Error err);

};








