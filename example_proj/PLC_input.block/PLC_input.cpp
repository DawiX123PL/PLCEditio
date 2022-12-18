
//////****** begin includes ******//////

//////****** end includes ******//////
class PLC_input_block{ 
public: 
    int64_t  parameter0;
    bool  output0;

//////****** begin functions ******//////

//////****** end functions ******//////

    void init(){
//////****** begin init ******//////
		output0 = false;
//////****** end init ******//////
    }

    void update(){
//////****** begin update ******//////
		PLC::IOmoduleData data = PLC::GetIO(0);
		output0 = data.input & (1<<parameter0);
//////****** end update ******//////
    }
};
