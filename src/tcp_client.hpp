// THIS File is based on code from examples from:
// https://www.boost.org/doc/libs/1_81_0/doc/html/boost_asio/examples.html

#pragma once


#include <boost/asio.hpp>
#include <iostream>
#include <queue>
#include <boost/json.hpp>
#include <chrono>
#include <functional>
#include "thread.hpp"
#include "debug_console.hpp"




class TCPclient: public Thread{

public:
    enum class Status{ DISCONNECTED, CONNECTED, CONNECTING};


private:
    std::recursive_mutex tcp_mutex;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket;
    boost::asio::ip::tcp::endpoint endpoint;

    static constexpr size_t read_buffer_size = 1024;
    uint8_t read_buffer[read_buffer_size];

    Status status;


public:

    struct IPaddress {
        //  ip1.ip2.ip3.ip4:port
        uint8_t addr[4];
        uint16_t port; 
        IPaddress() { addr[0] = 0;addr[1] = 0;addr[2] = 0;addr[3] = 0; port = 0; }
    };

    TCPclient(): status(Status::DISCONNECTED), io_context(), socket(io_context) {
        Start(); // start thread routine 
    }

    ~TCPclient(){
        Stop(); // stop thread routine
        Join();
    }


    Status GetStatus(){
        std::scoped_lock lock(tcp_mutex);
        return status;
    }

    bool IsConnected(){
        return GetStatus() == Status::CONNECTED;
    }

    bool IsDisconnected(){
        return GetStatus() == Status::DISCONNECTED;
    }

    bool IsConnecting(){
        return GetStatus() == Status::CONNECTING;
    }


    bool SetIp( IPaddress ip ){
        std::scoped_lock lock(tcp_mutex);

        bool result;
        if(status == Status::DISCONNECTED){
            boost::asio::ip::address_v4::bytes_type addr = { ip.addr[0], ip.addr[1], ip.addr[2], ip.addr[3] };          
            endpoint.address(boost::asio::ip::make_address_v4(addr));
            endpoint.port(ip.port);
            return true;
        }else{
            return false;
        }
    }


    void Connect(){
        std::scoped_lock lock(tcp_mutex);

        if(status != Status::DISCONNECTED){
            socket.close();
        }

        status = Status::CONNECTING;

        onConnecting();
        socket.async_connect(
            endpoint, 
            [this](const boost::system::error_code& error)
            {
                if(error){
                    status = Status::DISCONNECTED;
                    socket.close();
                }else{
                    status = Status::CONNECTED;
                }

                Read();
                onConnected(error);
            } 
            );

    }


    bool Disconnect(){
        std::scoped_lock lock(tcp_mutex);
        
        boost::system::error_code err;
        socket.close();
        if(status != Status::DISCONNECTED) 
            onDisconnected(err);
        status = Status::DISCONNECTED;

        if(err) return false;
        else return true;
    }


protected:


    void Read(){

        std::scoped_lock lock(tcp_mutex);

        socket.async_receive(
            boost::asio::buffer(read_buffer, read_buffer_size), 
            [this](const boost::system::error_code& error, size_t bytes_received)
            {
                if(error){
                    socket.close();
                    if(status != Status::DISCONNECTED) 
                        onDisconnected(error);

                    status = Status::DISCONNECTED;
                }else{
                    status = Status::CONNECTED;
                }

                onRead(error, bytes_received, read_buffer);
                if(!error) Read();
            }
        );
    }


    void Write(const uint8_t* const data, size_t len){

        std::scoped_lock lock(tcp_mutex);

        // copy data to memory
		// shared pointer is necessary because data must be valid until handler is called
		std::shared_ptr<uint8_t[]> mem(new uint8_t[len]);
		memcpy(mem.get(), data, len);
        
        socket.async_write_some(
            boost::asio::buffer(mem.get(), len),
			[this, mem](const boost::system::error_code& error, std::size_t bytes_transferred)
			{ 
                if(error){
                    socket.close();
                    if(status != Status::DISCONNECTED) 
                        onDisconnected(error);

                    status = Status::DISCONNECTED;
                }else{
                    status = Status::CONNECTED;
                }

                onWrite(error, bytes_transferred); 
            }
        );

    }


    virtual void onConnecting() = 0;


    virtual void onDisconnected(const boost::system::error_code& error) = 0;


    virtual void onConnected(const boost::system::error_code& error) = 0;

