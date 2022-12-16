
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

//////****** end init ******//////
    }

    void update(){
//////****** begin update ******//////
		output0 = *input0 || *input1;
		output1 = !output0;
//////****** end update ******//////
    }
};
