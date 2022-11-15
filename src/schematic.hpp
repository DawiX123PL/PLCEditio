#pragma once

#include <list>
#include <memory>
#include <string>
#include <fstream>
#include <boost/json.hpp>
#include <filesystem>

#include "schematic_block.hpp"
#include "librarian.hpp"


class Schematic {


	struct Block {
		std::weak_ptr<BlockData> lib_block;
		int id;
		std::string full_name;
		struct Pos { int x; int y; } pos;

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

		// returns true if connection is valid
		bool verify() {
			if (src.expired()) return false;
			if (dst.expired()) return false;
			return true;
		}

		bool IsValid(){
			// test if pin ids are valid number
			if(src_pin < 0) return false;
			if(dst_pin < 0) return false;

			// test if connection is created between valid blocks
			if (src.expired()) return false;
			if (dst.expired()) return false;

			auto src_ptr = src.lock();
			auto dst_ptr = dst.lock();

			if (src_ptr) return false; // this migth be redundant
			if (dst_ptr) return false; // this migth be redundant

			// test if source and dest blocks are valid
			if(!src_ptr->IsValid()) return false;
			if(!dst_ptr->IsValid()) return false;

			// test if connection is created between valid inputs/outputs
			auto src_data_ptr = src_ptr->lib_block.lock();
			auto dst_data_ptr = dst_ptr->lib_block.lock();

			if(src_data_ptr->Outputs().size() < src_pin) return false;
			if(dst_data_ptr->Inputs().size() < dst_pin) return false;

			return true;
		}
	};

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

		JSON_CONNECTION_NOT_AN_OBJECT,
		JSON_CONNECTION_IS_INVALID,
		JSON_CONNECTION_INVALID_SRC,
		JSON_CONNECTION_INVALID_SRCPIN,
		JSON_CONNECTION_INVALID_DST,
		JSON_CONNECTION_INVALID_DSTPIN,

	};

	bool CreateConnection(int src_id, int src_pin, int dst_id, int dst_pin){

		// find src and dst block;
		std::shared_ptr<Block> src = nullptr;
		std::shared_ptr<Block> dst = nullptr;
		
		for(auto b: blocks){
			if(b->id == src_id) src = b;
			if(b->id == dst_id) dst = b;
		};
		
		Connection conn(next_connection_id++, src, src_pin, dst, dst_pin);
		if (conn.verify()) {
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
		b.full_name = block_data->Name();
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

	}



	void RemoveInvalidElements(){

		blocks.remove_if(
			[](std::shared_ptr<Block>& block)
			{
				if(!block) return true;
				if(!block->IsValid()) return true;
				return false;
			} 
			);

		connetions.remove_if(
			[](Connection& conn)
			{
				return conn.IsValid();
			}
		);

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





	Error Read(const std::filesystem::path& _path);

private:
    Error SaveFile(const std::filesystem::path& _path, const std::string& data);
    Error LoadFile(const std::filesystem::path& path, std::string* result);

	Error ParseJson(const std::string& data_str);
	Error ParseJsonBlock(const boost::json::value& js, Block* block);
	Error ParseJsonConnection(const boost::json::value& js, ConnectionRaw* conn);

	Error Serialize(std::string* data) {

		boost::json::array js_blocks;

		for (const auto& block_ptr : blocks) {
			if (!block_ptr) continue;

			boost::json::value js_block = {
				{"id", block_ptr->id},
				{"pos", boost::json::array({block_ptr->pos.x, block_ptr->pos.y })},
				{"name", block_ptr->GetFullName()},
			};

			js_blocks.push_back(js_block);
		}

		boost::json::array js_connections;

		for (const auto& conn : connetions) {

			int src_id = -1;
			int dst_id = -1;

			auto src = conn.src.lock();
			auto dst = conn.dst.lock();
			if (!src || !dst) continue;

			src_id = src->id;
			dst_id = dst->id;


			boost::json::value js_conn = {
				{"src",     src_id},
				{"src_pin", conn.src_pin},
				{"dst",     dst_id},
				{"dst_pin", conn.dst_pin},
			};

			js_connections.push_back(js_conn);
		}

		boost::json::object js;

		js["blocks"] = js_blocks;
		js["connections"] = js_connections;

		*data = boost::json::serialize(js);

		return Error::OK;
	}

};













