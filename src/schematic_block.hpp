#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <assert.h>
#include <variant>
#include <boost/json.hpp>
#include <imgui.h>
#include <imnodes.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>



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

    std::string title;
    std::filesystem::path path;
    std::string name;       
    std::string name_prefix;  // full_name is NOT related to block path 
                            // full_name = name_prefix + name;
                            // eg.: 
                            //    name         = "test"
                            //    name_prefix  = "local\example"
                            //    full_name    = "local\example\test"




    std::vector<IO> inputs;
    std::vector<IO> outputs;
    std::vector<IO> parameters;


    inline int max(int a, int b){
        return a > b ? a : b; 
    }

    static constexpr float pin_icon_size = 24; 

public:

    const std::string& Title(){ return title; };
    const std::filesystem::path& Path(){ return path; };
    const std::vector<IO>& Inputs(){ return inputs; };
    const std::vector<IO>& Outputs(){ return outputs; };
    const std::vector<IO>& Parameters(){ return parameters; };
    void SetInputs(const std::vector<IO>& _inputs){ inputs = _inputs; };
    void SetOutputs(const std::vector<IO>& _outputs){ outputs = _outputs; };
    void SetParameters(const std::vector<IO>& _parameters){ parameters = _parameters; };

    const std::string& Name(){return name;}
    const std::string FullName(){return name_prefix + "\\" + name;}

    void SetName(std::string n){ name = n; }
    void SetNamePrefix(std::string n){ name_prefix = n; }
    const std::string& GetNamePrefix(){ return name_prefix; }


    void SetTitle(const std::string& _title){ title = _title; };
    void SetPath(const std::filesystem::path& _path){ 
        path = _path;
    };

    static constexpr float parameter_element_width = 100;


public:


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


    std::vector<std::variant<std::monostate, bool, int64_t, double, std::string>> SetupParameterMemoryTypes(){
        std::vector<std::variant<std::monostate, bool, int64_t, double, std::string>> param_mem;

        for(auto& p: parameters){
            if(p.type == "bool")
                param_mem.emplace_back<bool>(false);
            else if(p.type == "int64_t")
                param_mem.emplace_back<int64_t>(0);
            else if(p.type == "double")
                param_mem.emplace_back<double>(0.0);
            else if(p.type == "std::string")
                param_mem.emplace_back<std::string>("");
            else
                param_mem.emplace_back<std::monostate>(std::monostate());
        }
    }

private:

    void GetPinProperties(const IO& io, ImNodesPinShape* shape, ImColor* color ){

        if(io.type == "bool"){                      // bool - blue circle
            *color = ImColor(0, 69, 242);
            *shape = ImNodesPinShape_Circle;
        }else if(io.type == "int64_t"){             // int64_t - green/blue circle
            *color = ImColor(0, 255, 89);
            *shape = ImNodesPinShape_Circle;
        }else if(io.type == "double"){              // double - yellow circle
            *color = ImColor(166, 255, 0);
            *shape = ImNodesPinShape_Circle;
        }else if(io.type == "std::string"){         // std::string - purple circle
            *color = ImColor(222, 0, 242);        
            *shape = ImNodesPinShape_Circle;
        }else{
            *color = ImColor(150,150,150);          // other - grey triangle
            *shape = ImNodesPinShape_Triangle;
        }
    }


