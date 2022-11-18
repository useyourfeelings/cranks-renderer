#include<iostream>
#include<vector>
#include<list>
#include<memory>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include"thread.h"


/*
* 
* 可设计成等待唯一的events_mutex。所有消息都在里面处理，在里面区分不同消息类型，比如线程调度相关，比如单纯的消息传递。
* 
* 需定义一个详细的消息类型。覆盖所有消息。
* 
* 应有一批最底层的事件。比如有线程结束。Thread结束时通知loop进行join。
* 
* 客户端代码可以注册消息
* 
* 只提供底层框架。不应与具体功能耦合。
* 
* 其他：
* 可做心跳。线程太久没反应，需要处理。
*/

static bool got_events = 0;
std::vector<int> events;
std::vector<std::function<void(const MultiTaskArg&)>> events_map;
std::mutex events_mutex, test_mutex;
std::condition_variable events_cv;

int RegisterEvent(std::function<void(const MultiTaskArg&)> f) {
    std::lock_guard<std::mutex> lk(events_mutex);

    std::cout << "register_event" << std::endl;

    events_map.push_back(f);

    return int(events_map.size() - 1 + int(EVENT_END));
}

int SendEvent(int id) {
    std::lock_guard<std::mutex> lk(events_mutex);

    got_events = true;
    events.push_back(id);
    events_cv.notify_all();

    return 0;
}

/*

主循环

启动init_func，等待消息。可注册消息，然后发消息创建新线程。

threads存放线程。

*/ 
int EventLoop(std::function<void(const MultiTaskArg&)> init_func) {
    std::lock_guard<std::mutex> guard(test_mutex);
    
    std::cout << "EventLoop" << std::endl << "core number: " << std::thread::hardware_concurrency() << std::endl;

    std::list<Thread> threads;

    threads.push_back(Thread(init_func));
    threads.back().Start();

    for (;;) {
        std::unique_lock<std::mutex> lock(events_mutex);
        //std::cout << "events_cv.wait_for start" << std::endl;

        // https://en.cppreference.com/w/cpp/thread/condition_variable/wait_for
        // 阻塞等各种消息
        // got_events用于区分收到事件和wait超时
        // wait_for等notify或超时，然后检查是否got_events。
        // 必须先争events_mutex，否则是UB。
        if (events_cv.wait_for(lock, std::chrono::seconds(5), [] { return got_events; })) {
            // got events
            std::cout << "got events" << std::endl;

            for (auto event : events) {
                std::cout << event << std::endl;

                if (event < int(EVENT_END)) { // system
                    int event_id = event - int(EVENT_END);

                    switch(event_id){
                        case EVENT_THREAD_OVER: // 其他线程结束时发这个消息，让这边及时join。
                            break;// 啥也不干。触发wait就行。下面处理join。

                    }
                }
                else { // client
                    threads.push_back(Thread(events_map[event - int(EVENT_END)]));
                    threads.back().Start();
                }
            }

            // 删除所有消息
            if (events.size())
                events.clear();
        }
        else {
            // timeout
        }

        //std::cout << "events_cv.wait_for end" << std::endl;

        // check threads
        auto t = threads.begin();
        while (t != threads.end()) {
            if (t->IsDone()) {
                t->Join();
                t = threads.erase(t);
            }
            else {
                ++t;
            }
        }

        // all dead
        if (threads.empty()) {
            std::cout << "EventLoop no running threads" << std::endl;
            break;
        }

        got_events = false;
    }

    std::cout << "EventLoop over" << std::endl;

    return 0;
}

// 阻塞执行多线程任务
void StartMultiTask(std::function<void(MultiTaskArg)> thread_func,
    std::function<MultiTaskArg(int, MultiTaskArg)> manager_func,
    MultiTaskArg args) {

    std::vector<Thread> threads;

    std::cout << "StartMultiTask" << std::endl;

    for (int i = 0; i < args.task_count; ++ i) {
        threads.push_back(Thread(thread_func, manager_func(i, args)));

        // https://stackoverflow.com/questions/31071761/why-only-last-thread-executing
        // 可导致之前的thread失效，只有最后一个thread能正常执行。
        //threads.back().Start(); 
    }

    for (auto t = threads.begin(); t != threads.end(); ++t) {
        t->Start();
    }

    for (auto t = threads.begin(); t != threads.end(); ++ t) {
        t->Join();
    }

}

