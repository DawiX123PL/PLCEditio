
//////****** begin includes ******//////
#include <map>


#ifndef PLC_LOCAL_MEMORY_OBJECT
#define PLC_LOCAL_MEMORY_OBJECT

std::map<int64_t, bool> plc_local_memory_object;

#endif




//////****** end includes ******//////
class mem_read_block{ 
public: 
    int64_t  parameter0;
    bool  output0;

//////****** begin functions ******//////

//////****** end functions ******//////

    void init(){
//////****** begin init ******//////

//////****** end init ******//////
    }

    void update(){
//////****** begin update ******//////
		auto iter = plc_local_memory_object.find(parameter0);

		if(iter != plc_local_memory_object.end()){
			// return memory value
			output0 = iter->second;
		}else{
			// if memory not assigned return false
			output0 = false;
		}
//////****** end update ******//////
    }
};
