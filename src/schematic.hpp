#pragma once

#include <list>
#include <memory>
#include <string>
#include <fstream>
#include <boost/json.hpp>



class Schematic {


	struct Block {
		int id;
		std::string src;
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


public:

	enum class Error {
		OK,

		CANNOT_OPEN_FILE,
		JSON_PARSING_ERROR,
		JSON_NOT_AN_OBJECT,
		JSON_MISSING_BLOCKS_FIELD,
		JSON_BLOCKS_FIELD_NOT_ARRAY,

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

	static const char* ErrorToStr(Error err) {
		switch (err) {
		case Error::OK: return "OK";
		case Error::CANNOT_OPEN_FILE: return "CANNOT_OPEN_FILE";
		case Error::JSON_PARSING_ERROR: return "JSON_PARSING_ERROR";
		case Error::JSON_NOT_AN_OBJECT: return "JSON_NOT_AN_OBJECT";
		case Error::JSON_MISSING_BLOCKS_FIELD: return "JSON_MISSING_BLOCKS_FIELD";
		case Error::JSON_BLOCKS_FIELD_NOT_ARRAY: return "JSON_BLOCKS_FIELD_NOT_ARRAY";
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

	Error LoadFromJsonFile(std::string path);

private:
	Error LoadFile(std::string path, std::string* result);
	Error ParseJson(const boost::json::value& js, std::vector<Block>* blocks_raw, std::vector<ConnectionRaw>* connection_raw_list);
	Error ParseJsonBlock(const boost::json::value& js, Block* block);
	Error ParseJsonConnection(const boost::json::value& js, ConnectionRaw* conn);




};













