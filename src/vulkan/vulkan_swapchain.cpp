#include"vulkan_swapchain.h"

VulkanSwapchain::VulkanSwapchain(std::shared_ptr<VulkanDevice> device, VkInstance instance, GLFWwindow* window):
    device(device),
    instance(instance),
    window(window){

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
        err = vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice, device->queueFamily, surface, &support);
        if (support != VK_TRUE) {
            throw std::runtime_error("Error no WSI support on physical device");
        }


    }

    // surface cap/format/mode

    {
        /*err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physicalDevice, surface, &cap);
        if (err != VK_SUCCESS) {
            throw std::runtime_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR fail");
        }*/

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

            // VK_FORMAT_B8G8R8A8_SRGB 颜色会变浅
            if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_R8G8B8A8_UNORM) { // VK_FORMAT_R8G8B8A8_UNORM VK_FORMAT_B8G8R8A8_SRGB
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

}

void VulkanSwapchain::Create() {
    VkResult err;

    // swapchain
    glfwGetFramebufferSize(window, &width, &height);

    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkResult.html
    // resize时重建swapchain。必须调一下这个。否则永远报VK_ERROR_OUT_OF_DATE_KHR。
    {
        err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physicalDevice, surface, &cap);
        if (err != VK_SUCCESS) {
            throw std::runtime_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR fail");
        }
    }

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

        VK_CHECK_RESULT(vkCreateSwapchainKHR(device->device, &info, nullptr, &swapChain));

        std::cout << "old swapChain = " << oldSwapchain << " new swapChain = "<< swapChain << std::endl;

        // delete old 

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            for (auto view : imageViews) {
                vkDestroyImageView(device->device, view, nullptr);
            }
            vkDestroySwapchainKHR(device->device, oldSwapchain, nullptr);
        }

        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device->device, swapChain, &imageCount, NULL));

        std::cout << "imageCount = " << imageCount << std::endl;

        images.resize(imageCount);
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device->device, swapChain, &imageCount, images.data()));

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

            VK_CHECK_RESULT(vkCreateImageView(device->device, &createInfo, nullptr, &imageViews[i]));
        }
    }
}

void VulkanSwapchain::Clean() {
    std::cout << "VulkanSwapchain.clean()" << std::endl;
    for (auto view : imageViews) {
        vkDestroyImageView(device->device, view, nullptr);
    }

    vkDestroySwapchainKHR(device->device, swapChain, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
}