#pragma once

#include "tcp_client.hpp"
#include "thread.hpp"
#include <chrono>


class CodeUploader: public Thread{

    PLCclient* plc_client;
    
    std::string code;
    std::string config;

    static constexpr std::chrono::duration timeout_duration = std::chrono::seconds(5);

public:
    enum class Status{
        _NONE,
        _WAIT,
        _OK,
        _ERROR,
        _TIMEOUT,
        _DISCONNECTED,
    };

    CodeUploader(PLCclient* c): plc_client(c){}

    void UploadAndBuild(std::string _code, std::string _config){
        if(IsRunning()) return;
        code = _code;
        config = _config;
        Start();
    }

    struct CompilationResult{
        int64_t exit_code;
        std::string file;
        std::string error;
        CompilationResult(): exit_code(0),file(),error(){};
        CompilationResult(int64_t _exit_code, std::string _file, std::string _error): exit_code(_exit_code),file(_file),error(_error){};
    };


private:
    
    std::mutex flag_msg_mutex;
    Status app_stop_flag = Status::_NONE;
    Status code_upload_flag = Status::_NONE;
    Status config_upload_flag = Status::_NONE;
    Status code_compilation_flag = Status::_NONE;

    std::string app_stop_msg;
    std::string code_upload_msg;
    std::string config_upload_msg;
    std::string code_compilation_msg;


    void SetFlag(Status* flag,const Status& status){
        std::scoped_lock(flag_msg_mutex);
        *flag = status;
    }

    void SetResponseMsg(std::string* msg, const std::string& response_message ){
        std::scoped_lock(flag_msg_mutex);
        *msg = response_message;
    }

    
    std::mutex compilation_result_mutex;
    std::vector<CompilationResult> compilation_result;

public:

    Status GetFlagStopApp()        { std::scoped_lock lock(flag_msg_mutex); return app_stop_flag;}
    Status GetFlagCodeUpload()     { std::scoped_lock lock(flag_msg_mutex); return code_upload_flag;}
    Status GetFlagConfigUpload()   { std::scoped_lock lock(flag_msg_mutex); return config_upload_flag;}
    Status GetFlagCodeCompilation(){ std::scoped_lock lock(flag_msg_mutex); return code_compilation_flag;}

    std::string GetMsgAppStop()        { std::scoped_lock lock(flag_msg_mutex); return app_stop_msg;}
    std::string GetMsgCodeUpload()     { std::scoped_lock lock(flag_msg_mutex); return code_upload_msg;}
    std::string GetMsgConfigUpload()   { std::scoped_lock lock(flag_msg_mutex); return config_upload_msg;}
    std::string GetMsgCodeCompilation(){ std::scoped_lock lock(flag_msg_mutex); return code_compilation_msg;}

    void ClearFlags(){
        if(IsRunning()) return;

        std::scoped_lock lock(flag_msg_mutex);
        app_stop_flag = Status::_NONE;
        code_upload_flag = Status::_NONE;
        config_upload_flag = Status::_NONE;
        code_compilation_flag = Status::_NONE;
        app_stop_msg = "";
        code_upload_msg = "";
        config_upload_msg = "";
        code_compilation_msg = "";
    }

    std::vector<CompilationResult> GetCompilationResult(){
        std::scoped_lock lock(compilation_result_mutex);
        return compilation_result;
    }


private:


