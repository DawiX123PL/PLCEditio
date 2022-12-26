
//////****** begin includes ******//////

//////****** end includes ******//////
class D_block{ 
public: 
    bool* input0;
    bool* input1;
    bool  output0;
    bool  output1;

//////****** begin functions ******//////
	bool old_clk;
//////****** end functions ******//////

    void init(){
//////****** begin init ******//////
	
//////****** end init ******//////
    }

    void update(){
//////****** begin update ******//////
		if(old_clk != (*input1)) output0 = (*input0);

		output1 = !output0;
		old_clk = (*input1);
//////****** end update ******//////
    }
};
