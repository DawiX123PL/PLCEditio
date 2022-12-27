
//////****** begin includes ******//////
#include <chrono>
#include <iostream>

//////****** end includes ******//////
class clock_block{ 
public: 
    bool* input0;
    bool  parameter0;
    int64_t  parameter1;
    bool  output0;
    bool  output1;

//////****** begin functions ******//////
	
    std::chrono::milliseconds period;
    std::chrono::milliseconds half;
	std::chrono::high_resolution_clock::time_point past;

	bool enabled_old = false;
//////****** end functions ******//////

    void init(){
//////****** begin init ******//////
		period = std::chrono::milliseconds(parameter1 * 1000);
		half = std::chrono::milliseconds(parameter1 * 1000/2);
		past = std::chrono::high_resolution_clock::now();
//////****** end init ******//////
    }

    void update(){
//////****** begin update ******//////

		// enable flags
		bool en_i = input0 ? (*input0) : false;
		bool en_p = parameter0;
		bool en = en_i || en_p;
		
		// check if block is enabled. if not return immediately
		if(!en){
			enabled_old = false;
			output0 = false;
			output1 = false;
			return;
		}

		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

		// check if timer has been enabled
		if(!enabled_old && en){
			enabled_old = true;
			past = now;
		}

		output0 = false;

		if(now < past + half){
			output1 = true;
		}else{
			output1 = false;
		}

		if(now >= past + period){
			output0 = true;
			output1 = true;
			past = past + period;
		}

		std::cout << (now-past+period) / std::chrono::milliseconds(1) << " \t " << (now-past) / std::chrono::milliseconds(1) <<"\n";
//////****** end update ******//////
    }
};
