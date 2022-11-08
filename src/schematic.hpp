#pragma once

#include <list>
#include <memory>
#include <string>
#include <fstream>
#include <boost/json.hpp>
#include <filesystem>

#include "schematic_block.hpp"



class Schematic {


	struct Block {
		std::weak_ptr<BlockData> lib_block;
		int id;
		std::string name;
		enum class Location{PROJECT, STD, EXTERNAL} location;
		struct Pos { int x; int y; } pos;
	};

	struct Connection {
		std::weak_ptr<Block> src;
		std::weak_ptr<Block> dst;
		int src_pin;
		int dst_pin;

		Connection(std::weak_ptr<Block> _src, int _src_pin, std::weak_ptr<Block> _dst, int _dst_pin)
			:src(_src), src_pin(_src_pin), dst(_dst), dst_pin(_dst_pin){}

		// returns true if connection is valid
		bool verify() {
			if (src.expired()) return false;
			if (dst.expired()) return false;
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


	std::list<std::shared_ptr<Block>> blocks;
	std::list<Connection> connetions;
	std::filesystem::path path;
public:

	std::list<std::shared_ptr<Block>> Blocks(){return blocks;};
	std::list<Connection> Connetions(){return connetions;};
	std::filesystem::path Path(){return path;};

	enum class Error {
		OK,

		CANNOT_OPEN_FILE,
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

	void LinkWithLibrary(std::list<std::shared_ptr<BlockData>>* library){
		if(!library) return;

		for(auto& block_ptr: blocks){
			for(const auto block_data_ptr: *library){
				if(block_ptr->name == block_data_ptr->Name()){
					block_ptr->lib_block = block_data_ptr;
				}
			}
		}

	}


	static const char* ErrorToStr(Error err);

	Error LoadFromJsonFile(std::string path);

private:
	Error LoadFile(const std::filesystem::path& path, std::string* result);
	Error ParseJson(const std::string& data_str);
	Error ParseJsonBlock(const boost::json::value& js, Block* block);
	Error ParseJsonConnection(const boost::json::value& js, ConnectionRaw* conn);

};