    // this function is called when succesfully(or not) received some data
    virtual void onRead(const boost::system::error_code& error, size_t bytes_received, const uint8_t* const data) = 0;

	// this function is called when data is succesfuly(or not) sent to client
	virtual void onWrite(const boost::system::error_code& error, std::size_t bytes_transferred) = 0;

    virtual void onLoop() = 0;


    void threadJob() override{


        try{
            while(IsRun()){

                auto step = std::chrono::milliseconds(10);
                auto now = std::chrono::high_resolution_clock::now();

                {
                    std::scoped_lock lock(tcp_mutex);
                    onLoop();
                    io_context.run_until(now + step);
                }

                std::this_thread::sleep_until(now + step);

            }
        }catch (std::exception& e) {
			std::cout << "Caught error in context.run(): \n\t" << e.what() << "\n\t"
					  << "TCP Client stops working !!!\n\n";
		}

    }

};




class PLCclient : public TCPclient {

public:

    enum class EventType{
        NONE,
        CONNECTING,
        CONNECTED,
        DISCONNECTED,
        CONNECTION_FAILED,
        CONNECTION_LOST
    };

    struct Event{
        EventType event;
        boost::system::error_code error;

        Event(): event(EventType::NONE), error(){};
        Event(EventType e): event(e), error(){};
        Event(EventType e, const boost::system::error_code& err): event(e), error(err){};

        std::string ToStr(){

            std::string str;

            switch (event){
                case EventType::NONE:              str = "None";              break;
                case EventType::CONNECTING:        str = "Connecting ...";    break;
                case EventType::CONNECTED:         str = "Connected";         break;
                case EventType::DISCONNECTED:      str = "Disconnected";      break;
                case EventType::CONNECTION_FAILED: str = "Connection Failed"; break;
                case EventType::CONNECTION_LOST:   str = "Connection Lost";   break;
                default: str = "Unnown event";
            }

            if(error){
                str += " - " + error.what();
            }
            return str;
        }

        DebugLogger::Priority GetPriority(){
            if(error) return DebugLogger::Priority::_ERROR;

            switch (event){
                case EventType::NONE:              return DebugLogger::Priority::_INFO;
                case EventType::CONNECTING:        return DebugLogger::Priority::_INFO;
                case EventType::CONNECTED:         return DebugLogger::Priority::_SUCCESS;
                case EventType::DISCONNECTED:      return DebugLogger::Priority::_WARNING;
                case EventType::CONNECTION_FAILED: return DebugLogger::Priority::_ERROR;
                case EventType::CONNECTION_LOST:   return DebugLogger::Priority::_ERROR;
                default: return DebugLogger::Priority::_INFO;
            }
        }

    };


    std::queue<Event> PullEvent(){
        std::queue<Event> events;

        event_queue_mutex.lock();
        event_queue.swap(events);
        event_queue_mutex.unlock();

        return events;
    }


    std::queue<std::string> GetTxMessages(){
        std::queue<std::string> messages;
        
        RX_TX_messages_mutex.lock();
        TX_messages.swap(messages);
        RX_TX_messages_mutex.unlock();

        return messages;
    }

    std::queue<std::string> GetRxMessages(){
        std::queue<std::string> messages;

        RX_TX_messages_mutex.lock();
        RX_messages.swap(messages);;
        RX_TX_messages_mutex.unlock();

        return messages;
    }


    bool IsResponding(){
        event_queue_mutex.lock();
        bool is_resp = is_responding;
        event_queue_mutex.unlock();
        return is_resp;
    }



private:

    std::mutex event_queue_mutex;
    std::queue<Event> event_queue;

    std::mutex RX_TX_messages_mutex;
    std::queue<std::string> RX_messages;
    std::queue<std::string> TX_messages;


    static constexpr auto response_check_delay = std::chrono::milliseconds(1000);
    std::chrono::steady_clock::time_point last_received_time;
    bool is_responding = false;


    boost::json::stream_parser json_parser;

public:
    struct FileWriteResponse{
        FileWriteResponse():result(Result::_ERR){};

        enum class Result{_OK, _ERR} result;
        std::string msg;
    };

    struct AppBuildResponse{
        AppBuildResponse():result(Result::_ERR){};

