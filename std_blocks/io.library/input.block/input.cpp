
//////****** begin includes ******//////

//////****** end includes ******//////
class input_block{ 
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
		PLC::IOmoduleData data  = PLC::GetIO();
		output0 = data.input & (1<<parameter0);
//////****** end update ******//////
    }
};
