
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
        bool data = input0 ? (*input0) : false;
        bool clk = input1 ? (*input1) : false;

		if(old_clk != clk) output0 = data;

		output1 = !output0;
		old_clk = clk;
//////****** end update ******//////
    }
};