        enum class Result{_OK, _ERR} result;
        struct ErrorMsg{
            int64_t exit_code;
            std::string file;
            std::string error;
            ErrorMsg(): exit_code(0),file(),error(){};
            ErrorMsg(int64_t _exit_code, std::string _file, std::string _error): exit_code(_exit_code), file(_file),error(_error){};
        };
        std::vector<ErrorMsg> compilation_errors;
    };

    struct AppStartResponse{
        AppStartResponse():result(Result::_ERR){};

        enum class Result{_OK, _ERR} result;
        std::string msg;
    };

    struct AppStopResponse{
        AppStopResponse():result(Result::_ERR){};

        enum class Result{_OK, _ERR} result;
        std::string msg;
    };

    struct AppStatusResponse{
        AppStatusResponse():result(Result::_ERR), status(Status::_UNNOWN){};

        enum class Result{_OK, _ERR} result;
        enum class Status{_UNNOWN,_STOPPED, _RUNNING} status;
    };

private:
    std::mutex response_mutex;
    bool filewrite_response_received = false;
    FileWriteResponse filewrite_response;

    bool appbuild_response_received = false;
    AppBuildResponse appbuild_response;

    bool appstart_response_received = false;
    AppStartResponse appstart_response;

    bool appstop_response_received = false;
    AppStopResponse appstop_response;

    bool appstatus_response_received = false;
    AppStatusResponse appstatus_response;

public:


    bool GetIfFileWriteResponse(FileWriteResponse* response){
        std::scoped_lock(response_mutex);

        if(filewrite_response_received){
            *response = filewrite_response;
        }
        return filewrite_response_received;
    }

    
    bool GetIfCompileCodeeResponse(AppBuildResponse* response){
        std::scoped_lock(response_mutex);

        if(appbuild_response_received){
            *response = appbuild_response;
        }
        return appbuild_response_received;
    }

    bool GetIfAppStartResponse(AppStartResponse* response){
        std::scoped_lock(response_mutex);

        if(appstart_response_received){
            *response = appstart_response;
        }
        return appstart_response_received;
    }

    bool GetIfAppStopResponse(AppStopResponse* response){
        std::scoped_lock(response_mutex);

        if(appstop_response_received){
            *response = appstop_response;
        }
        return appstop_response_received;
    }

    bool GetIfAppStatusResponse(AppStatusResponse* response){
        std::scoped_lock(response_mutex);

        if(appstatus_response_received){
            *response = appstatus_response;
        }
        return appstatus_response_received;
    }


private:
    

	void parseJsonStream( const char * const data, size_t len) {
		boost::json::value json;

		std::error_code err;
		size_t start = 0;
		for (size_t i = 0; i < len; i++) {
			if (data[i] == '\n') {
				json_parser.write(&data[start], i - start, err);
				json_parser.finish(err);

				if (json_parser.done()) {
					auto json = json_parser.release();
					//DataFrame data = boost::json::value_to<DataFrame>(json);
					onReadCommand(json);
				}
				else {
					onReadCommand(err);
				}
				json_parser.reset();
				start = i;
			}
		}
		json_parser.write(&data[start], len - start, err);
	}



    virtual void onConnecting(){
        event_queue_mutex.lock();
        event_queue.emplace(EventType::CONNECTING);
        event_queue_mutex.unlock();
    }


    virtual void onConnected(const boost::system::error_code& error) {
        event_queue_mutex.lock();
        if(error) event_queue.emplace(EventType::CONNECTION_FAILED, error);
        else event_queue.emplace(EventType::CONNECTED, error);
        event_queue_mutex.unlock();
    }


    virtual void onDisconnected(const boost::system::error_code& error) {
        event_queue_mutex.lock();
        if(error) event_queue.emplace(EventType::CONNECTION_LOST, error);
        else event_queue.emplace(EventType::DISCONNECTED, error);
        event_queue_mutex.unlock();
    }


    // this function is called when succesfully(or not) received some data
    virtual void onRead(const boost::system::error_code& error, size_t bytes_received, const uint8_t * const data) {
        if(error) return;
        
        parseJsonStream((const char * const)data, bytes_received);
    }

    void onReadCommand(std::error_code err){

    }

