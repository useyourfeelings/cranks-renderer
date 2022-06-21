#include<iostream>
#include<vector>
#include<queue>
#include<thread>
#include <condition_variable>
#include <mutex>
#include <chrono>

#include"gui_main.h"
#include"core/api.h"

static bool got_events = 0;
std::vector<int> events;
std::mutex events_mutex, test_mutex;
std::condition_variable events_cv;

int (*fff)();

int register_event(int id, int (*f)()) {
    std::cout << "Cranks Renderer register_event" << std::endl;

    fff = f;

    return 0;
}

int send_event(int id) {
    std::lock_guard<std::mutex> lk(events_mutex);
    //std::cerr << "Notifying...\n";
    //i = 1;
    got_events = true;
    events.push_back(id);
    events_cv.notify_all();

    return 0;
}

int event_loop() {
    std::cout << "Cranks Renderer event_loop" << std::endl << "core number: " << std::thread::hardware_concurrency() << std::endl;

    std::lock_guard<std::mutex> guard(test_mutex);

    std::vector<std::thread> threads;

    threads.push_back(std::thread(gui_thread_func));

    //gui_thread_func();

    for (;;) {
        std::unique_lock<std::mutex> lock(events_mutex);
        if (events_cv.wait_for(lock, std::chrono::seconds(5), [] {return got_events; })) {
            // got events
            std::cout << "got events" << std::endl;

            for (auto event : events) {
                std::cout << event << std::endl;

                if (event == 666) {
                    ///threads.push_back(std::thread(PBR_API_render));
                    threads.push_back(std::thread(fff));
                }
            }
        }


        if (events.size())
            events.clear();

        // todo: check threads status
        // other checks



        got_events = false;
    }


    for (std::thread& thread : threads) {
        thread.join();
    }

    return 0;
}