    void threadJob() override{

        // step 0, reset status flags
        SetFlag(&app_stop_flag, Status::_NONE);
        SetFlag(&code_upload_flag, Status::_NONE);
        SetFlag(&config_upload_flag, Status::_NONE);
        SetFlag(&code_compilation_flag, Status::_NONE);
        SetResponseMsg(&app_stop_msg, "");
        SetResponseMsg(&code_upload_msg, "");
        SetResponseMsg(&config_upload_msg, "");
        SetResponseMsg(&code_compilation_msg, "");

        { // step 1, stop currently running application
            if(!plc_client->IsConnected()){
                SetFlag(&app_stop_flag, Status::_DISCONNECTED);
                return;    
            }

            plc_client->AppStop();
            SetFlag(&app_stop_flag, Status::_WAIT);
            
            // wait until received response
            PLCclient::AppStopResponse response;
            auto start_time = std::chrono::high_resolution_clock::now();
            
            while(!plc_client->GetIfAppStopResponse(&response)){
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                auto now = std::chrono::high_resolution_clock::now();
                if(now > (start_time + timeout_duration)){
                    SetFlag(&app_stop_flag, Status::_TIMEOUT);
                    return;
                }
            }
            
            if(response.status == PLCclient::AppStopResponse::Status::_ERR){
                // stop thread on error
                SetFlag(&app_stop_flag, Status::_ERROR);
                SetResponseMsg(&app_stop_msg, response.msg);
                return;
            }else{
                SetFlag(&app_stop_flag, Status::_OK);
                SetResponseMsg(&app_stop_msg, response.msg);
            }
        }

        
        { // step 2, upload code file
            if(!plc_client->IsConnected()){
                SetFlag(&code_upload_flag, Status::_DISCONNECTED);
                return;    
            }

            plc_client->FileWriteStr(code, "file1.cpp");
            SetFlag(&code_upload_flag, Status::_WAIT);
            
            // wait until received response
            PLCclient::FileWriteResponse response;
            auto start_time = std::chrono::high_resolution_clock::now();
            
            while(!plc_client->GetIfFileWriteResponse(&response)){
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                auto now = std::chrono::high_resolution_clock::now();
                if(now > (start_time + timeout_duration)){
                    SetFlag(&code_upload_flag, Status::_TIMEOUT);
                    return;
                }
            }
            
            if(response.status == PLCclient::FileWriteResponse::Status::_ERR){
                // stop thread on error
                SetFlag(&code_upload_flag, Status::_ERROR);
                SetResponseMsg(&code_upload_msg, response.msg);
                return;
            }else{
                SetFlag(&code_upload_flag, Status::_OK);
                SetResponseMsg(&code_upload_msg, response.msg);
            }

        }


        { // step 3, upload config file
            if(!plc_client->IsConnected()){
                SetFlag(&config_upload_flag, Status::_DISCONNECTED);
                return;    
            }
            
            plc_client->FileWriteStr(config, "build.conf");
            SetFlag(&config_upload_flag, Status::_WAIT);
            
            // wait until received response
            PLCclient::FileWriteResponse response;
            auto start_time = std::chrono::high_resolution_clock::now();

            while(!plc_client->GetIfFileWriteResponse(&response)){
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                auto now = std::chrono::high_resolution_clock::now();
                if(now > (start_time + timeout_duration)){
                    SetFlag(&config_upload_flag, Status::_TIMEOUT);
                    return;
                }
            }
            
            if(response.status == PLCclient::FileWriteResponse::Status::_ERR){
                // stop thread on error
                SetFlag(&config_upload_flag, Status::_ERROR);
                SetResponseMsg(&config_upload_msg, response.msg);
                return;
            }else{
                SetFlag(&config_upload_flag, Status::_OK);
                SetResponseMsg(&config_upload_msg, response.msg);
            }

        }

        { // step 4, compile code
            if(!plc_client->IsConnected()){
                SetFlag(&code_compilation_flag, Status::_DISCONNECTED);
                return;    
            }

            plc_client->CompileCode();
            SetFlag(&code_compilation_flag, Status::_WAIT);

            // wait until received response
            PLCclient::AppBuildResponse response;
            auto start_time = std::chrono::high_resolution_clock::now();

            while(!plc_client->GetIfCompileCodeeResponse(&response)){
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                auto now = std::chrono::high_resolution_clock::now();
                if(now > (start_time + timeout_duration)){
                    SetFlag(&code_compilation_flag, Status::_TIMEOUT);
                    return;
                }
            }

            if(response.status == PLCclient::AppBuildResponse::Status::_ERR){
                // stop thread on error
                SetFlag(&code_compilation_flag, Status::_ERROR);
                SetResponseMsg(&code_compilation_msg, "");
                return;
            }else{
                SetFlag(&code_compilation_flag, Status::_OK);
                SetResponseMsg(&code_compilation_msg, "");

                {
                    std::scoped_lock lock(compilation_result_mutex);
                    compilation_result.clear();
                    for(auto& err :response.compilation_errors)
                        compilation_result.emplace_back(err.exit_code, err.file, err.error);
                }
            }

        }
        

    };

};