    void onReadCommand(const boost::json::value& js){
        last_received_time = std::chrono::high_resolution_clock::now();

        // std::cout << "Received :: " << js << "\n";
        if(auto obj_js = js.if_object()){
            
            if(auto cmd_js = obj_js->if_contains("Cmd")){

                if(auto cmd_str_js = cmd_js->if_string()){

                    std::string cmd = cmd_str_js->c_str();
                    
                    if(cmd == "PING") onReadCommandResponsePing();
                    else if(cmd == "FILE_WRITE") onReadCommandResponseFileWrite(*obj_js);
                    else if(cmd == "APP_BUILD") onReadCommandResponseAppBuild(*obj_js);
                    else if(cmd == "APP_START") onReadCommandResponseAppStart(*obj_js);
                    else if(cmd == "APP_STOP") onReadCommandResponseAppStop(*obj_js);
                    else if(cmd == "APP_STATUS") onReadCommandResponseAppStatus(*obj_js);
                }
            }
        }

        

        RX_TX_messages_mutex.lock();
        RX_messages.emplace(boost::json::serialize(js));
        RX_TX_messages_mutex.unlock();
    }

    void onReadCommandResponsePing(){

    }

    void onReadCommandResponseFileWrite(const boost::json::object& js){
        FileWriteResponse response;

        if(auto result_js = js.if_contains("Result")){
            if(auto result_str = result_js->if_string()){
                if(*result_str == "OK") response.result = FileWriteResponse::Result::_OK;
                else response.result = FileWriteResponse::Result::_ERR;
            }
        }

        if(auto msg_js = js.if_contains("Msg")){
            if(auto msg_str = msg_js->if_string()){
                response.msg = msg_str->c_str();
            }
        }

        {
            std::scoped_lock lock(response_mutex);
            filewrite_response_received = true;
            filewrite_response = response;
        }
    }

    void onReadCommandResponseAppBuild(const boost::json::object& js){
        AppBuildResponse response;

        if(auto result_js = js.if_contains("Result")){
            if(auto result_str = result_js->if_string()){
                if(*result_str == "OK") response.result = AppBuildResponse::Result::_OK;
                else response.result = AppBuildResponse::Result::_ERR;
            }
        }

        if(auto errors_js = js.if_contains("CompilationResult")){
            if(auto errors_arr_js = errors_js->if_array()){
                for(auto error_js: *errors_arr_js){
                    if(auto error_obj_js = error_js.if_object()){

                        auto file_val = error_obj_js->if_contains("File");
                        auto exit_code_val = error_obj_js->if_contains("ExitCode");
                        auto error_msg_val = error_obj_js->if_contains("ErrorMsg");
                        
                        if(!file_val || !exit_code_val || !error_msg_val) continue;

                        auto file_str = file_val->if_string();
                        auto error_msg_str = error_msg_val->if_string();

                        auto exit_code_num_i64 = exit_code_val->if_int64();
                        auto exit_code_num_u64 = exit_code_val->if_uint64();

                        if(!file_str || !error_msg_str) continue;
                        if(!exit_code_num_i64 && !exit_code_num_u64) continue;

                        int64_t error_code = exit_code_num_i64 ? *exit_code_num_i64 : *exit_code_num_u64;

                        response.compilation_errors.emplace_back(error_code, file_str->c_str(), error_msg_str->c_str());
                    }
                }
            }
        }

        {
            std::scoped_lock lock(response_mutex);
            appbuild_response_received = true;
            appbuild_response = response;
        }
    }

    void onReadCommandResponseAppStart(const boost::json::object& js){
        AppStartResponse response;

        if(auto result_js = js.if_contains("Result")){
            if(auto result_str = result_js->if_string()){
                if(*result_str == "OK") response.result = AppStartResponse::Result::_OK;
                else response.result = AppStartResponse::Result::_ERR;
            }
        }

        if(auto msg_js = js.if_contains("Msg")){
            if(auto msg_str = msg_js->if_string()){
                response.msg = msg_str->c_str();
            }
        }

        {
            std::scoped_lock lock(response_mutex);
            appstart_response_received = true;
            appstart_response = response;
        }
    }

    void onReadCommandResponseAppStop(const boost::json::object& js){
        AppStopResponse response;

        if(auto result_js = js.if_contains("Result")){
            if(auto result_str = result_js->if_string()){
                if(*result_str == "OK") response.result = AppStopResponse::Result::_OK;
                else response.result = AppStopResponse::Result::_ERR;
            }
        }

        if(auto msg_js = js.if_contains("Msg")){
            if(auto msg_str = msg_js->if_string()){
                response.msg = msg_str->c_str();
            }
        }

        {
            std::scoped_lock lock(response_mutex);
            appstop_response_received = true;
            appstop_response = response;
        }
    }

