
//////****** begin includes ******//////

//////****** end includes ******//////
class not_block{ 
public: 
    bool* input0;
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
        bool i0 = input0 ? (*input0) : false;
		output0 = ! i0;
//////****** end update ******//////
    }
};
