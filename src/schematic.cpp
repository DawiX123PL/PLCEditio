#include "schematic.hpp"
#include <boost/json.hpp>

#include <vector>


const char* Schematic::ErrorToStr(Error err) {
	switch (err) {
	case Error::OK: return "OK";
	case Error::CANNOT_OPEN_FILE: return "CANNOT_OPEN_FILE";
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
	case Error::JSON_CONNECTION_NOT_AN_OBJECT: return "JSON_CONNECTION_NOT_AN_OBJECT";
	case Error::JSON_CONNECTION_IS_INVALID: return "JSON_CONNECTION_IS_INVALID";
	case Error::JSON_CONNECTION_INVALID_SRC: return "JSON_CONNECTION_INVALID_SRC";
	case Error::JSON_CONNECTION_INVALID_SRCPIN: return "JSON_CONNECTION_INVALID_SRCPIN";
	case Error::JSON_CONNECTION_INVALID_DST: return "JSON_CONNECTION_INVALID_DST";
	case Error::JSON_CONNECTION_INVALID_DSTPIN: return "JSON_CONNECTION_INVALID_DSTPIN";
	default: return "Unnown Error";
	}
}


Schematic::Error Schematic::LoadFromJsonFile(std::string _path) {


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
				ParseJsonBlock(js_blocks_list->at(i), &block);
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
				ParseJsonConnection(js_connections_list->at(i), &conn);
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
	for (const auto& conn_raw : connections_raw) {

		std::weak_ptr<Block> src_ptr;
		std::weak_ptr<Block> dst_ptr;

		// find blocks with specified indexes
		for (const auto& block_ptr : blocks) {
			if (conn_raw.src == block_ptr.get()->id) src_ptr = block_ptr;
			if (conn_raw.dst == block_ptr.get()->id) dst_ptr = block_ptr;
		}

		connetions.emplace_back(src_ptr, conn_raw.src_pin, dst_ptr, conn_raw.dst_pin);

	}


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
			block->pos.x = GetInt(js_pos_arr->at(1));
		}
		else return Error::JSON_BLOCK_POS_NOT_ARRAY;
	}
	else return Error::JSON_BLOCK_MISSING_POS;

	// get src
	if (auto js_src = js_obj.if_contains("pos")) {
		if (auto js_src_str = js_src->if_string()) {
			block->src = std::string(js_src_str->begin(), js_src_str->end());
		}
		else return Error::JSON_BLOCK_POS_NOT_STRING;
	}
	else return Error::JSON_BLOCK_MISSING_SRC;

	// get location
	if (auto js_loc = js_obj.if_contains("loc")) {
		if (auto js_loc_str = js_loc->if_string()) {
			std::string location(js_loc_str->begin(), js_loc_str->end());

			if (location == "PROJ") block->location = Block::Location::PROJECT;
			else if (location == "EXTERN") block->location = Block::Location::EXTERNAL;
			else if (location == "STD") block->location = Block::Location::STD;
			else return Error::JSON_BLOCK_INVALID_LOC;
		}
		else return Error::JSON_BLOCK_LOC_NOT_STRING;
	}
	else return Error::JSON_BLOCK_MISSING_LOC;


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
		if (IsInt(js_num)) conn->src = GetInt(js_num);
		else return Error::JSON_CONNECTION_INVALID_SRCPIN;
	}

	// src
	if (auto js_num = js_obj.if_contains("dst")) {
		if (IsInt(js_num)) conn->src = GetInt(js_num);
		else return Error::JSON_CONNECTION_INVALID_DST;
	}

	// src_pin
	if (auto js_num = js_obj.if_contains("dst_pin")) {
		if (IsInt(js_num)) conn->src = GetInt(js_num);
		else return Error::JSON_CONNECTION_INVALID_DSTPIN;
	}

	if (!conn->IsValid())
		return Error::JSON_CONNECTION_IS_INVALID;

	return Error::OK;

}






