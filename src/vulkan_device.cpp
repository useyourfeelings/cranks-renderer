#include"vulkan_device.h"

VulkanDevice::VulkanDevice(VkInstance instance) {

    VkResult err;

    // physical device
    {
        uint32_t gpuCount = 0;
        // Get number of available physical devices
        vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
        if (gpuCount == 0) {
            throw std::runtime_error("no device");
        }

        // Enumerate devices
        std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
        err = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
        if (err) {
            throw std::runtime_error("vkEnumeratePhysicalDevices error");
        }

        // select gpu
        for (auto device : physicalDevices) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            std::cout << properties.apiVersion << " " << properties.limits.maxComputeSharedMemorySize << " " << properties.deviceName << " "
                << properties.deviceType << " " << properties.limits.maxMemoryAllocationCount << std::endl;
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                this->physicalDevice = device;
                break;
            }
        }

    }

    // physical prop
    {
        // Store Properties features, limits and properties of the physical device for later use
        // Device properties also contain limits and sparse properties
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        // Features should be checked by the examples before using them
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);
        // Memory properties are used regularly for creating all kinds of buffers
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    }
    

    // Select graphics queue family
    {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, NULL);
        //VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
        std::vector<VkQueueFamilyProperties> queues(count);

        std::cout << "gpu queue family count = " << count << std::endl;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queues.data());
        for (uint32_t i = 0; i < count; i++) {
            std::cout << "gpu queue family " << queues[i].queueFlags << " queue count " << queues[i].queueCount << std::endl;

            // my family
            // flag 1111 1100 1110
            // count 16   2    8

            /*VK_QUEUE_GRAPHICS_BIT = 0x00000001,
            VK_QUEUE_COMPUTE_BIT = 0x00000002,
            VK_QUEUE_TRANSFER_BIT = 0x00000004,
            VK_QUEUE_SPARSE_BINDING_BIT = 0x00000008,*/
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                queueFamily = i;
                //break;
            }
        }

        //free(queues);
        //IM_ASSERT(g_QueueFamily != (uint32_t)-1);
    }

    // logical device
    {
        int device_extension_count = 1;
        const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME }; // "VK_KHR_swapchain"
        const float queue_priority[] = { 1.0f };
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = queueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = device_extension_count;
        create_info.ppEnabledExtensionNames = device_extensions;
        err = vkCreateDevice(physicalDevice, &create_info, nullptr, &device);
        //check_vk_result(err);
        vkGetDeviceQueue(device, queueFamily, 0, &graphicsQueue);
    }

    // Create a default command pool for graphics command buffers
    commandPool = createCommandPool(queueFamily);


}

VulkanDevice::~VulkanDevice()
{
    std::cout << "~VulkanDevice()" << std::endl;

    
}

void VulkanDevice::clean() {
    std::cout << "VulkanDevice.clean()" << std::endl;

    if (commandPool)
    {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

    if (device)
    {
        vkDestroyDevice(device, nullptr);
    }
}

uint32_t VulkanDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound) const
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    { 
        if ((typeBits & 1) == 1)
        {
            if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                if (memTypeFound)
                {
                    *memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }

    if (memTypeFound)
    {
        *memTypeFound = false;
        return 0;
    }
    else
    {
        throw std::runtime_error("Could not find a matching memory type");
    }
}

VkCommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
    cmdPoolInfo.flags = createFlags;
    VkCommandPool cmdPool;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
    return cmdPool;
}



VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VulkanBuffer* buffer, VkDeviceSize size, void* data)
{
    buffer->device = this->device;

    // Create the buffer handle
    VkBufferCreateInfo bufferCreateInfo = vktool::bufferCreateInfo(usageFlags, size);
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer->buffer));

    // Create the memory backing up the buffer handle
    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo memAlloc = vktool::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(device, buffer->buffer, &memReqs);

    memAlloc.allocationSize = memReqs.size;
    // Find a memory type index that fits the properties of the buffer
    memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
    // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        memAlloc.pNext = &allocFlagsInfo;
    }
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &buffer->memory));

    buffer->alignment = memReqs.alignment;
    buffer->size = size;
    buffer->usageFlags = usageFlags;
    buffer->memoryPropertyFlags = memoryPropertyFlags;

    // If a pointer to the buffer data has been passed, map the buffer and copy over the data
    if (data != nullptr)
    {
        VK_CHECK_RESULT(buffer->map());
        memcpy(buffer->mapped, data, size);
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            buffer->flush();

        buffer->unmap();
    }

    // Initialize a default descriptor that covers the whole buffer size
    buffer->setupDescriptor();

    // Attach the memory to the buffer object
    return buffer->bind();
}


VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, bool begin)
{
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = vktool::commandBufferAllocateInfo(commandPool, level, 1);
    VkCommandBuffer cmdBuffer;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));
    // If requested, also start recording for the new command buffer
    if (begin)
    {
        VkCommandBufferBeginInfo cmdBufInfo = vktool::commandBufferBeginInfo();
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }
    return cmdBuffer;
}

void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
    if (commandBuffer == VK_NULL_HANDLE)
    {
        return;
    }

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = vktool::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo = vktool::fenceCreateInfo(VK_FLAGS_NONE);
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
    // Submit to the queue
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
    vkDestroyFence(device, fence, nullptr);
    if (free)
    {
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
}