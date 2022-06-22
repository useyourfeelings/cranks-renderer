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
std::vector<int (*)()> events_map;
std::mutex events_mutex, test_mutex;
std::condition_variable events_cv;

int RegisterEvent(int (*f)()) {
    std::lock_guard<std::mutex> lk(events_mutex);

    std::cout << "register_event" << std::endl;

    events_map.push_back(f);

    return events_map.size() - 1 + int(EVENT_END);
}

int SendEvent(int id) {
    std::lock_guard<std::mutex> lk(events_mutex);

    got_events = true;
    events.push_back(id);
    events_cv.notify_all();

    return 0;
}

int EventLoop(int (*init_func)()) {
    std::lock_guard<std::mutex> guard(test_mutex);
    
    std::cout << "event_loop" << std::endl << "core number: " << std::thread::hardware_concurrency() << std::endl;

    std::list<Thread> threads;

    threads.push_back(Thread(init_func));
    threads.back().Start();

    for (;;) {
        std::unique_lock<std::mutex> lock(events_mutex);
        std::cout << "events_cv.wait_for start" << std::endl;

        // �����ȸ�����Ϣ
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

            if (events.size())
                events.clear();
        }

        std::cout << "events_cv.wait_for end" << std::endl;

        auto t = threads.begin();
        while (t != threads.end()) {
            if (t->IsDone()) {
                t->Join();
                t = threads.erase(t);
            }

            ++t;
        }

        if (threads.empty()) {
            std::cout << "event_loop no running threads" << std::endl;
            break;
        }

        got_events = false;
    }

    std::cout << "event_loop over" << std::endl;

    return 0;
}
