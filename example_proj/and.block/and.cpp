
//////****** begin includes ******//////
#include <string>
//////****** end includes ******//////
namespace LOCAL{ 
 
    class and_block{ 
    public: 
        bool* input0;
        bool* input1;
        bool  output0;
        bool  output1;

//////****** begin functions ******//////
        void Init(){

        }

        void Update(){
			output0 = *input0 && *input1;
			output1 = !output1;
        }
//////****** end functions ******//////
    };
};
