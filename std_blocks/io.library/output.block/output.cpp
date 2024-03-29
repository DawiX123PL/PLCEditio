
//////****** begin includes ******//////

//////****** end includes ******//////
class output_block{ 
public: 
    const bool* input0;
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

		PLC::IOmoduleData data = PLC::GetIO();

		// change output only if block is connected
		if(input0){

			if(*input0){
				data.output |= (1<<parameter0);
			}else{
				data.output &= ~(1<<parameter0);
			}

		}

		output0 = (data.output & (1<<parameter0)) != 0;

		PLC::SetIO(data);
//////****** end update ******//////
    }
};
