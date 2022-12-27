
//////****** begin includes ******//////

//////****** end includes ******//////
class or_block{ 
public: 
    bool* input0;
    bool* input1;
    bool  output0;
    bool  output1;

//////****** begin functions ******//////

//////****** end functions ******//////

    void init(){
//////****** begin init ******//////
        output0 = false;
        output1 = true;
//////****** end init ******//////
    }

    void update(){
//////****** begin update ******//////
        
        // check if inputs are != null
        bool i0 = input0 ? (*input0) : false;
        bool i1 = input1 ? (*input1) : false;

		output0 = i0 || i1;
		output1 = !output0;
//////****** end update ******//////
    }
};
