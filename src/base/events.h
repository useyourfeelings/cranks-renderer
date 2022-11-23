#ifndef EVENTS_H
#define EVENTS_H

#include<functional>
#include<mutex>
#include"../tool/json.h"

struct MultiTaskEvent {
    MultiTaskEvent(int event_type, int task_index = 0):
        type(event_type),
        task_index(task_index)
    {
    }

    int task_index;
    int type; // 0-over 1-call help

    json extra;

};

struct MultiTaskCtx {
    MultiTaskCtx() {
    }

    MultiTaskCtx(int task_count):
        task_count(task_count),
        task_index(0),
        event_flag(std::make_shared<int>(0)),
        control_mutex(std::make_shared<std::mutex>()),
        control_cv(std::make_shared<std::condition_variable>()),
        task_cv(std::make_shared<std::vector<std::condition_variable>>(task_count)),
        task_flag(std::make_shared<std::vector<int>>(task_count)),
        task_mutex(std::make_shared<std::vector<std::mutex>>(task_count)),
        events(std::make_shared<std::vector<MultiTaskEvent>>()),
        events_map(std::make_shared<std::vector<std::function<void(const MultiTaskCtx&)>>>())
    {
    }

    int task_count;
    int task_index;
    int x_start;
    int x_end;
    int y_start;
    int y_end;
    int task_progress_total;

    json extra;

    std::shared_ptr<int> event_flag;
    std::shared_ptr<std::mutex> control_mutex;
    std::shared_ptr<std::condition_variable> control_cv;
    std::shared_ptr<std::vector<std::condition_variable>> task_cv;
    std::shared_ptr<std::vector<std::mutex>> task_mutex;
    std::shared_ptr<std::vector<int>> task_flag;
    std::shared_ptr<std::vector<MultiTaskEvent>> events; // std::list

    std::shared_ptr<std::vector<std::function<void(const MultiTaskCtx&)>>> events_map;
};



//int RegisterEvent(std::function<void(const MultiTaskCtx&)> f);
//int SendEvent(int id);

int RegisterEvent(MultiTaskCtx& thread_ctx, std::function<void(const MultiTaskCtx&)> f);
int SendEvent(MultiTaskCtx& thread_ctx, int id);

int EventLoop(MultiTaskCtx& thread_ctx, std::function<void(MultiTaskCtx&)> init_func);

void StartMultiTask(std::function<void(MultiTaskCtx)> thread_func,
    std::function<MultiTaskCtx(int, MultiTaskCtx)> args_func,
    MultiTaskCtx args);

void StartMultiTask2(std::function<void(MultiTaskCtx)> thread_func,
    std::function<MultiTaskCtx(int, MultiTaskCtx)> args_func,
    MultiTaskCtx args);// , std::mutex& task_mutex, std::condition_variable& task_cv);

int MultiTaskCallHelp(MultiTaskCtx& args, int help_x_start, int help_x_end);
//int MultiTaskNotifyDone(MultiTaskCtx& args);


enum EVENT_ID {
    EVENT_THREAD_OVER = 0,
    EVENT_THREAD_CALL_HELP = 1,

    EVENT_END
};

#endif