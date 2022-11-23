#include<iostream>
#include "base/events.h"
#include"gui_main.h"
#include"vulkan/vulkan_main.h"
/*
main线程
应维护消息队列


 
gui线程
应维护消息队列


*/


int main(int, char**)
{
    std::cout << "Cranks Renderer main" << std::endl;

    MultiTaskCtx thread_ctx(1);
    EventLoop(thread_ctx, vulkan_main);

    std::cout << "Cranks Renderer over" << std::endl;

    return 0;
}
