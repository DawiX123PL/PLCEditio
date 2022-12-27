
//////****** begin includes ******//////
#include <map>


#ifndef PLC_LOCAL_MEMORY_OBJECT
#define PLC_LOCAL_MEMORY_OBJECT

std::map<int64_t, bool> plc_local_memory_object;

#endif


//////****** end includes ******//////
class mem_write_block{ 
public: 
    const bool* input0;
    int64_t  parameter0;

//////****** begin functions ******//////

//////****** end functions ******//////

    void init(){
//////****** begin init ******//////

//////****** end init ******//////
    }

    void update(){
//////****** begin update ******//////
		if(input0)
			plc_local_memory_object[parameter0] = *input0;
//////****** end update ******//////
    }
};
