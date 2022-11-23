#include<iostream>
#include<vector>
#include<list>
#include<memory>
#include<condition_variable>
#include<mutex>
#include<chrono>
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

int RegisterEvent(MultiTaskCtx& thread_ctx, std::function<void(const MultiTaskCtx&)> f) {
    std::cout << std::format("try RegisterEvent task {}", thread_ctx.task_index);
    std::lock_guard<std::mutex> lock(*thread_ctx.control_mutex);

    std::cout << std::format("do RegisterEvent task {}", thread_ctx.task_index);

    thread_ctx.events_map->push_back(f);

    return int(thread_ctx.events_map->size() - 1 + int(EVENT_END));
}

int SendEvent(MultiTaskCtx& thread_ctx, int event_id) {
    std::cout << std::format("try SendEvent task {} event {}", thread_ctx.task_index, event_id);
    {
        std::lock_guard<std::mutex> lock(*thread_ctx.control_mutex);

        std::cout << std::format("do SendEvent task {} event {}", thread_ctx.task_index, event_id);

        *thread_ctx.event_flag = 1;
        thread_ctx.events->push_back(MultiTaskEvent(event_id, thread_ctx.task_index));
    }
    thread_ctx.control_cv->notify_all();

    return 0;
}

/*

��ѭ��

����init_func���ȴ���Ϣ����ע����Ϣ��Ȼ����Ϣ�������̡߳�

threads����̡߳�

*/ 
int EventLoop(MultiTaskCtx& thread_ctx, std::function<void(MultiTaskCtx&)> init_func) {
    std::cout << "EventLoop" << std::endl << "core number: " << std::thread::hardware_concurrency() << std::endl;

    std::list<Thread> threads;

    threads.push_back(Thread(init_func, thread_ctx));
    threads.back().Start();

    for (;;) {
        std::unique_lock<std::mutex> lock(*thread_ctx.control_mutex); // block
        //std::cout << "events_cv.wait_for start" << std::endl;

        // https://en.cppreference.com/w/cpp/thread/condition_variable/wait_for
        // �����ȸ�����Ϣ
        // got_events���������յ��¼���wait��ʱ
        // wait_for��notify��ʱ��Ȼ�����Ƿ�got_events��
        // ��������events_mutex��������UB��
        if (thread_ctx.control_cv->wait_for(lock, std::chrono::seconds(8), [&] { return *thread_ctx.event_flag != 0; })) {
            // got events
            std::cout << "got events" << std::endl;

            *thread_ctx.event_flag = 0; // reset

            for (auto event : *thread_ctx.events) {
                std::cout << event.type << std::endl;

                if (event.type < int(EVENT_END)) { // system
                    int event_id = event.type - int(EVENT_END);

                    switch(event_id){
                        case EVENT_THREAD_OVER: // �����߳̽���ʱ�������Ϣ������߼�ʱjoin��
                            break;// ɶҲ���ɡ�����wait���С����洦��join��

                    }
                }
                else { // client
                    threads.push_back(Thread((*thread_ctx.events_map)[event.type - int(EVENT_END)], thread_ctx));
                    threads.back().Start();
                }
            }

            // ɾ��������Ϣ
            thread_ctx.events->clear();
        }
        else {
            // timeout
        }

        std::cout << "events_cv.wait_for end" << std::endl;

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

    }

    std::cout << "EventLoop over" << std::endl;

    return 0;
}

