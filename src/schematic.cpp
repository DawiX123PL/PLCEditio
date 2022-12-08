#include "schematic.hpp"
#include <boost/json.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>


const char* Schematic::ErrorToStr(Error err) {
	switch (err) {
	case Error::OK: return "OK";
	case Error::CANNOT_OPEN_FILE: return "CANNOT_OPEN_FILE";
	case Error::CANNOT_SAVE_FILE: return "CANNOT_SAVE_FILE";
	case Error::PATH_EMPTY: return "PATH_EMPTY";
	case Error::JSON_PARSING_ERROR: return "JSON_PARSING_ERROR";
	case Error::JSON_NOT_AN_OBJECT: return "JSON_NOT_AN_OBJECT";
	case Error::JSON_MISSING_BLOCKS_FIELD: return "JSON_MISSING_BLOCKS_FIELD";
	case Error::JSON_BLOCKS_FIELD_NOT_ARRAY: return "JSON_BLOCKS_FIELD_NOT_ARRAY";
	case Error::JSON_CONNECTIONS_FIELD_NOT_ARRAY: return "JSON_CONNECTIONS_FIELD_NOT_ARRAY";
	case Error::JSON_MISSING_CONNECTIONS_FIELD: return "JSON_MISSING_CONNECTIONS_FIELD";
	case Error::JSON_BLOCK_NOT_AN_OBJECT: return "JSON_BLOCK_NOT_AN_OBJECT";
	case Error::JSON_BLOCK_MISSING_ID: return "JSON_BLOCK_MISSING_ID";
	case Error::JSON_BLOCK_INVALID_ID: return "JSON_BLOCK_INVALID_ID";
	case Error::JSON_BLOCK_MISSING_POS: return "JSON_BLOCK_MISSING_POS";
	case Error::JSON_BLOCK_POS_NOT_ARRAY: return "JSON_BLOCK_POS_NOT_ARRAY";
	case Error::JSON_BLOCK_INVALID_POS: return "JSON_BLOCK_INVALID_POS";
	case Error::JSON_BLOCK_MISSING_SRC: return "JSON_BLOCK_MISSING_SRC";
	case Error::JSON_BLOCK_POS_NOT_STRING: return "JSON_BLOCK_POS_NOT_STRING";
	case Error::JSON_BLOCK_MISSING_LOC: return "JSON_BLOCK_MISSING_LOC";
	case Error::JSON_BLOCK_LOC_NOT_STRING: return "JSON_BLOCK_LOC_NOT_STRING";
	case Error::JSON_BLOCK_INVALID_LOC: return "JSON_BLOCK_INVALID_LOC";
	case Error::JSON_BLOCK_NAME_NOT_STRING: return "JSON_BLOCK_NAME_NOT_STRING";
	case Error::JSON_BLOCK_MISSING_NAME: return "JSON_BLOCK_MISSING_NAME";
	case Error::JSON_BLOCK_PARAMETER_INVALID_TYPE: return "JSON_BLOCK_PARAMETER_INVALID_TYPE";
	case Error::JSON_BLOCK_PARAMS_NOT_ARRAY: return "JSON_BLOCK_PARAMS_NOT_ARRAY";
	case Error::JSON_CONNECTION_NOT_AN_OBJECT: return "JSON_CONNECTION_NOT_AN_OBJECT";
	case Error::JSON_CONNECTION_IS_INVALID: return "JSON_CONNECTION_IS_INVALID";
	case Error::JSON_CONNECTION_INVALID_SRC: return "JSON_CONNECTION_INVALID_SRC";
	case Error::JSON_CONNECTION_INVALID_SRCPIN: return "JSON_CONNECTION_INVALID_SRCPIN";
	case Error::JSON_CONNECTION_INVALID_DST: return "JSON_CONNECTION_INVALID_DST";
	case Error::JSON_CONNECTION_INVALID_DSTPIN: return "JSON_CONNECTION_INVALID_DSTPIN";
	default: return "Unnown Error";
	}
}


Schematic::Error Schematic::Read(const std::filesystem::path& _path) {


	path = _path;

	std::string data_str;
	Error file_err = LoadFile(path, &data_str);
	if (file_err != Error::OK) return file_err;

	return ParseJson(data_str);

}




