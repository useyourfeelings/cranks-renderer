#ifndef EVENTS_H
#define EVENTS_H

int register_event(int id, int (*f)());
int send_event(int id);
int event_loop();

#endif