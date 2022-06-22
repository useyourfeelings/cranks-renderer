#include<iostream>
#include<thread>
#include"events.h"

enum ThreadStatus {
    THREAD_NEW = 0,
    THREAD_RUNNING,
    THREAD_DONE
};

class Thread {
public:
    Thread(int (*f)()) :thread_function(f), status(THREAD_NEW) {
    }

    bool IsDone() {
        std::cout << "IsDone id " << system_thread.get_id() <<" status = "<< status << std::endl;
        return status == THREAD_DONE;
    }

    void Join() {
        std::cout << "Join id "<< system_thread.get_id() << std::endl;
        system_thread.join();
    }

    void Start() {
        //std::cout << "start ..." << std::endl;
        system_thread = std::thread(&Thread::thread_wrapper, this);
    }

    /*int GetID() {
        return system_thread.get_id();
    }*/


private:
    int thread_wrapper() {
        //std::lock_guard<std::mutex> lock(thread_mutex);
        status = THREAD_RUNNING;

        std::cout << "Thread start " << system_thread.get_id() << std::endl;

        try {
            thread_function();
        }
        catch (const std::exception& e) {
            std::cout << "Thread catch exception " << e.what();
        }
        catch (...) {
            std::cout << "Thread catch ..." << std::endl;
        }

        status = THREAD_DONE;

        std::cout << "Thread end " << system_thread.get_id() << std::endl;

        SendEvent(EVENT_THREAD_OVER);

        return 0;
    }

    int (*thread_function)();
    //std::mutex thread_mutex;

    ThreadStatus status;

    std::thread system_thread;
};