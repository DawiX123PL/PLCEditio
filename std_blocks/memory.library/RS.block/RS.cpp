
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
		// Set
		if(*input0) output0 = true;

		// Reset
		if(*input1) output0 = false;

		output1 = !output0;
//////****** end update ******//////
    }
};
