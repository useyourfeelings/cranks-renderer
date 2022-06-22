#ifndef EVENTS_H
#define EVENTS_H

int RegisterEvent(int (*f)());
int SendEvent(int id);
int EventLoop(int (*init_func)());

enum EVENT_ID {
    EVENT_THREAD_OVER = 0,

    EVENT_END
};

#endif