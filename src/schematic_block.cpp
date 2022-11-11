#include "schematic_block.hpp"


const char* BlockData::ErrorToStr(Error err) {
    switch (err)
    {

    case Error::OK: return "OK";
    case Error::PATH_EMPTY: return "PATH_EMPTY";
    case Error::CANNOT_CREATE_PARENT_FOLDER: return "CANNOT_CREATE_PARENT_FOLDER";
    case Error::CANNOT_OPEN_FILE: return "CANNOT_OPEN_FILE";
    case Error::CANNOT_SAVE_FILE: return "CANNOT_SAVE_FILE";
    case Error::JSON_PARSING_ERROR: return "JSON_PARSING_ERROR";
    case Error::JSON_NOT_AN_OBJECT: return "JSON_NOT_AN_OBJECT";
    case Error::JSON_NAME_NOT_A_STRING: return "JSON_NAME_NOT_A_STRING";
    case Error::JSON_MISSING_NAME_FIELD: return "JSON_MISSING_NAME_FIELD";
    case Error::JSON_INPUTS_EL_LABEL_NOT_A_STRING: return "JSON_INPUTS_EL_LABEL_NOT_A_STRING";
    case Error::JSON_INPUTS_EL_TYPE_NOT_A_STRING: return "JSON_INPUTS_EL_TYPE_NOT_A_STRING";
    case Error::JSON_INPUTS_EL_MISSING_LABEL: return "JSON_INPUTS_EL_MISSING_LABEL";
    case Error::JSON_INPUTS_EL_MISSING_TYPE: return "JSON_INPUTS_EL_MISSING_TYPE";
    case Error::JSON_INPUTS_EL_NOT_AN_OBJECT: return "JSON_INPUTS_EL_NOT_AN_OBJECT";
    case Error::JSON_INPUTS_NOT_AN_ARRAY: return "JSON_INPUTS_NOT_AN_ARRAY";
    case Error::JSON_MISSING_INPUTS_FIELD: return "JSON_MISSING_INPUTS_FIELD";
    case Error::JSON_OUTPUTS_EL_LABEL_NOT_A_STRING: return "JSON_OUTPUTS_EL_LABEL_NOT_A_STRING";
    case Error::JSON_OUTPUTS_EL_TYPE_NOT_A_STRING: return "JSON_OUTPUTS_EL_TYPE_NOT_A_STRING";
    case Error::JSON_OUTPUTS_EL_MISSING_LABEL: return "JSON_OUTPUTS_EL_MISSING_LABEL";
    case Error::JSON_OUTPUTS_EL_MISSING_TYPE: return "JSON_OUTPUTS_EL_MISSING_TYPE";
    case Error::JSON_OUTPUTS_EL_NOT_AN_OBJECT: return "JSON_OUTPUTS_EL_NOT_AN_OBJECT";
    case Error::JSON_OUTPUTS_NOT_AN_ARRAY: return "JSON_OUTPUTS_NOT_AN_ARRAY";
    case Error::JSON_MISSING_OUTPUTS_FIELD: return "JSON_MISSING_OUTPUTS_FIELD";

    default: return "Unnown Error";
    }
}


BlockData::Error BlockData::Save(std::filesystem::path _path){
    if(_path.empty()) return Error::PATH_EMPTY;
    path = _path;
    return Save();
}



BlockData::Error BlockData::Save(){
    if(path.empty()) return Error::PATH_EMPTY;

    // create parent folder for all block files
    {
        std::error_code err;
        std::filesystem::create_directory(path, err);
        if(err) return Error::CANNOT_CREATE_PARENT_FOLDER;
    }

    Error err;
    std::string data;
    
    err = SerializeJson(&data);
    if(err != Error::OK) return err;

    std::filesystem::path data_path = path / path.stem().concat(".json");
    err = SaveFile(data_path, data);
    return err;
}





BlockData::Error BlockData::SaveFile(const std::filesystem::path& _path, const std::string& data){
    std::fstream file(_path, std::ios::out | std::ios::binary);

    if(!file.is_open()) return Error::CANNOT_SAVE_FILE;
    if(!file.good()) return Error::CANNOT_SAVE_FILE;

    try{ // try-catch just in case 
        size_t count = data.size();
        const char* data_ptr = data.c_str();

        file.write(data_ptr, count);
        if(!file.is_open()) return Error::CANNOT_SAVE_FILE;
        if(!file.good()) return Error::CANNOT_SAVE_FILE;
    }catch(...){
        return Error::CANNOT_SAVE_FILE;
    }

    return Error::OK; 
}



BlockData::Error BlockData::SerializeJson(std::string* data){
    
    boost::json::array js_inputs;
    for(const auto& i: inputs)
        js_inputs.push_back({{"label",i.label}, {"type", i.type}});

    boost::json::array js_outputs;
    for(const auto& o: outputs)
        js_outputs.push_back({{"label",o.label}, {"type", o.type}});


    boost::json::object js;

    js["name"] = name;
    js.insert(boost::json::object::value_type("inputs", js_inputs));
    js.insert(boost::json::object::value_type("outputs",js_outputs));

    *data = boost::json::serialize(js);

    return Error::OK;
}




BlockData::Error BlockData::Read(const std::filesystem::path &_path)
{
    if(_path.empty()) return Error::PATH_EMPTY;
    path = _path;

    std::string file_data;

    std::filesystem::path data_path = path / _path.stem().concat(".json");
    Error err = LoadFile(data_path, &file_data);
    if (err != Error::OK)
        return err;

    err = ParseJson(file_data);
    return err;
}



