#include<iostream>
#include "events.h"
//#include"gui_main.h"

//#include<vector>
//#include<thread>

/*
main线程
应维护消息队列


 
gui线程
应维护消息队列


*/



int main(int, char**)
{
    //gui_thread_func();
    std::cout << "Cranks Renderer main" << std::endl;

    /*std::vector<std::thread> threads;

    threads.push_back(std::thread(gui_thread_func));
    for (std::thread& thread : threads) {
        thread.join();
    }*/

    event_loop();
    //gui_thread_func();

    std::cout << "Cranks Renderer over" << std::endl;

    return 0;
}
