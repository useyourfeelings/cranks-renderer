#include"vulkan_swapchain.h"

VulkanSwapchain::VulkanSwapchain(std::shared_ptr<VulkanDevice> new_device, VkInstance new_instance, GLFWwindow* window) {

    this->device = new_device;
    this->instance = new_instance;

    VkResult err;

    // wsi
        // 
        // surface

        /*VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
        surfaceCreateInfo.hwnd = (HWND)platformWindow;
        err = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);*/
    {

        auto res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (res != VK_SUCCESS) {
            std::cout << res << std::endl;
            throw std::runtime_error("failed to create window surface!");
        }

        // Check for WSI support

        // 这里估计一般是support的，刚好能成功。正规的需要遍历queue来挑选。
        // 所以graphicsQueue == presentQueue
        VkBool32 support;
        vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice, device->queueFamily, surface, &support);
        if (support != VK_TRUE) {
            throw std::runtime_error("Error no WSI support on physical device");
        }

        
    }

    // surface cap/format/mode

    

    {

        err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physicalDevice, surface, &cap);
        if (err != VK_SUCCESS) {
            throw std::runtime_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR fail");
        }

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device->physicalDevice, surface, &formatCount, nullptr);

        std::vector<VkSurfaceFormatKHR> formats;

        if (formatCount != 0) {
            formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device->physicalDevice, surface, &formatCount, formats.data());
        }

        std::vector<VkPresentModeKHR> presentModes;

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice, surface, &presentModeCount, presentModes.data());
        }

        // choose

        surfaceFormat = formats[0];
        std::cout << "formatCount " << formatCount << std::endl;
        for (auto format : formats) {
            std::cout << "format " << format.format << " color space  " << format.colorSpace << std::endl;
            if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB) {
                surfaceFormat = format;
            }
        }

        std::cout << "presentModeCount " << presentModeCount << std::endl;
        for (auto mode : presentModes) {
            std::cout << "mode " << mode << std::endl;
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                surfaceMode = VK_PRESENT_MODE_MAILBOX_KHR;
            }
        }
    }

    // swapchain
    glfwGetFramebufferSize(window, &width, &height);

    imageCount = std::min(cap.minImageCount + 1, cap.maxImageCount);

    {
        VkSwapchainKHR oldSwapchain = swapChain;

        VkSwapchainCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = surface;
        info.minImageCount = imageCount;
        info.imageFormat = surfaceFormat.format;
        info.imageColorSpace = surfaceFormat.colorSpace;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
        info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = surfaceMode;
        info.clipped = VK_TRUE;
        info.oldSwapchain = oldSwapchain;

        if (cap.currentExtent.width == 0xffffffff)
        {
            info.imageExtent.width = width;
            info.imageExtent.height = height;
        }
        else
        {
            info.imageExtent.width = width = cap.currentExtent.width;
            info.imageExtent.height = height = cap.currentExtent.height;
        }
        err = vkCreateSwapchainKHR(device->device, &info, nullptr, &swapChain);
        if (err != VK_SUCCESS) {
            throw std::runtime_error("vkCreateSwapchainKHR fail");
        }

        err = vkGetSwapchainImagesKHR(device->device, swapChain, &imageCount, NULL);
        if (err != VK_SUCCESS) {
            throw std::runtime_error("vkGetSwapchainImagesKHR fail");
        }

        std::cout << "imageCount = " << imageCount << std::endl;

        images.resize(imageCount);
        err = vkGetSwapchainImagesKHR(device->device, swapChain, &imageCount, images.data());
        if (err != VK_SUCCESS) {
            throw std::runtime_error("vkGetSwapchainImagesKHR fail");
        }

    }

    // image view

    std::cout << "create image view" << std::endl;
    {
        imageViews.resize(imageCount);

        for (uint32_t i = 0; i < imageCount; i++)
        {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.pNext = NULL;
            createInfo.format = surfaceFormat.format;
            createInfo.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A
            };
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.flags = 0;

            createInfo.image = images[i];

            err = vkCreateImageView(device->device, &createInfo, nullptr, &imageViews[i]);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateImageView fail");
            }
        }
    }

}

void VulkanSwapchain::clean() {
    std::cout << "VulkanSwapchain.clean()" << std::endl;
    for (auto view : imageViews) {
        vkDestroyImageView(device->device, view, nullptr);
    }

    vkDestroySwapchainKHR(device->device, swapChain, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
}