// ����ִ�ж��߳�����
void StartMultiTask(std::function<void(MultiTaskCtx)> thread_func,
    std::function<MultiTaskCtx(int, MultiTaskCtx)> manager_func,
    MultiTaskCtx args) {

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

// ����ִ�ж��߳�����
// ʵ�ֵ��ȡ������������Լ�������join���̣߳����������̰߳���ִ�������̵߳�����
// ��EventLoop���ơ�todo���ܷ�����ͨ�õģ�
void StartMultiTask2(std::function<void(MultiTaskCtx)> thread_func,
    std::function<MultiTaskCtx(int, MultiTaskCtx)> manager_func,
    MultiTaskCtx args) {

    std::vector<Thread> threads;

    std::cout << "StartMultiTask2" << std::endl;

    for (int i = 0; i < args.task_count; ++i) {

        threads.push_back(Thread(thread_func, manager_func(i, args)));

        // https://stackoverflow.com/questions/31071761/why-only-last-thread-executing
        // �ɵ���֮ǰ��threadʧЧ��ֻ�����һ��thread������ִ�С�
        //threads.back().Start(); 
    }

    for (auto t = threads.begin(); t != threads.end(); ++t) {
        t->Start();
    }

    for (;;) {
        std::cout << "StartMultiTask2 wait events" << std::endl;
        std::unique_lock<std::mutex> lock(*args.control_mutex); // block
        std::cout << "StartMultiTask2 wait events locked" << std::endl;
        if (args.control_cv->wait_for(lock, std::chrono::seconds(8), [&] { return *args.event_flag != 0; })) {
            // got events
            //std::cout << "StartMultiTask2 got events" << std::endl;

            *args.event_flag = 0; // reset

            for (auto event : *args.events) {
                std::cout << std::format("event = {} task = {}", event.type, event.task_index);

                if (event.type == EVENT_THREAD_OVER) {
                    std::cout << "StartMultiTask2 EVENT_THREAD_OVER join 1" << std::endl;
                    threads[event.task_index].Join(); // һ������join���߳�
                    std::cout << "StartMultiTask2 EVENT_THREAD_OVER join ok" << std::endl;
                }
                else if (event.type == EVENT_THREAD_CALL_HELP) {
                    int has_helper = 0;
                    for (int i = 0; i < args.task_count; ++i) {
                        if (i == event.task_index)
                            continue;

                        if (threads[i].IsJoined()) {
                            //std::cout << "StartMultiTask2 " << i << " before help " << event.task_index << std::endl;
                            //threads[i].Join();

                            has_helper = 1;
                            std::cout << "StartMultiTask2 " << i <<" will help " << event.task_index << std::endl;

                            // help
                            auto newArg = args;
                            newArg.task_index = i;
                            newArg.x_start = event.extra["x_start"];
                            newArg.x_end = event.extra["x_end"];
                            // same y

                            newArg.task_progress_total = (newArg.x_end - newArg.x_start + 1) * (newArg.y_end - newArg.y_start + 1);

                            threads[i] = Thread(thread_func, newArg);
                            threads[i].Start();

                            // send result to caller
                            (*args.task_flag)[event.task_index] = 1; // ok
                            (*args.task_cv)[event.task_index].notify_all();

                            break;
                        }
                    }

                    if (!has_helper) {
                        (*args.task_flag)[event.task_index] = 2; // fail
                        (*args.task_cv)[event.task_index].notify_all();
                    }
                }
            }

            // ɾ��������Ϣ
            args.events->clear();
        }
        else {
            // timeout
            std::cout << "StartMultiTask2 wait_for events timeout" << std::endl;
        }

        // check threads
        bool all_over = true;
        for (int i = 0; i < args.task_count; ++i){
            if (threads[i].IsDone()) {
                threads[i].Join();
                //t = threads.erase(t);
            }
            else if (threads[i].IsJoined()) {
            } else {
                all_over = false;
            }
        }

        if (all_over) {
            std::cout << "StartMultiTask2 no running threads" << std::endl;
            break;
        }
    }

    std::cout << "StartMultiTask2 over" << std::endl;
}


int MultiTaskCallHelp(MultiTaskCtx& args, int help_x_start, int help_x_end) {
    int task_index = args.task_index;

    std::cout << "MultiTaskCallHelp caller = " << task_index << std::endl;

    MultiTaskEvent event(EVENT_THREAD_CALL_HELP, args.task_index);
    event.extra["x_start"] = help_x_start;
    event.extra["x_end"] = help_x_end;

    args.control_mutex->lock();
    std::cout << "MultiTaskCallHelp lock " << task_index << std::endl;

    (*args.task_flag)[task_index] = 0; // ״̬��Ϊ0���ȴ�control���ý����
    *args.event_flag = 1;
    args.events->push_back(event);
    args.control_cv->notify_all();

    args.control_mutex->unlock();

    std::cout << "MultiTaskCallHelp unlock " << task_index << std::endl;

    for (;;) {
        std::unique_lock<std::mutex> lock((*args.task_mutex)[task_index]);

        if ((*args.task_cv)[task_index].wait_for(lock, std::chrono::milliseconds(200), [&] { return (*args.task_flag)[task_index] != 0; })) {
            int result = (*args.task_flag)[task_index];

            if (result == 1) { // ok
                std::cout << "MultiTaskCallHelp ok caller = " << task_index << std::endl;
                return 1;
            }
            else if (result == 2) {
                std::cout << "MultiTaskCallHelp fail caller = " << task_index << std::endl;
                return 0;
            }
        }
        else {
            // timeout
            std::cout << "MultiTaskCallHelp timeout caller = " << task_index << std::endl;
            return 0;
        }
    }
}