#include<iostream>
#include "base/events.h"
#include"gui_main.h"
#include"vulkan/vulkan_main.h"
/*
main�߳�
Ӧά����Ϣ����


 
gui�߳�
Ӧά����Ϣ����


*/


int main(int, char**)
{
    std::cout << "Cranks Renderer main" << std::endl;

    EventLoop(gui_main);
    EventLoop(vulkan_main);

    std::cout << "Cranks Renderer over" << std::endl;

    return 0;
}
