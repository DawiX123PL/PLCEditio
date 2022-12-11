
//////****** begin includes ******//////

//////****** end includes ******//////
class PLC_output_block{ 
public: 
    bool* input0;
    int64_t  parameter0;

//////****** begin functions ******//////

//////****** end functions ******//////

    void init(){
//////****** begin init ******//////

//////****** end init ******//////
    }

    void update(){
//////****** begin update ******//////
		
		
		PLC::IOmoduleData data = PLC::GetIO();

		if(*input0)
			data.output = 0xffffffff;
		else
			data.output = 0x00000000;

		PLC::SetIO(data);

//////****** end update ******//////
    }
};
