#pragma once

#include <list>
#include <memory>
#include <string>
#include <fstream>
#include <boost/json.hpp>
#include <filesystem>
#include <variant>
#include <inttypes.h>

#include "schematic_block.hpp"
#include "librarian.hpp"


class Schematic {

public:

	struct Block {
		bool is_sorted;

		std::weak_ptr<BlockData> lib_block;
		int id;
		std::string full_name;
		struct Pos { int x; int y; } pos;

		std::vector<std::variant<std::monostate, bool, int64_t, double, std::string>> parameters;


		std::string GetFullName(){
			auto block_data = lib_block.lock();
			std::string n;
			if(!block_data) 
				n = full_name;
			else 
				n = block_data->FullName();
			return n;
		}

		bool IsValid(){
			if(id <= 0) return false;
			if(!lib_block.lock()) return false;
			if(full_name.empty()) return false;
			return true;
		}
	};

	struct Connection {
		int id;
		std::weak_ptr<Block> src;
		std::weak_ptr<Block> dst;
		int src_pin;
		int dst_pin;

		Connection(int _id, std::weak_ptr<Block> _src, int _src_pin, std::weak_ptr<Block> _dst, int _dst_pin)
			:id(_id), src(_src), src_pin(_src_pin), dst(_dst), dst_pin(_dst_pin){}


		bool IsValid(){
			// test if pin ids are valid number
			if(src_pin < 0) return false;
			if(dst_pin < 0) return false;

			// test if connection is created between valid blocks
			if (src.expired()) return false;
			if (dst.expired()) return false;

			auto src_ptr = src.lock();
			auto dst_ptr = dst.lock();

			if (!src_ptr) return false; // this migth be redundant
			if (!dst_ptr) return false; // this migth be redundant

			// test if source and dest blocks are valid
			if(!src_ptr->IsValid()) return false;
			if(!dst_ptr->IsValid()) return false;

			// test if connection is created between valid inputs/outputs
			auto src_data_ptr = src_ptr->lib_block.lock();
			auto dst_data_ptr = dst_ptr->lib_block.lock();

			if(src_data_ptr->Outputs().size() < src_pin) return false;
			if(dst_data_ptr->Inputs().size() < dst_pin) return false;

			// test if src and dst have same data type
			if(src_data_ptr->Outputs()[src_pin].type != dst_data_ptr->Inputs()[dst_pin].type) return false;
			
			return true; // return true if passed every test
		}
	};


private:
	struct ConnectionRaw {
		int src;
		int src_pin;
		int dst;
		int dst_pin;

		ConnectionRaw() :src(-1), src_pin(-1), dst(-1), dst_pin(-1){}
		ConnectionRaw(int _src, int _src_pin, int _dst, int _dst_pin) 
			:src(_src), src_pin(_src_pin), dst(_dst), dst_pin(_dst_pin) {}

		bool IsValid() {
			if (src == dst) return false;
			if (src < 0 || src_pin < 0 || dst < 0 || dst_pin < 0) return false;
			return true;
		}
	};


	int next_block_id;
	int next_connection_id;


public:

	std::list<std::shared_ptr<Block>> blocks;
	std::list<Connection> connetions;
	std::filesystem::path path;
	
	
	Schematic(){
		next_block_id = 1;
		next_connection_id = 1;
	}

	std::list<std::shared_ptr<Block>> Blocks(){return blocks;};
	std::list<Connection> Connetions(){return connetions;};
	std::filesystem::path Path(){return path;};

	enum class Error {
		OK,

		CANNOT_OPEN_FILE,
		CANNOT_SAVE_FILE,
		PATH_EMPTY,
		JSON_PARSING_ERROR,
		JSON_NOT_AN_OBJECT,

		JSON_MISSING_BLOCKS_FIELD,
		JSON_BLOCKS_FIELD_NOT_ARRAY,
		JSON_CONNECTIONS_FIELD_NOT_ARRAY,
		JSON_MISSING_CONNECTIONS_FIELD,

		JSON_BLOCK_NAME_NOT_STRING,
		JSON_BLOCK_MISSING_NAME,
		JSON_BLOCK_NOT_AN_OBJECT,
		JSON_BLOCK_MISSING_ID,
		JSON_BLOCK_INVALID_ID,
		JSON_BLOCK_MISSING_POS,
		JSON_BLOCK_POS_NOT_ARRAY,
		JSON_BLOCK_INVALID_POS,
		JSON_BLOCK_MISSING_SRC,
		JSON_BLOCK_POS_NOT_STRING,
		JSON_BLOCK_MISSING_LOC,
		JSON_BLOCK_LOC_NOT_STRING,
		JSON_BLOCK_INVALID_LOC,
		JSON_BLOCK_PARAMETER_INVALID_TYPE,
		JSON_BLOCK_PARAMS_NOT_ARRAY,

		JSON_CONNECTION_NOT_AN_OBJECT,
		JSON_CONNECTION_IS_INVALID,
		JSON_CONNECTION_INVALID_SRC,
		JSON_CONNECTION_INVALID_SRCPIN,
		JSON_CONNECTION_INVALID_DST,
		JSON_CONNECTION_INVALID_DSTPIN,

	};


private:

	bool IsConnectedToAlreadyConnectedPin(const Connection* conn1){
		if(!conn1) return true;

		auto dst1 = conn1->dst.lock();
		if(!dst1) return false; // not connected to anything

		for(auto iter = connetions.begin(); iter != connetions.end(); /*none*/){
			const Connection* conn2 = &(*iter);

			if(conn2 == conn1){
				iter ++;
				continue; // skip same connection
			} 


			auto dst2 = conn2->dst.lock();

			if(dst1 != dst2){
				iter++;
				continue; // not connected to same block
			}

			if(conn1->dst_pin != conn2->dst_pin){
				iter++;
				continue; // not connected to same pin
			} 

			return true; // connected to same block and pin
		}

		return false;

	}