Schematic::Error Schematic::LoadFile(const std::filesystem::path& path, std::string* result) {
	try {

		std::fstream file(path, std::ios::binary | std::ios::in);

		if (!file.is_open()) return Error::CANNOT_OPEN_FILE;
		if (!file.good()) return Error::CANNOT_OPEN_FILE;

		// get file size
		file.seekg(0, std::ios::end);
		size_t count = file.tellg();
		file.seekg(0, std::ios::beg);

		// read file content
		std::unique_ptr<char[]> data_unique_ptr = std::make_unique<char[]>(count); // this prevents memory leaks
		char* data = data_unique_ptr.get();
		file.read(data, count);

		// convert char[] to std::string
		*result = std::string(data, count);

		return Error::OK;

	}
	catch(...) {
		return Error::CANNOT_OPEN_FILE;
	}
}



Schematic::Error Schematic::SaveFile(const std::filesystem::path& _path, const std::string& data){
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



Schematic::Error Schematic::ParseJson(const std::string& data_str) {

	std::vector<Block> blocks_raw;
	std::vector<ConnectionRaw> connections_raw;

	// parse json string

	//                                          // just to make things simple in future
	boost::json::parse_options parse_opt;       // 
	parse_opt.allow_comments = true;			// 1) allow comments in json
	parse_opt.allow_trailing_commas = true;     // 2) allow trailing commas

	std::error_code err;
	boost::json::value js = boost::json::parse(data_str, err, {}, parse_opt);

	if (err) return Error::JSON_PARSING_ERROR;

	if (!js.is_object()) return Error::JSON_NOT_AN_OBJECT;

	auto js_obj = js.as_object();

	// blocks 
	if (auto js_blocks = js_obj.if_contains("blocks")) {

		if (auto js_blocks_list = js_blocks->if_array()) {

			for (int i = 0; i < js_blocks_list->size(); i++) {

				Block block;
				Error e = ParseJsonBlock(js_blocks_list->at(i), &block);
				if (e != Error::OK) return e;
				blocks_raw.push_back(block);
			}

		}
		else { return Error::JSON_BLOCKS_FIELD_NOT_ARRAY; }

	}
	else { return Error::JSON_MISSING_BLOCKS_FIELD; }


	// connections
	if (auto js_connections = js_obj.if_contains("connections")) {

		if (auto js_connections_list = js_connections->if_array()) {

			for (int i = 0; i < js_connections_list->size(); i++) {

				ConnectionRaw conn;
				Error e = ParseJsonConnection(js_connections_list->at(i), &conn);
				if (e != Error::OK) return e;
				connections_raw.push_back(conn);
			}

		}
		else { return Error::JSON_CONNECTIONS_FIELD_NOT_ARRAY; }

	}
	else { return Error::JSON_MISSING_CONNECTIONS_FIELD; }




	blocks.clear();
	connetions.clear();

	// convert blocks to valid representation
	for (const auto& block_raw : blocks_raw) {
		blocks.push_back(std::make_shared<Block>(block_raw));
	}

	// convert connectons to valid internal representation
	int conn_id = 1;
	for (const auto& conn_raw : connections_raw) {

		std::weak_ptr<Block> src_ptr;
		std::weak_ptr<Block> dst_ptr;

		// find blocks with specified indexes
		for (const auto& block_ptr : blocks) {
			if (conn_raw.src == block_ptr.get()->id) src_ptr = block_ptr;
			if (conn_raw.dst == block_ptr.get()->id) dst_ptr = block_ptr;
		}

		connetions.emplace_back(conn_id++, src_ptr, conn_raw.src_pin, dst_ptr, conn_raw.dst_pin);

	}


	// find id for next block/connection
	for(auto& b: blocks){
		next_block_id = next_block_id > b->id ? next_block_id : b->id; // next_block_id = max(next_block_id, id);
	}
	next_block_id++;

	for(auto& c: connetions){
		next_connection_id = next_connection_id  > c.id ? next_connection_id  : c.id; // next_block_id = max(next_block_id, id);
	}
	next_connection_id++;

	return Error::OK;
}




Schematic::Error Schematic::ParseJsonBlock(const boost::json::value& js, Block* block) {
	if (!js.is_object()) return Error::JSON_BLOCK_NOT_AN_OBJECT;

	auto js_obj = js.as_object();

	auto IsInt =
		[](const boost::json::value& js) -> bool
	{
		return js.is_int64() || js.is_uint64();
	};


	auto GetInt =
		[](const boost::json::value& js) -> int
	{
		if (js.is_int64()) return js.as_int64();
		if (js.is_uint64()) return js.as_uint64();
		return 0;
	};


	// get id
	if (auto js_id = js_obj.if_contains("id")) {
		if (uint64_t* num = js_id->if_uint64())
			block->id = *num;
		else if (int64_t* num = js_id->if_int64())
			block->id = *num;
		else
			return Error::JSON_BLOCK_INVALID_ID;
	}
	else { return Error::JSON_BLOCK_MISSING_ID; }

	// get pos
	if (auto js_pos = js_obj.if_contains("pos")) {
		if (auto js_pos_arr = js_pos->if_array()) {
			if (js_pos_arr->size() != 2)
				return Error::JSON_BLOCK_INVALID_POS;

			if (!IsInt(js_pos_arr->at(0))) return Error::JSON_BLOCK_INVALID_POS;
			if (!IsInt(js_pos_arr->at(1))) return Error::JSON_BLOCK_INVALID_POS;

			block->pos.x = GetInt(js_pos_arr->at(0));
			block->pos.y = GetInt(js_pos_arr->at(1));
		}
		else return Error::JSON_BLOCK_POS_NOT_ARRAY;
	}
	else return Error::JSON_BLOCK_MISSING_POS;

	// get src
	if (auto js_name = js_obj.if_contains("name")) {
		if (auto js_name_str = js_name->if_string()) {
			block->full_name = std::string(js_name_str->begin(), js_name_str->end());
		}
		else return Error::JSON_BLOCK_NAME_NOT_STRING;
	}
	else return Error::JSON_BLOCK_MISSING_NAME;

	// get params
	// dont throw error if missing
	if (auto js_params = js_obj.if_contains("params")){
		if(auto js_params_arr = js_params->if_array()){
			
			for(auto& js_par: *js_params_arr){

				if(js_par.is_null())
					block->parameters.emplace_back<std::monostate>(std::monostate());
				else if(js_par.is_bool())
					block->parameters.emplace_back(js_par.as_bool());
				else if(js_par.is_int64())
					block->parameters.emplace_back(js_par.as_int64());
				else if(js_par.is_uint64())
					block->parameters.emplace_back((int64_t)js_par.as_uint64());
				else if(js_par.is_double())
					block->parameters.emplace_back(js_par.as_double());
				else if(js_par.is_string())
					block->parameters.emplace_back(js_par.as_string().c_str());
				else
					return Error::JSON_BLOCK_PARAMETER_INVALID_TYPE; 
					
			}

		}else return Error::JSON_BLOCK_PARAMS_NOT_ARRAY;
	}

	return Error::OK;
}




Schematic::Error Schematic::ParseJsonConnection(const boost::json::value& js, ConnectionRaw* conn) {
	if (!js.is_object())
		return Error::JSON_CONNECTION_NOT_AN_OBJECT;

	auto js_obj = js.as_object();

	auto IsInt =
		[](const boost::json::value* js) -> bool
	{
		return js->is_int64() || js->is_uint64();
	};


	auto GetInt =
		[](const boost::json::value* js) -> int
	{
		if (js->is_int64()) return js->as_int64();
		if (js->is_uint64()) return js->as_uint64();
		return -1;
	};

	
	// src
	if (auto js_num = js_obj.if_contains("src")) {
		if (IsInt(js_num)) conn->src = GetInt(js_num);
		else return Error::JSON_CONNECTION_INVALID_SRC;
	}

	// src_pin
	if (auto js_num = js_obj.if_contains("src_pin")) {
		if (IsInt(js_num)) conn->src_pin = GetInt(js_num);
		else return Error::JSON_CONNECTION_INVALID_SRCPIN;
	}

	// src
	if (auto js_num = js_obj.if_contains("dst")) {
		if (IsInt(js_num)) conn->dst = GetInt(js_num);
		else return Error::JSON_CONNECTION_INVALID_DST;
	}

	// src_pin
	if (auto js_num = js_obj.if_contains("dst_pin")) {
		if (IsInt(js_num)) conn->dst_pin = GetInt(js_num);
		else return Error::JSON_CONNECTION_INVALID_DSTPIN;
	}

	if (!conn->IsValid())
		return Error::JSON_CONNECTION_IS_INVALID;

	return Error::OK;

}



bool Schematic::CodeExtractSection(const std::string& code, std::string* result, const std::string& marker){

	std::string begin_marker = "\n//////****** begin " + marker + " ******//////\n";
	std::string end_marker = "\n//////****** end " + marker + " ******//////\n";

	size_t start = code.find(begin_marker);
	size_t end = code.find(end_marker);
	
	if(start == std::string::npos) return false;
	if(end == std::string::npos) return false;

	size_t begin_pos = start + begin_marker.size();
	size_t section_len = end - begin_pos;

	if(section_len <= 0) return false;

	*result = code.substr(begin_pos, section_len); 

	return true;
}


std::string Schematic::BuildToCPP(){

	std::list<std::string> blocks_cpp_classes;
	std::list<std::shared_ptr<BlockData>> lib_blocks;

	// step 1 - create list of unique library blocks 
	{
		// 1.1 - list all library blocks 
		for(const auto& block: blocks) {
			auto lib_block = block->lib_block.lock(); 

			if(lib_block){
				lib_blocks.push_back(lib_block);
			}else{
				//TODO: handle this error later;
			}
		}

		// 1.2 - remove duplicates
		lib_blocks.unique();
	}


	// step 2 - read and process blocks code
	for(const auto& block_lib: lib_blocks){
		
		std::string full_name =  block_lib->FullName();
		std::string code;

		blocks_cpp_classes.push_back("");
		
		{// 2.1 - read code from file
			BlockData::Error err = block_lib->ReadCode(&code);
			if(err != BlockData::Error::OK) continue; //TODO: handle this error later;
		}

		{// 2.2 - process code and replace name with full_name;

			std::string user_include;
			std::string user_functions;
			std::string user_init_func_body;
			std::string user_update_func_body;

			// extract code
			CodeExtractSection(code, &user_include, "includes");
			CodeExtractSection(code, &user_functions, "functions");
			CodeExtractSection(code, &user_init_func_body, "init");
			CodeExtractSection(code, &user_update_func_body, "update");

			// replace '/' and '\' with '__'
			std::string full_name_processed = full_name;
			boost::replace_all(full_name_processed, "\\", "__");
			boost::replace_all(full_name_processed, "/", "__");


			// class prolog
			std::string r_class_prolog;
			{
				r_class_prolog = 
					"class " + full_name_processed + "_block{ \n"
					"public: \n";
				
				auto inputs = block_lib->Inputs();
				for(int i = 0; i < inputs.size(); i++){
					r_class_prolog +=
					"    " + inputs[i].type + "* input" + std::to_string(i) + ";\n";
				}

				auto parameters = block_lib->Parameters();
				for(int i = 0; i < parameters.size(); i++){
					r_class_prolog +=
					"    " + parameters[i].type + "  parameter" + std::to_string(i) + ";\n";
				}

				auto outputs = block_lib->Outputs();
				for(int i = 0; i < outputs.size(); i++){
					r_class_prolog +=
					"    " + outputs[i].type + "  output" + std::to_string(i) + ";\n";
				}
			}

			// init function prolog
			std::string r_init_func_prolog = 
					"\n" 
					"    void init(){\n";

			// update function prolog
			std::string r_update_func_prolog = 
					"\n"
					"    }\n"
					"\n"        
					"    void update(){\n";
			
			// class epilogue
			std::string r_class_epilog = 
						"\n"
						"    }\n"
						"};\n";


			std::string processed_code = 
			 		  user_include
					+ r_class_prolog
					+ user_functions
					+ r_init_func_prolog
					+ user_init_func_body
					+ r_update_func_prolog
					+ user_update_func_body
					+ r_class_epilog;

			blocks_cpp_classes.back() = processed_code;
		}
	}


	std::list<std::string> blocks_cpp_objects;
	std::list<std::string> blocks_cpp_init;
	std::list<std::string> blocks_cpp_update;

	// step 3 - create all objects representing blocks + init/update functions
	for(const auto& block: blocks){

		std::string class_name = block->GetFullName() + "_block";
		// replace '/' and '\' with '__'
		boost::replace_all(class_name, "\\", "__");
		boost::replace_all(class_name, "/", "__");

		std::string object_name = "block_" + std::to_string(block->id);

		std::string object = class_name + " " + object_name + ";";
		std::string init_call =  object_name + ".init();";
		std::string update_call = object_name + ".update();";

		blocks_cpp_objects.push_back(object);
		blocks_cpp_init.push_back(init_call);
		blocks_cpp_update.push_back(update_call);
	}

	std::list<std::string> connections_cpp;
	// step 4 - create connections between blocks;
	for(const auto& conn: connetions){
		auto src = conn.src.lock();
		auto dst = conn.dst.lock();

		if(!dst || !src) continue; // TODO: handle this error later;

		std::string src_name = "block_" + std::to_string(src->id);
		std::string dst_name = "block_" + std::to_string(dst->id);
		std::string src_output_name = "output" + std::to_string(conn.src_pin);
		std::string dst_input_name = "input" + std::to_string(conn.dst_pin);

		std::string conn_code = dst_name + "." + dst_input_name + " = &" + src_name + "." + src_output_name + ";";

		connections_cpp.push_back(conn_code);
	}

	std::list<std::string> parameters_cpp;
	// step 5 - setup parameters
	for(const auto& block: blocks){


		auto lib_block = block->lib_block.lock();
		if(!lib_block) continue;

		auto params = block->parameters;
		auto lib_params =lib_block->Parameters();


		std::string object_name = "block_" + std::to_string(block->id);
		
		for(int i = 0; i < lib_params.size(); i++){

			std::string param_str;

			// check for bool value 
			if(lib_params[i].type == "bool"){
				if(i < params.size()){
					if(std::holds_alternative<bool>(params[i])){
						std::string val = std::get<bool>(params[i]) ? "true" : "false";
						param_str = object_name + ".parameter" + std::to_string(i) + " = " + val + ";";
					}else{
						param_str = object_name + ".parameter" + std::to_string(i) + " = false; // variant error";
					}
				}else{
					param_str = object_name + ".parameter" + std::to_string(i) + " = false; // default";
				}
			}

			// check for double value 
			else if(lib_params[i].type == "double"){
				if(i < params.size()){
					if(std::holds_alternative<double>(params[i])){
						std::string val = std::to_string(std::get<double>(params[i]));
						param_str =  object_name + ".parameter" + std::to_string(i) + " = " + val + ";";
					}else{
						param_str =  object_name + ".parameter" + std::to_string(i) + " = 0.0; // variant error";
					}
				}else{
					param_str =  object_name + ".parameter" + std::to_string(i) + " = 0.0; // default";
				}
			}

			// check for int64_t value 
			else if(lib_params[i].type == "int64_t"){
				if(i < params.size()){
					if(std::holds_alternative<int64_t>(params[i])){
						std::string val = std::to_string(std::get<int64_t>(params[i]));
						param_str = object_name + ".parameter" + std::to_string(i) + " = " + val + ";";
					}else{
						param_str = object_name + ".parameter" + std::to_string(i) + " = 0; // variant error";
					}
				}else{
					param_str = object_name + ".parameter" + std::to_string(i) + " = 0; // default";
				}
			}

			// check for std::string value 
			else if(lib_params[i].type == "std::string"){
				if(i < params.size()){
					if(std::holds_alternative<std::string>(params[i])){
						std::string val = std::get<std::string>(params[i]);
						param_str = object_name + ".parameter" + std::to_string(i) + " = \"" + val + "\";";
					}else{
						param_str = object_name + ".parameter" + std::to_string(i) + " = \"\"; // variant error";
					}
				}else{
					param_str = object_name + ".parameter" + std::to_string(i) + " = \"\"; // default";
				}
			}

			parameters_cpp.push_back(param_str);
		}


	}


	// step 6 - merge all code

	std::string code = 
	"#include <string>\n"
	"#include <inttypes.h>\n"
	"\n\n"
	"// 	block classes"
	"\n\n";

	for(std::string& _class: blocks_cpp_classes)
		code += _class + "\n"; 
	

	code += 
	"\n\n"
	"int main(){\n"
	"\n\n"
	"// 	block instances\n"
	"\n\n";

	for(std::string& object: blocks_cpp_objects)
		code += "    " + object + "\n";

	code += 
	"\n\n"
	"// 	connections\n"
	"\n\n";

	for(std::string& conn: connections_cpp)
		code += "    " + conn + "\n";

	code += 
	"\n\n"
	"// 	parameters\n"
	"\n\n";

	for(std::string& params: parameters_cpp)
		code += "    " + params + "\n";

	code += 
	"\n\n"
	"// 	Init blocks\n"
	"\n\n";

	for(std::string& inits: blocks_cpp_init)
		code += "    " + inits + "\n";

	code += 
	"\n\n"
	"    while(true){\n\n"
	"// 	Update blocks\n"
	"\n\n";

	for(std::string& inits: blocks_cpp_update)
		code += "        " + inits + "\n";


	code += 
	"\n\n"
	"    }\n"
	"}\n"
	"\n\n";


	return code;

}




