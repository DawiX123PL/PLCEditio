#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <boost/json.hpp>
#include <imgui.h>
#include <imnodes.h>



class BlockData{

public:
    struct IO{
        std::string label;
        std::string type;
        IO()
            : label(), type() {}; 

        IO(std::string _label, std::string _type)
            : label(_label), type(_type) {}; 
    };

private:

    std::string name;
    std::filesystem::path path;

    std::vector<IO> inputs;
    std::vector<IO> outputs;

    inline int max(int a, int b){
        return a > b ? a : b; 
    }

    static constexpr float pin_icon_size = 24; 

public:

    const std::string& Name(){ return name; };
    const std::filesystem::path& Path(){ return path; };
    const std::vector<IO>& Inputs(){ return inputs; };
    const std::vector<IO>& Outputs(){ return outputs; };


    void SetName(const std::string& _name){ name = _name; };
    void SetPath(const std::string& _path){ path = _path; };
    void SetInputs(const std::vector<IO>& _inputs){ inputs = _inputs; };
    void SetOutputs(const std::vector<IO>& _outputs){ outputs = _outputs; };




    // 
    // id - current object id
    // node_id    = (id << 8)                   
    // input_id   = (id << 8) |          (pin+1)
    // output_id  = (id << 8) | (1<<7) | (pin+1)
    //
    // Imnode Block id
    // 
    //     ID
    // ┌───────┐ ┌──────┐
    // 0000 0012 0000 0000
    // 
    // 
    // Imnode input id
    // 
    //     ID     PinNr+1
    // ┌───────┐  ┌──────┐
    // 0000 0012 0000 0002
    //           ▲
    //           └── 0 - input pin
    // 
    // 
    // Imnode output id
    // 
    //     ID     PinNr+1
    // ┌───────┐  ┌──────┐
    // 0000 0012 1000 0002
    //           ▲
    //           └── 1 - output pin

    static inline ImGuiID GetImnodeID(int id)                { return (id << 8);                    }
    static inline ImGuiID GetImnodeInputID(int id, int pin)  { return (id << 8) |          (pin+1); }
    static inline ImGuiID GetImnodeOutputID(int id, int pin) { return (id << 8) | (1<<7) | (pin+1); }

    static inline int ImnodeToID(ImGuiID id)                 { return (id >> 8);                          }
    static inline int ImnodeToInputID(ImGuiID id)            { return (id & 0b10000000) ? -1 : (id & 0b01111111)-1; }
    static inline int ImnodeToOutputID(ImGuiID id)           { return (id & 0b10000000) ? (id & 0b01111111)-1 : -1; }



    int Render(int id){

        int node_id = GetImnodeID(id);
        int input_id = GetImnodeInputID(id, 0);
        int output_id = GetImnodeOutputID(id, 0);

        ImNodes::BeginNode(node_id);

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(name.c_str());
        ImNodes::EndNodeTitleBar();

        for(auto& i: inputs){
            ImNodes::BeginInputAttribute(input_id++, ImNodesPinShape_Circle);
            ImGui::Text(i.label.c_str());
            ImNodes::EndInputAttribute();
        }

        for(auto& o: outputs){
            ImNodes::BeginOutputAttribute(output_id++, ImNodesPinShape_Circle);
            ImGui::Text(o.label.c_str());
            ImNodes::EndOutputAttribute();
        }

        ImNodes::EndNode();

        return node_id;
    }



    enum class Error{
        OK,
        PATH_EMPTY,
        CANNOT_CREATE_PARENT_FOLDER,
        CANNOT_OPEN_FILE,
        CANNOT_SAVE_FILE,
        JSON_PARSING_ERROR,
        JSON_NOT_AN_OBJECT,

        JSON_NAME_NOT_A_STRING,
        JSON_MISSING_NAME_FIELD,

        JSON_INPUTS_EL_LABEL_NOT_A_STRING,
        JSON_INPUTS_EL_TYPE_NOT_A_STRING,
        JSON_INPUTS_EL_MISSING_LABEL,
        JSON_INPUTS_EL_MISSING_TYPE,
        JSON_INPUTS_EL_NOT_AN_OBJECT,
        JSON_INPUTS_NOT_AN_ARRAY,
        JSON_MISSING_INPUTS_FIELD,

        JSON_OUTPUTS_EL_LABEL_NOT_A_STRING,
        JSON_OUTPUTS_EL_TYPE_NOT_A_STRING,
        JSON_OUTPUTS_EL_MISSING_LABEL,
        JSON_OUTPUTS_EL_MISSING_TYPE,
        JSON_OUTPUTS_EL_NOT_AN_OBJECT,
        JSON_OUTPUTS_NOT_AN_ARRAY,
        JSON_MISSING_OUTPUTS_FIELD,
    };


    void SetDemoBlockData(){
        try{
            name = path.stem().string(); // this function can fail sometimes. for some reason.
        }catch(...){
            name = "Example Block";
        }

        inputs.clear();
        outputs.clear();
    }


    static void LoadProjectLibrary(std::list<BlockData>* library, std::filesystem::path path);
    static const char* ErrorToStr(Error err);

    Error Save(std::filesystem::path _path);
    Error Save();
    Error Read(const std::filesystem::path& _path);

private:
    Error SaveFile(const std::filesystem::path& _path, const std::string& data);
    Error LoadFile(const std::filesystem::path& path, std::string* result);

    Error SerializeJson(std::string* data);
    Error ParseJson(const std::string& str);

};