	std::vector<Connection> GetAllBlockInputConnections(std::shared_ptr<Block>);

	

public:


	void SortBlocks();


	bool CreateConnection(int src_id, int src_pin, int dst_id, int dst_pin){

		// find src and dst block;
		std::shared_ptr<Block> src = nullptr;
		std::shared_ptr<Block> dst = nullptr;
		
		for(auto b: blocks){
			if(b->id == src_id) src = b;
			if(b->id == dst_id) dst = b;
		};
		
		Connection conn(next_connection_id++, src, src_pin, dst, dst_pin);
		if (conn.IsValid() && !IsConnectedToAlreadyConnectedPin(&conn)) {
			connetions.push_back(conn);
			return true;
		}
		else { 
			return false; 
		}

	}


	std::shared_ptr<Block> CreateBlock(std::shared_ptr<BlockData> block_data, int x, int y){
		if(block_data == nullptr) return 0;

		Block b;
		b.id = next_block_id++;
		b.lib_block = block_data;
		b.full_name = block_data->FullName();
		b.pos.x = x;
		b.pos.y = y;

		auto block_ptr = std::make_shared<Block>(b);
	
		blocks.push_back(block_ptr);

		return block_ptr;
	}



	void LinkWithLibrary(Librarian* library){
		if(!library) return;

		// for(auto& block_ptr: blocks){
		// 	for(const auto block_data_ptr: *library){
		// 		if(block_ptr->name == block_data_ptr->Name()){
		// 			block_ptr->lib_block = block_data_ptr;
		// 		}
		// 	}
		// }

		for(auto& block_ptr: blocks){
			auto block_data = library->FindBlock(block_ptr->full_name);
			block_ptr->lib_block = block_data;
		}

		RemoveInvalidElements();

	}



	void RemoveInvalidElements(){

		// remove invalid blocks
		blocks.remove_if(
			[](std::shared_ptr<Block>& block)
			{
				if(!block) return true;
				if(!block->IsValid()) return true;
				return false;
			} 
			);

		// remove invalid connections
		connetions.remove_if(
			[](Connection& conn)
			{
				return !conn.IsValid();
			}
		);

		// remove multiple connections connected to single input
		for(auto iter = connetions.begin(); iter != connetions.end(); /*none*/){
			Connection& conn = *iter;
			if(IsConnectedToAlreadyConnectedPin(&conn)){
				iter = connetions.erase(iter);
			}else{
				iter++;
			}

		}

	}



	static const char* ErrorToStr(Error err);


	Error Save(const std::filesystem::path& _path){
		if(_path.empty()) return Error::PATH_EMPTY;
		path = _path;
		return Save();
	}


	Error Save(){
		if (path.empty()) return Error::PATH_EMPTY;

		std::string data;
		Error err;

		err = Serialize(&data);
		if(err != Error::OK) return err;

		err = SaveFile(path, data);
		return err;
	}


	std::string BuildToCPP();


	Error Read(const std::filesystem::path& _path);

private:

	bool CodeExtractSection(const std::string& code, std::string* result, const std::string& marker);

    Error SaveFile(const std::filesystem::path& _path, const std::string& data);
    Error LoadFile(const std::filesystem::path& path, std::string* result);

	Error ParseJson(const std::string& data_str);
	Error ParseJsonBlock(const boost::json::value& js, Block* block);
	Error ParseJsonConnection(const boost::json::value& js, ConnectionRaw* conn);

	Error Serialize(std::string* data) {

		boost::json::array js_blocks;

		for (const auto& block_ptr : blocks) {
			if (!block_ptr) continue;

			boost::json::object js_block;
			js_block["id"] = block_ptr->id;
			js_block["pos"] = boost::json::array({block_ptr->pos.x, block_ptr->pos.y });
			js_block["name"] = block_ptr->GetFullName();
			if(block_ptr->parameters.size()){

				boost::json::array js_param_arr;
				for(auto& p: block_ptr->parameters){
					// std::monostate, bool, int64_t, double, std::string
					if(std::holds_alternative<bool>(p))
						js_param_arr.push_back(std::get<bool>(p));
					else if(std::holds_alternative<int64_t>(p))
						js_param_arr.push_back(std::get<int64_t>(p));
					else if(std::holds_alternative<double>(p))
						js_param_arr.push_back(std::get<double>(p));
					else if(std::holds_alternative<std::string>(p))
						js_param_arr.push_back(boost::json::string(std::get<std::string>(p)));
					else
						js_param_arr.push_back(nullptr);	
				}

				js_block["params"] = js_param_arr;
			}

			js_blocks.push_back(js_block);
		}

		boost::json::array js_connections;

		for (const auto& conn : connetions) {

			auto src = conn.src.lock();
			auto dst = conn.dst.lock();
			if (!src || !dst) continue;

			boost::json::object js_conn;
			js_conn["src"] = src->id;
			js_conn["src_pin"] = conn.src_pin;
			js_conn["dst"] = dst->id;
			js_conn["dst_pin"] = conn.dst_pin;

			js_connections.push_back(js_conn);
		}

		boost::json::object js;

		js["blocks"] = js_blocks;
		js["connections"] = js_connections;

		*data = boost::json::serialize(js);

		return Error::OK;
	}

};













