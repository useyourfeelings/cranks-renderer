#ifndef THREAD_H
#define THREAD_H

#include<iostream>
#include<thread>
#include <functional>
#include"events.h"
#include"../tool/json.h"

enum ThreadStatus {
    THREAD_NEW = 0,
    THREAD_RUNNING,
    THREAD_DONE,
    THREAD_JOINED,
};

class Thread {
public:
    Thread(std::function<void(MultiTaskCtx &)> f, MultiTaskCtx args) :
        thread_function(f),
        args(args),
        status(THREAD_NEW) {
    }

    bool IsDone() {
        //std::cout << "IsDone id " << system_thread.get_id() <<" status = "<< status << std::endl;
        return status == THREAD_DONE;
    }

    bool IsJoined() {
        //std::cout << "IsDone id " << system_thread.get_id() <<" status = "<< status << std::endl;
        return status == THREAD_JOINED;
    }

    void Join() {
        if (status == THREAD_JOINED)
            return;

        std::cout << "Join id "<< system_thread.get_id() << std::endl;
        system_thread.join();
        status = THREAD_JOINED;
    }

    void Start() {
        //std::cout << "start ..." << std::endl;

        // https://en.cppreference.com/w/cpp/thread/thread/thread
        system_thread = std::thread(&Thread::thread_wrapper, this);
    }

    /*int GetID() {
        return system_thread.get_id();
    }*/


private:
    int thread_wrapper() {
        status = THREAD_RUNNING;

        //std::cout << "Thread start " << system_thread.get_id() << " " << &thread_function << std::endl;

        try {
            thread_function(args);
        }
        catch (const std::exception& e) {
            std::cout << "Thread catch exception " << e.what();
        }
        /*catch (...) {
            std::cout << "Thread catch ..." << std::endl;
        }*/

        //status = THREAD_DONE;

        //std::cout << "Thread end " << system_thread.get_id() << std::endl;

        SendEvent(args, EVENT_THREAD_OVER);

        status = THREAD_DONE;

        return 0;
    }

    //int (*thread_function)();
    std::function<void(MultiTaskCtx &)> thread_function;

    MultiTaskCtx args;

    ThreadStatus status;

    std::thread system_thread;
};

#endif