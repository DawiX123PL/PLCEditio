#pragma once


#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <iostream>
#include <queue>
#include <boost/json.hpp>
#include <chrono>
#include "debug_console.hpp"



class Thread {
	std::thread* thread;
	std::mutex stopMutex;
	bool stopFlag;


public:
	Thread() :thread(nullptr), stopFlag(false) {}
	~Thread() { if (thread) delete thread; }

	void Start() {
		thread = new std::thread(&Thread::threadJob, this);
	}


	void Stop() {
		stopMutex.lock();
		stopFlag = true;
		stopMutex.unlock();
	}


	bool IsRun() {
		stopMutex.lock();
		bool isRun = !stopFlag;
		stopMutex.unlock();
		return isRun;
	}

	void Join() {
		thread->join();
	}

private:
	virtual void threadJob() = 0;

};



class TCPclient: public Thread{

public:
    enum class Status{ DISCONNECTED, CONNECTED, CONNECTING};


private:
    std::recursive_mutex mutex;
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
        mutex.lock();
        Status s = status;
        mutex.unlock();
        return s;
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
        mutex.lock();
        bool result;
        if(status == Status::DISCONNECTED){
            boost::asio::ip::address_v4::bytes_type addr = { ip.addr[0], ip.addr[1], ip.addr[2], ip.addr[3] };          
            endpoint.address(boost::asio::ip::make_address_v4(addr));
            endpoint.port(ip.port);
            result = true;
        }else{
            result = false;
        }
        mutex.unlock();
        return result;
    }


    void Connect(){
        mutex.lock();

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

        mutex.unlock();
    }


    bool Disconnect(){
        mutex.lock();     
        
        boost::system::error_code err;
        socket.close();
        if(status != Status::DISCONNECTED) 
            onDisconnected(err);
        status = Status::DISCONNECTED;

        mutex.unlock();
        if(err) return false;
        else return true;
    }


protected:


    void Read(){

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

    virtual void loop() = 0;


    void threadJob() override{


        try{
            while(IsRun()){

                auto step = std::chrono::milliseconds(10);
                auto now = std::chrono::high_resolution_clock::now();

                {
                    std::scoped_lock lock(mutex);
                    loop();
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

    bool IsResponding(){
        event_queue_mutex.lock();
        bool is_resp = is_responding;
        event_queue_mutex.unlock();
        return is_resp;
    }



private:

    std::mutex event_queue_mutex;
    std::queue<Event> event_queue;

    static constexpr auto response_check_delay = std::chrono::milliseconds(1000);
    std::chrono::steady_clock::time_point last_received_time;
    bool is_responding = false;


    boost::json::stream_parser json_parser;


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


    void onReadCommand(const boost::json::value& js){
        last_received_time = std::chrono::high_resolution_clock::now();

        std::cout << "Received :: " << js << "\n";
    }


    void onReadCommand(std::error_code err){

    }



    // this function is called when data is succesfuly(or not) sent to client
    virtual void onWrite(const boost::system::error_code& error, std::size_t bytes_transferred) {
        if(error) return;
    }


    int i=0;

    virtual void loop() {
        if(!IsConnected()) return;

        is_responding = std::chrono::high_resolution_clock::now() < (response_check_delay + last_received_time);

        if((i--) == 0){
            i = 200;

            SendPing();
        }
    }


    void SendPing(){
        boost::json::object msg;
        msg["Cmd"] = "PING";

        std::string msg_str = boost::json::serialize(msg) + "\n";

        Write((const uint8_t *) msg_str.c_str(), msg_str.size());
    }


};





