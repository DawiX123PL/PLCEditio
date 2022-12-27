
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
		
		// enable flags
		bool en_i = input0 ? (*input0) : false;
		bool en_p = parameter0;
		bool en = en_i || en_p;
		
		bool in = input1 ? (*input1) : false;

		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		std::chrono::seconds delay = std::chrono::seconds(0);

		// detect edge 
		if(previous_in != *input1){

			previous_in = *input1;
			past = now;

			delay = std::chrono::seconds(0);
		}


		// rising edge
		if(in && parameter2 && en){
			delay = std::chrono::seconds(parameter1);
		}

		// falling edge
		if(!in && parameter3 && en){
			delay = std::chrono::seconds(parameter1);
		}

		if(now > past + delay){
			output0 = in;
		}
		

//////****** end update ******//////
    }
};
