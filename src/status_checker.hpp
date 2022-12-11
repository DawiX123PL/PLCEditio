#pragma once

#include <mutex>
#include "thread.hpp"
#include "tcp_client.hpp"

class StatusChecker: public Thread{
public:

    enum class AppStatus{
        _DISCONNECTED,
        _UNNOWN,
        _TIMEOUT,
        _RUNNING,
        _STOPPED,
    };

    StatusChecker(PLCclient* client):app_status(AppStatus::_UNNOWN), plc_client(client){
        Start();
    }

    ~StatusChecker(){
        Join();
    }

private:

    std::mutex mutex;
    AppStatus app_status;
    PLCclient* plc_client;


    static constexpr std::chrono::duration delay_time = std::chrono::milliseconds(300);
    static constexpr std::chrono::duration response_timeout = std::chrono::milliseconds(500);


    AppStatus GetStatusFromServer(){

        if(!plc_client->IsConnected()){ 
            return AppStatus::_DISCONNECTED;
        }

        plc_client->CheckAppStatus();

        PLCclient::AppStatusResponse response;
        auto start = std::chrono::high_resolution_clock::now();
        while(!plc_client->GetIfAppStatusResponse(&response)){
            auto now = std::chrono::high_resolution_clock::now();

            if(now > start + response_timeout){
                return AppStatus::_TIMEOUT;
            }
        }
        
        switch(response.status){
        case PLCclient::AppStatusResponse::Status::_UNNOWN : return AppStatus::_UNNOWN;
        case PLCclient::AppStatusResponse::Status::_RUNNING : return AppStatus::_RUNNING;
        case PLCclient::AppStatusResponse::Status::_STOPPED : return AppStatus::_STOPPED;
        }


        return AppStatus::_UNNOWN;
        
    }

    void threadJob(){
        while(IsRun()){
            std::this_thread::sleep_for(delay_time);
            AppStatus status = GetStatusFromServer();

            {
                std::scoped_lock lock(mutex);
                app_status = status;
            }

        }
    }


public:

    AppStatus GetAppStatus(){
        std::scoped_lock lock(mutex);
        return app_status;
    }

};




