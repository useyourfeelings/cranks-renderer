#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include<iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include"vulkan_device.h"
#include"vulkan_tool.h"

class VulkanSwapchain {
public:
	explicit VulkanSwapchain(std::shared_ptr<VulkanDevice> device, VkInstance instance, GLFWwindow* window);

	void Create();
	void Clean();

	//VkPhysicalDevice physicalDevice;

	//VkDevice device;

	//VkQueue graphicsQueue;

	VkInstance instance;

	int width, height;

	uint32_t imageCount = 0;

	GLFWwindow* window;

	std::shared_ptr<VulkanDevice> device = nullptr;

	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR surfaceMode = VK_PRESENT_MODE_FIFO_KHR;

	VkSurfaceCapabilitiesKHR cap;

};

#endif