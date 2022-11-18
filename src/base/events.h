#ifndef EVENTS_H
#define EVENTS_H

#include <functional>
#include"../tool/json.h"

struct MultiTaskArg {
    int task_count;
    int task_index;
    __int64 x_start;
    __int64 x_end;
    __int64 y_start;
    __int64 y_end;
    __int64 task_progress_total;

    json extra;
};

int RegisterEvent(std::function<void(const MultiTaskArg&)> f);
int SendEvent(int id);
int EventLoop(std::function<void(const MultiTaskArg&)> init_func);
void StartMultiTask(std::function<void(MultiTaskArg)> thread_func,
    std::function<MultiTaskArg(int, MultiTaskArg)> args_func,
    MultiTaskArg args);

enum EVENT_ID {
    EVENT_THREAD_OVER = 0,

    EVENT_END
};

#endif