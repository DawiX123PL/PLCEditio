
//////****** begin includes ******//////

//////****** end includes ******//////
class output_block{ 
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
		PLC::IOmoduleData data  = PLC::GetIO(0);
		if(*input0){
			data.output |= (1<<parameter0);
		}else{
			data.output &= ~(1<<parameter0);
		}
		PLC::SetIO(data,0);

//////****** end update ******//////
    }
};