BlockData::Error BlockData::LoadFile(const std::filesystem::path &_path, std::string *result)
{
    try
    {
        std::fstream file(_path, std::ios::binary | std::ios::in);

        if (!file.is_open())
            return Error::CANNOT_OPEN_FILE;
        if (!file.good())
            return Error::CANNOT_OPEN_FILE;

        // get file size
        file.seekg(0, std::ios::end);
        size_t count = file.tellg();
        file.seekg(0, std::ios::beg);

        // read file content
        std::unique_ptr<char[]> data_unique_ptr = std::make_unique<char[]>(count); // this prevents memory leaks
        char *data = data_unique_ptr.get();
        file.read(data, count);

        // convert char[] to std::string
        *result = std::string(data, count);

        return Error::OK;
    }
    catch (...)
    {
        return Error::CANNOT_OPEN_FILE;
    }
}



BlockData::Error BlockData::ParseJson(const std::string &str)
{

    std::error_code err;
    //                                          // just to make things simple in future
    boost::json::parse_options parse_opt;   //
    parse_opt.allow_comments = true;        // 1) allow comments in json
    parse_opt.allow_trailing_commas = true; // 2) allow trailing commas

    boost::json::value js = boost::json::parse(str, err, {}, parse_opt);

    if (err)
        return Error::JSON_PARSING_ERROR;

    if (!js.is_object())
        return Error::JSON_NOT_AN_OBJECT;
    auto js_obj = js.as_object();

    // 'name' field
    if (auto js_name = js_obj.if_contains("name"))
    {
        if (auto str = js_name->if_string())
        {

            name = *str;
        }
        else
            return Error::JSON_NAME_NOT_A_STRING;
    }
    else
        return Error::JSON_MISSING_NAME_FIELD;

    // 'inputs' field
    if (auto js_inputs = js_obj.if_contains("inputs"))
    {
        if (auto js_inputs_arr = js_inputs->if_array())
        {

            // iterate over array
            for (auto js_el : *js_inputs_arr)
            {

                if (auto js_el_obj = js_el.if_object())
                {

                    auto js_label = js_el_obj->if_contains("label");
                    auto js_type = js_el_obj->if_contains("type");

                    if (!js_label)
                        return Error::JSON_INPUTS_EL_MISSING_LABEL;
                    if (!js_type)
                        return Error::JSON_INPUTS_EL_MISSING_TYPE;

                    auto js_label_str = js_label->if_string();
                    auto js_type_str = js_type->if_string();

                    if (!js_label_str)
                        return Error::JSON_INPUTS_EL_LABEL_NOT_A_STRING;
                    if (!js_type_str)
                        return Error::JSON_INPUTS_EL_TYPE_NOT_A_STRING;

                    std::string label = js_label_str->c_str();
                    std::string type  = js_type_str->c_str();

                    inputs.emplace_back(label, type);
                }
                else
                    return Error::JSON_INPUTS_EL_NOT_AN_OBJECT;
            }
        }
        else
            return Error::JSON_INPUTS_NOT_AN_ARRAY;
    }
    else
        return Error::JSON_MISSING_INPUTS_FIELD;

    // 'outputs' field
    if (auto js_outputs = js_obj.if_contains("outputs"))
    {
        if (auto js_outputs_arr = js_outputs->if_array())
        {

            // iterate over array
            for (auto js_el : *js_outputs_arr)
            {

                if (auto js_el_obj = js_el.if_object())
                {

                    auto js_label = js_el_obj->if_contains("label");
                    auto js_type = js_el_obj->if_contains("type");

                    if (!js_label)
                        return Error::JSON_OUTPUTS_EL_MISSING_LABEL;
                    if (!js_type)
                        return Error::JSON_OUTPUTS_EL_MISSING_TYPE;

                    auto js_label_str = js_label->if_string();
                    auto js_type_str = js_type->if_string();

                    if (!js_label_str)
                        return Error::JSON_OUTPUTS_EL_LABEL_NOT_A_STRING;
                    if (!js_type_str)
                        return Error::JSON_OUTPUTS_EL_TYPE_NOT_A_STRING;

                    std::string label = js_label_str->c_str();
                    std::string type  = js_type_str->c_str();

                    outputs.emplace_back(label, type);
                }
                else
                    return Error::JSON_OUTPUTS_EL_NOT_AN_OBJECT;
            }
        }
        else
            return Error::JSON_OUTPUTS_NOT_AN_ARRAY;
    }
    else
        return Error::JSON_MISSING_OUTPUTS_FIELD;

    return Error::OK;
}



// TODO: finish this function ASAP
void BlockData::LoadProjectLibrary(std::list<BlockData>* library, std::filesystem::path path){

    if(!library) return;
    if (!std::filesystem::is_directory(path)) return;



    std::filesystem::directory_iterator dir(path);

    for(const auto& dir_entry : dir){
        if(!dir_entry.is_directory()) continue;
        if(dir_entry.path().extension() != ".block") continue;

        std::cout << "Load dir : " << dir_entry.path() << "\n";

        const auto lib_block_name = dir_entry.path().stem().concat(".json");
        std::cout << "Stem : " << lib_block_name << "\n";

        BlockData block;
        Error err = block.Read(dir_entry.path());

        if(err != Error::OK) continue;

        library->push_back(block);
    }

}
