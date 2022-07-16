#ifndef EVENTS_H
#define EVENTS_H

#include <functional>
#include"json.h"

//int RegisterEvent(int (*f)());
int RegisterEvent(std::function<void(const json&)> f);
int SendEvent(int id);
//int EventLoop(int (*init_func)());
int EventLoop(std::function<void(const json&)> init_func);
void StartMultiTask(std::function<void(const json&)> thread_func,
    std::function<json(int, const json&)> args_func,
    const json& args);

enum EVENT_ID {
    EVENT_THREAD_OVER = 0,

    EVENT_END
};

#endif