public:

    int Render(int id, std::vector<std::variant<std::monostate, bool, int64_t, double, std::string>>& param_memory){

        int node_id = GetImnodeID(id);
        int input_id = GetImnodeInputID(id, 0);
        int output_id = GetImnodeOutputID(id, 0);

        ImNodes::BeginNode(node_id);

        // Title
        // if(title.size() != 0){ // this feature does not work correctly
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(title.c_str());
            ImNodes::EndNodeTitleBar();
        // }

        // Inputs
        ImGui::BeginGroup();
            for(auto& i: inputs){
                
                ImColor color;
                ImNodesPinShape shape;
                GetPinProperties(i, &shape, &color);
                ImNodes::PushColorStyle(ImNodesCol_Pin, color);
                
                ImNodes::BeginInputAttribute(input_id++, shape);
                ImGui::Text(i.label.c_str());
                ImNodes::EndInputAttribute();

                ImNodes::PopColorStyle();
            }
        ImGui::EndGroup();

        // Vertical Separator
        if(inputs.size() && parameters.size()){
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        }
        ImGui::SameLine();

        // Parameters
        ImGui::BeginGroup();
            ImGui::PushItemWidth(parameter_element_width);
            int param_id = 1;
            if(param_memory.size() != parameters.size()) param_memory.resize(parameters.size());

            for(int i = 0; i <parameters.size(); i++){
                auto& p = parameters[i];
                auto& p_mem = param_memory[i];

                ImGui::PushID(param_id++);

                if(p.type == "bool"){
                    if(!std::holds_alternative<bool>(p_mem)) p_mem = false;

                    bool& val = std::get<bool>(p_mem);
                    int int_val = val ? 1 : 0;
                    ImGui::SliderInt(p.label.c_str(), &int_val, 0, 1);
                    val = int_val != 0;

                }
                else if(p.type == "int64_t"){
                    if(!std::holds_alternative<int64_t>(p_mem)) p_mem = (int64_t) 0;
                    ImGui::InputScalar(p.label.c_str(), ImGuiDataType_S64, &std::get<int64_t>(p_mem));

                }
                else if(p.type == "double"){
                    if(!std::holds_alternative<double>(p_mem)) p_mem = (double) 0.0;
                    double& val = std::get<double>(p_mem);

                    const char* format = (val > 1000000.0) || (val < 1.0/1000000.0) ? "%.6e" : "%.6f";
                    ImGui::InputDouble(p.label.c_str(), &val, 0, 0, format);
                }
                else if(p.type == "std::string"){
                    if(!std::holds_alternative<std::string>(p_mem)) p_mem = std::string("");
                    ImGui::InputText(p.label.c_str(), &std::get<std::string>(p_mem));
                }
                else{
                    ImGui::Text(p.label.c_str());
                }
                
                ImGui::PopID();
            }

            ImGui::PopItemWidth();
        ImGui::EndGroup();
        
        
        // Vertical Separator
        if((parameters.size() && outputs.size())|| (inputs.size()  && outputs.size())){
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        }
        ImGui::SameLine();

        
        // Outputs
        ImGui::BeginGroup();
            for(auto& o: outputs){

                ImColor color;
                ImNodesPinShape shape;
                GetPinProperties(o, &shape, &color);
                ImNodes::PushColorStyle(ImNodesCol_Pin, color);

                ImNodes::BeginOutputAttribute(output_id++, shape);
                ImGui::Text(o.label.c_str());
                ImNodes::EndOutputAttribute();

                ImNodes::PopColorStyle();
            }
        ImGui::EndGroup();


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

        JSON_PARAMETERS_EL_MISSING_LABEL,
        JSON_PARAMETERS_EL_MISSING_TYPE,
        JSON_PARAMETERS_EL_LABEL_NOT_A_STRING,
        JSON_PARAMETERS_EL_TYPE_NOT_A_STRING,
        JSON_PARAMETERS_EL_NOT_AN_OBJECT,
        JSON_PARAMETERS_NOT_AN_ARRAY,
        JSON_MISSING_PARAMETERS_FIELD,

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
            title = path.filename().stem().string(); // this function can fail sometimes. for some reason.
        }catch(...){
            title = "Example Block";
        }

        inputs.clear();
        outputs.clear();
        parameters.clear();
    }


    static const char* ErrorToStr(Error err);

    Error Save(std::filesystem::path _path);
    Error Save();
    Error Read(const std::filesystem::path& _path);


    Error ReadCode(std::string* data){
        assert(data); // data ptr cannot be null;

        Error err;
        err = LoadFile(path / (name + ".cpp"), data);

        return err;
    }

    Error SaveCode(const std::string& data){
        Error err;
        err = SaveFile(path / (name + ".cpp"), data);
        return err;
    }


private:
    Error SaveFile(const std::filesystem::path& _path, const std::string& data);
    Error LoadFile(const std::filesystem::path& path, std::string* result);

    Error SerializeJson(std::string* data);
    Error ParseJson(const std::string& str);

};








