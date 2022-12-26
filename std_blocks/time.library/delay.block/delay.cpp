
//////****** begin includes ******//////
#include <chrono>


//////****** end includes ******//////
class delay_block{ 
public: 
    bool* input0;
    bool* input1;
    bool  parameter0;
    int64_t  parameter1;
    bool  parameter2;
    bool  parameter3;
    bool  output0;

//////****** begin functions ******//////
	std::chrono::high_resolution_clock::time_point past;
	
	bool previous_in;

//////****** end functions ******//////

    void init(){
//////****** begin init ******//////

//////****** end init ******//////
    }

    void update(){
//////****** begin update ******//////
		
		bool is_enabled = parameter0 || (*input0);

		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		std::chrono::seconds delay = std::chrono::seconds(0);

		// detect edge 
		if(previous_in != *input1){

			previous_in = *input1;
			past = now;

			delay = std::chrono::seconds(0);
		}


		// rising edge
		if(*input1 && parameter2 && is_enabled){
			delay = std::chrono::seconds(parameter1);
		}

		// falling edge
		if(!(*input1) && parameter3 && is_enabled){
			delay = std::chrono::seconds(parameter1);
		}

		if(now > past + delay){
			output0 = *input1;
		}
		

//////****** end update ******//////
    }
};