    void onReadCommandResponseAppStatus(const boost::json::object& js){
        AppStatusResponse response;

        if(auto result_js = js.if_contains("Result")){
            if(auto result_str = result_js->if_string()){
                if(*result_str == "OK") response.result = AppStatusResponse::Result::_OK;
                else response.result = AppStatusResponse::Result::_ERR;
            }
        }

        if(auto result_js = js.if_contains("Status")){
            if(auto result_str = result_js->if_string()){
                if(*result_str == "RUNNING") response.status = AppStatusResponse::Status::_RUNNING;
                else if(*result_str == "STOPPED") response.status = AppStatusResponse::Status::_STOPPED;
                else response.status = AppStatusResponse::Status::_UNNOWN;
            }
        }

        {
            std::scoped_lock lock(response_mutex);
            appstatus_response_received = true;
            appstatus_response = response;
        }
    }




    // this function is called when data is succesfuly(or not) sent to client
    virtual void onWrite(const boost::system::error_code& error, std::size_t bytes_transferred) {
        if(error) return;
    }


    int i=0;

    virtual void onLoop() {
        if(!IsConnected()) return;

        is_responding = std::chrono::high_resolution_clock::now() < (response_check_delay + last_received_time);

        // if((i--) == 0){
        //     i = 200;

        //     SendPing();
        // }
    }

    void WriteAndLog(const std::string& msg_str){
        Write((const uint8_t *) msg_str.c_str(), msg_str.size());
        RX_TX_messages_mutex.lock();
        TX_messages.emplace(msg_str);
        RX_TX_messages_mutex.unlock();
    }





    void SendPing(){
        boost::json::object msg;
        msg["Cmd"] = "PING";

        std::string msg_str = boost::json::serialize(msg) + "\n";

        WriteAndLog(msg_str);
        // Write((const uint8_t *) msg_str.c_str(), msg_str.size());
    }


    void DataToHexStr(const uint8_t* buf, size_t count, std::string* result){
        
        std::unique_ptr data = std::make_unique<char[]>(count*2); // this is only to prevent memory leaks
        char* data_ptr = data.get();

        for(size_t i = 0; i < count; i++){
            char lower = buf[i] & 0x0f;
            char higher = buf[i] >> 4;

            data_ptr[i * 2] = higher < 10 ? '0' + higher : 'a' - 0xa + higher;
            data_ptr[i * 2 + 1] = lower < 10 ? '0' + lower : 'a' - 0xa + lower;
        }

        result->assign(data_ptr, count * 2);
    }



public:


    void AppStart(){
        boost::json::object msg;
        msg["Cmd"] = "APP_START";

        std::string msg_str = boost::json::serialize(msg) + "\n";

        {
            std::scoped_lock lock(response_mutex);
            appstart_response_received = false;
        }

        WriteAndLog(msg_str);        
    }


    void AppStop(){
        boost::json::object msg;
        msg["Cmd"] = "APP_STOP";

        std::string msg_str = boost::json::serialize(msg) + "\n";

        {
            std::scoped_lock lock(response_mutex);
            appstop_response_received = false;
        }

        WriteAndLog(msg_str);        
    }



    void FileWriteStr(const std::string& str, std::string file_name ){
        
        std::string file_hex;
        DataToHexStr((const uint8_t*)str.c_str(), str.size(), &file_hex);

        boost::json::object msg;
        msg["Cmd"] = "FILE_WRITE";
        msg["FileName"] = file_name;
        msg["Data"] = file_hex;

        std::string msg_str = boost::json::serialize(msg) + "\n";
     
        // Write((const uint8_t*)msg_str.c_str(), msg_str.size());
        {
            std::scoped_lock lock(response_mutex);
            filewrite_response_received = false;
        }
        WriteAndLog(msg_str);
    }


    void CompileCode(){
        boost::json::object msg;
        msg["Cmd"] = "APP_BUILD";

        std::string msg_str = boost::json::serialize(msg) + "\n";

        {
            std::scoped_lock lock(response_mutex);
            appbuild_response_received = false;
        }

        WriteAndLog(msg_str);
    }


    void CheckAppStatus(){
        boost::json::object msg;
        msg["Cmd"] = "APP_STATUS";

        std::string msg_str = boost::json::serialize(msg) + "\n";

        {
            std::scoped_lock lock(response_mutex);
            appbuild_response_received = false;
        }

        WriteAndLog(msg_str);
    }




};






