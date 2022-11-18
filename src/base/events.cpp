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
* ����Ƴɵȴ�Ψһ��events_mutex��������Ϣ�������洦�����������ֲ�ͬ��Ϣ���ͣ������̵߳�����أ����絥������Ϣ���ݡ�
* 
* �趨��һ����ϸ����Ϣ���͡�����������Ϣ��
* 
* Ӧ��һ����ײ���¼����������߳̽�����Thread����ʱ֪ͨloop����join��
* 
* �ͻ��˴������ע����Ϣ
* 
* ֻ�ṩ�ײ��ܡ���Ӧ����幦����ϡ�
* 
* ������
* �����������߳�̫��û��Ӧ����Ҫ����
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

��ѭ��

����init_func���ȴ���Ϣ����ע����Ϣ��Ȼ����Ϣ�������̡߳�

threads����̡߳�

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
        // �����ȸ�����Ϣ
        // got_events���������յ��¼���wait��ʱ
        // wait_for��notify��ʱ��Ȼ�����Ƿ�got_events��
        // ��������events_mutex��������UB��
        if (events_cv.wait_for(lock, std::chrono::seconds(5), [] { return got_events; })) {
            // got events
            std::cout << "got events" << std::endl;

            for (auto event : events) {
                std::cout << event << std::endl;

                if (event < int(EVENT_END)) { // system
                    int event_id = event - int(EVENT_END);

                    switch(event_id){
                        case EVENT_THREAD_OVER: // �����߳̽���ʱ�������Ϣ������߼�ʱjoin��
                            break;// ɶҲ���ɡ�����wait���С����洦��join��

                    }
                }
                else { // client
                    threads.push_back(Thread(events_map[event - int(EVENT_END)]));
                    threads.back().Start();
                }
            }

            // ɾ��������Ϣ
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

// ����ִ�ж��߳�����
void StartMultiTask(std::function<void(MultiTaskArg)> thread_func,
    std::function<MultiTaskArg(int, MultiTaskArg)> manager_func,
    MultiTaskArg args) {

    std::vector<Thread> threads;

    std::cout << "StartMultiTask" << std::endl;

    for (int i = 0; i < args.task_count; ++ i) {
        threads.push_back(Thread(thread_func, manager_func(i, args)));

        // https://stackoverflow.com/questions/31071761/why-only-last-thread-executing
        // �ɵ���֮ǰ��threadʧЧ��ֻ�����һ��thread������ִ�С�
        //threads.back().Start(); 
    }

    for (auto t = threads.begin(); t != threads.end(); ++t) {
        t->Start();
    }

    for (auto t = threads.begin(); t != threads.end(); ++ t) {
        t->Join();
    }

}

