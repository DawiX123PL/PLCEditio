#pragma once

#include <thread>
#include <mutex>



class Thread {
	std::thread* thread;
	std::mutex stopMutex;
	bool stopFlag;
    bool isRunning;

public:
	Thread() :thread(nullptr), stopFlag(false), isRunning(false) {}
	virtual ~Thread() {
		if (thread) {
			Stop();
			Join();
			delete thread;
		}
	}

	void Start() {
		if(thread) Join();
		thread = new std::thread(&Thread::threadJobWrapper, this);
	}


	void Stop() {
		stopMutex.lock();
		stopFlag = true;
		stopMutex.unlock();
	}

protected:
	// to check if thread is forced to stop
	bool IsRun() {
		stopMutex.lock();
		bool isRun = !stopFlag;
		stopMutex.unlock();
		return isRun;
	}

public:
	// to check if thread stopped working
    bool IsRunning(){
        stopMutex.lock();
		bool is_run = isRunning;
		stopMutex.unlock();
		return is_run;
    }

	void Join() {
		if (thread) {
			if(thread->joinable())
				thread->join();
		}
	}

private:
    void threadJobWrapper(){
        stopMutex.lock();
        stopFlag = false;
		isRunning = true;
        stopMutex.unlock();

        threadJob();

        stopMutex.lock();
        stopFlag = true;
		isRunning = false;
        stopMutex.unlock();
    }

	virtual void threadJob() = 0;

};

