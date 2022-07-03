#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include<iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include"vulkan_tool.h"
#include"vulkan_buffer.h"

class VulkanDevice {
public:
	explicit VulkanDevice(VkInstance instance);
	~VulkanDevice();

	uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound = nullptr) const;
	VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VulkanBuffer* buffer, VkDeviceSize size, void* data = nullptr);
	VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin);
	void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free);
	VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	void clean();


	VkPhysicalDevice physicalDevice;

	VkDevice device;

	VkQueue graphicsQueue;

	uint32_t queueFamily = -1;

	VkPhysicalDeviceProperties properties;
	/** @brief Features of the physical device that an application can use to check if a feature is supported */
	VkPhysicalDeviceFeatures features;
	/** @brief Features that have been enabled for use on the physical device */
	VkPhysicalDeviceFeatures enabledFeatures;
	/** @brief Memory types and heaps of the physical device */
	VkPhysicalDeviceMemoryProperties memoryProperties;
	/** @brief Queue family properties of the physical device */
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	/** @brief List of extensions supported by the device */
	std::vector<std::string> supportedExtensions;

	VkCommandPool commandPool = VK_NULL_HANDLE;

};

#endif