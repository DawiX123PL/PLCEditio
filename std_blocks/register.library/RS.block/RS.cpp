
//////****** begin includes ******//////

//////****** end includes ******//////
class RS_block{ 
public: 
    bool* input0;
    bool* input1;
    bool  output0;
    bool  output1;

//////****** begin functions ******//////

//////****** end functions ******//////

    void init(){
//////****** begin init ******//////

//////****** end init ******//////
    }

    void update(){
//////****** begin update ******//////

        bool set = input0 ? (*input0) : false;
        bool reset = input1 ? (*input1) : false;

		// Set
		if(set) output0 = true;

		// Reset
		if(reset) output0 = false;

		output1 = !output0;
//////****** end update ******//////
    }
};
