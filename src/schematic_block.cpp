#include "schematic_block.hpp"


const char* Block_Element::ErrorToStr(Error err) {
    switch (err)
    {
    case Error::OK: return "OK";
    case Error::CANNOT_OPEN_FILE: return "CANNOT_OPEN_FILE";
    case Error::JSON_PARSING_ERROR: return "JSON_PARSING_ERROR";
    case Error::JSON_NOT_AN_OBJECT: return "JSON_NOT_AN_OBJECT";
    case Error::JSON_NAME_NOT_A_STRING: return "JSON_NAME_NOT_A_STRING";
    case Error::JSON_MISSING_NAME_FIELD: return "JSON_MISSING_NAME_FIELD";
    case Error::JSON_INPUTS_EL_MISSING_LABEL: return "JSON_INPUTS_EL_MISSING_LABEL";
    case Error::JSON_INPUTS_EL_MISSING_TYPE: return "JSON_INPUTS_EL_MISSING_TYPE";
    case Error::JSON_INPUTS_EL_NOT_AN_OBJECT: return "JSON_INPUTS_EL_NOT_AN_OBJECT";
    case Error::JSON_INPUTS_NOT_AN_ARRAY: return "JSON_INPUTS_NOT_AN_ARRAY";
    case Error::JSON_MISSING_INPUTS_FIELD: return "JSON_MISSING_INPUTS_FIELD";
    case Error::JSON_OUTPUTS_EL_MISSING_LABEL: return "JSON_OUTPUTS_EL_MISSING_LABEL";
    case Error::JSON_OUTPUTS_EL_MISSING_TYPE: return "JSON_OUTPUTS_EL_MISSING_TYPE";
    case Error::JSON_OUTPUTS_EL_NOT_AN_OBJECT: return "JSON_OUTPUTS_EL_NOT_AN_OBJECT";
    case Error::JSON_OUTPUTS_NOT_AN_ARRAY: return "JSON_OUTPUTS_NOT_AN_ARRAY";
    case Error::JSON_MISSING_OUTPUTS_FIELD: return "JSON_MISSING_OUTPUTS_FIELD";
    default: return "Unnown Error";
    }
}


Block_Element::Error Block_Element::FromJsonFile(const std::string &_path)
{
    path = _path;

    std::string file_data;
    Error err = LoadFile(_path, &file_data);
    if (err != Error::OK)
        return err;

    err = ParseJson(file_data);
    return err;
}

Block_Element::Error Block_Element::LoadFile(const std::string &path, std::string *result)
{
    try
    {
        std::fstream file(path, std::ios::binary | std::ios::in);

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

Block_Element::Error Block_Element::ParseJson(const std::string &str)
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
                    
                    // TODO: FIX THIS 
                    // inputs.emplace_back(*js_label_str, *js_type_str);
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

                    auto js_label_str = *js_label->if_string();
                    auto js_type_str = *js_type->if_string();

                    // TODO: FIX THIS
                    //outputs.emplace_back(js_label_str, js_type_str);
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
}
