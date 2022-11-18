#ifndef VULKAN_MAIN_H
#define VULKAN_MAIN_H

// https://www.glfw.org/docs/latest/vulkan_guide.html
//#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include"vulkan_device.h"
#include"vulkan_swapchain.h"
#include"vulkan_tool.h"
#include"../tool/json.h"
#include"../base/events.h"

class VulkanUI;

class VulkanApp {
public:
    VulkanApp();

    ~VulkanApp();

    int init();
    int BuildImage(int width, int height, unsigned char* pixels);
    int BuildTestImage(int width, int height, unsigned char* pixels, int index);
    int DestroyCommandBuffers();
    int CreateCommandBuffers();
    void BuildCommandBuffers();
    void FrontendUI(uint32_t lastFPS, float frameDuration);
    int VulkanDraw();
    void OnWindowResize();
    void DestroyFrameBuffer();
    void SetupFrameBuffer();
    int loop();
    int clean();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
    VkDebugUtilsMessengerEXT debugUtilsMessenger;

    std::vector<std::string> supportedInstanceExtensions;
    std::vector<const char*> enabledInstanceExtensions;

    GLFWwindow* window;
    VkInstance instance;

    bool prepared = false; // for OnWindowResize
    bool resized = false;

    //VkPhysicalDevice physicalGPU;
    std::shared_ptr<VulkanDevice> device = nullptr;

    std::shared_ptr<VulkanSwapchain> swapChain = nullptr;

    std::shared_ptr<VulkanUI> ui = nullptr;

    VkDevice gpu;
    //VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkShaderModule vertShader;
    VkShaderModule fragShader;

    VkDescriptorSetLayout descriptorSetLayout;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkRenderPass renderPass = VK_NULL_HANDLE;

    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo; // ø…∏¥”√

    std::vector<VkFramebuffer>frameBuffers;
    // Active frame buffer index
    uint32_t imageIndex = 0;

    VkCommandPool cmdPool;
    //VkCommandBuffer commandBuffer;
    std::vector<VkCommandBuffer> cmdBuffers;

    VkSemaphore presentCompleteSemaphore;
    VkSemaphore renderCompleteSemaphore;

    std::vector<VkFence> waitFences;

    // Stores physical device properties (for e.g. checking device limits)
    VkPhysicalDeviceProperties gpuProperties;
    // Stores the features available on the selected physical device (for e.g. checking if a feature is available)
    VkPhysicalDeviceFeatures gpuFeatures;
    // Stores all available memory (type) properties for the physical device
    VkPhysicalDeviceMemoryProperties gpuMemoryProperties;

    VulkanImage renderImage;

    std::vector<std::shared_ptr<VulkanImage>> testImages;
};

inline VulkanApp vulkan_app;

void vulkan_main(const MultiTaskArg& args);

#endif