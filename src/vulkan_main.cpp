#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<cassert>

// https://www.glfw.org/docs/latest/vulkan_guide.html
//#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}


class VulkanApp {
public:
    VulkanApp() {}

    int init() {
        std::cout << "VulkanApp init\n";

        if (!glfwInit())
            return 1;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(800, 600, "Cranks Renderer Vulkan", nullptr, nullptr);

        VkResult err;

        // extension
        // 
        //std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
        //{
        //    std::cout << "get extension\n";

        //    // Get extensions supported by the instance and store for later use
        //    uint32_t extCount = 0;
        //    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
        //    if (extCount > 0)
        //    {
        //        std::vector<VkExtensionProperties> extensions(extCount);
        //        if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
        //        {
        //            for (VkExtensionProperties extension : extensions)
        //            {
        //                std::cout << extension.extensionName << std::endl;
        //                supportedInstanceExtensions.push_back(extension.extensionName);
        //            }
        //        }
        //    }

        //    // 可在此之前按需定制enabledInstanceExtensions

        //    // Enabled requested instance extensions
        //    if (enabledInstanceExtensions.size() > 0)
        //    {
        //        for (auto enabledExtension : enabledInstanceExtensions)
        //        {
        //            // Output message if requested extension is not available
        //            if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension) == supportedInstanceExtensions.end())
        //            {
        //                std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
        //            }
        //            instanceExtensions.push_back(enabledExtension);
        //        }
        //    }
        //}

        //std::vector<const char*> instanceExtensions = { };
        uint32_t extensions_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
        //auto exs = glfwGetRequiredInstanceExtensions(&extensions_count);

        // add validation
        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + extensions_count);
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // "VK_EXT_debug_utils"

        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        
        // instance
        {
            VkApplicationInfo appInfo;
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Cranks Renderer";
            appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
            appInfo.pEngineName = "wtf Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
            appInfo.apiVersion = VK_API_VERSION_1_0;
            appInfo.pNext = nullptr;

            VkInstanceCreateInfo instanceCreateInfo{};
            instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceCreateInfo.pApplicationInfo = &appInfo;

            if(extensions.size() > 0)
            {
                instanceCreateInfo.enabledExtensionCount = extensions.size();// extensions_count;//(uint32_t)instanceExtensions.size();
                instanceCreateInfo.ppEnabledExtensionNames = extensions.data();// instanceExtensions.data();
            }

            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

            if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
                throw std::runtime_error("failed to create instance!");
            }
        }


        // setup debug
        {

            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;
            createInfo.pUserData = nullptr; // Optional

            vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
            vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

            VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
            debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
            debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugUtilsMessengerCI.pfnUserCallback = debugCallback;
            err = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCI, nullptr, &debugUtilsMessenger);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateDebugUtilsMessengerEXT fail");
            }
        }

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
            for (auto device: physicalDevices) {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);

                std::cout << properties.apiVersion << " " << properties.limits.maxComputeSharedMemorySize << " " << properties.deviceName << " "
                    << properties.deviceType << " " << properties.limits.maxMemoryAllocationCount << std::endl;
                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    this->gpu = device;
                    break;
                }
            }

        }

        // Select graphics queue family
        uint32_t queueFamily = -1;
        {
            uint32_t count;
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, NULL);
            //VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
            std::vector<VkQueueFamilyProperties> queues(count);

            std::cout << "gpu queue family count = " << count << std::endl;
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, queues.data());
            for (uint32_t i = 0; i < count; i++) {
                std::cout << "gpu queue family " << queues[i].queueFlags<<" queue count "<< queues[i].queueCount << std::endl;

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
            err = vkCreateDevice(gpu, &create_info, nullptr, &logicalGPU);
            //check_vk_result(err);
            vkGetDeviceQueue(logicalGPU, queueFamily, 0, &graphicsQueue);
        }

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
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queueFamily, surface, &support);
            if (support != VK_TRUE) {
                throw std::runtime_error("Error no WSI support on physical device");
            }

            vkGetDeviceQueue(logicalGPU, queueFamily, 0, &presentQueue);
        }

        // surface cap/format/mode

        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR surfaceMode = VK_PRESENT_MODE_FIFO_KHR;

        VkSurfaceCapabilitiesKHR cap;

        {
            
            err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &cap);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR fail");
            }

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr);

            std::vector<VkSurfaceFormatKHR> formats;

            if (formatCount != 0) {
                formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, formats.data());
            }

            std::vector<VkPresentModeKHR> presentModes;

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, nullptr);

            if (presentModeCount != 0) {
                presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, presentModes.data());
            }

            // choose

            surfaceFormat = formats[0];
            std::cout << "formatCount " << formatCount << std::endl;
            for (auto format : formats){
                std::cout << "format "<< format.format << " color space  " << format.colorSpace << std::endl;
                if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB) {
                    surfaceFormat = format;
                }
            }
                
            std::cout << "presentModeCount "<< presentModeCount << std::endl;
            for (auto mode : presentModes) {
                std::cout << "mode " << mode << std::endl;
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    surfaceMode = VK_PRESENT_MODE_MAILBOX_KHR;
                }
            }
        }

        // swapchain
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        auto imageCount = std::min(cap.minImageCount + 1, cap.maxImageCount);

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
            err = vkCreateSwapchainKHR(logicalGPU, &info, nullptr, &swapChain);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateSwapchainKHR fail");
            }

            err = vkGetSwapchainImagesKHR(logicalGPU, swapChain, &imageCount, NULL);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkGetSwapchainImagesKHR fail");
            }

            std::cout << "imageCount = "<< imageCount << std::endl;

            swapChainImages.resize(imageCount);
            err = vkGetSwapchainImagesKHR(logicalGPU, swapChain, &imageCount, swapChainImages.data());
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkGetSwapchainImagesKHR fail");
            }

        }

        // image view

        std::cout << "create image view" << std::endl;
        {
            swapChainImageViews.resize(imageCount);

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

                createInfo.image = swapChainImages[i];

                err = vkCreateImageView(logicalGPU, &createInfo, nullptr, &swapChainImageViews[i]);
                if (err != VK_SUCCESS) {
                    throw std::runtime_error("vkCreateImageView fail");
                }
            }
        }


        // shader module
        /*if(0)
        {
            VkShaderModuleCreateInfo vert_info = {};
            vert_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            vert_info.codeSize = sizeof(__glsl_shader_vert_spv);
            vert_info.pCode = (uint32_t*)__glsl_shader_vert_spv;
            err = vkCreateShaderModule(logicalGPU, &vert_info, nullptr, &vertShader);
            if (err != VK_SUCCESS || vertShader == VK_NULL_HANDLE) {
                throw std::runtime_error("vkCreateShaderModule fail");
            }

            VkShaderModuleCreateInfo frag_info = {};
            frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            frag_info.codeSize = sizeof(__glsl_shader_frag_spv);
            frag_info.pCode = (uint32_t*)__glsl_shader_frag_spv;
            err = vkCreateShaderModule(logicalGPU, &frag_info, nullptr, &fragShader);
            if (err != VK_SUCCESS || fragShader == VK_NULL_HANDLE) {
                throw std::runtime_error("vkCreateShaderModule fail");
            }

        }*/

        // pipeline

        struct Vertex {
            float position[3];
            float color[3];
        };

        {
            VkPipelineShaderStageCreateInfo stage[2] = {};
            stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            stage[0].module = vertShader = loadSPIRVShader("resource/shader/glsl/triangle_vert.spv");// vertShader;
            stage[0].pName = "main";
            stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            stage[1].module = fragShader = loadSPIRVShader("resource/shader/glsl/triangle_frag.spv");// fragShader;
            stage[1].pName = "main";


            VkVertexInputBindingDescription binding_desc = {};
            binding_desc.binding = 0;
            binding_desc.stride = sizeof(Vertex);
            binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            // Input attribute bindings describe shader attribute locations and memory layouts
            std::vector<VkVertexInputAttributeDescription> vertexInputAttributs(2);
            // These match the following shader layout (see triangle.vert):
            //	layout (location = 0) in vec3 inPos;
            //	layout (location = 1) in vec3 inColor;
            // Attribute location 0: Position
            vertexInputAttributs[0].binding = 0;
            vertexInputAttributs[0].location = 0;
            // Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
            vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertexInputAttributs[0].offset = offsetof(Vertex, position);
            // Attribute location 1: Color
            vertexInputAttributs[1].binding = 0;
            vertexInputAttributs[1].location = 1;
            // Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
            vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertexInputAttributs[1].offset = offsetof(Vertex, color);


            VkPipelineVertexInputStateCreateInfo vertex_info = {};
            vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_info.vertexBindingDescriptionCount = 0;// 1;
            vertex_info.pVertexBindingDescriptions = nullptr;// &binding_desc;// nullptr;// binding_desc;
            vertex_info.vertexAttributeDescriptionCount = 0;// 2;
            vertex_info.pVertexAttributeDescriptions = nullptr;// vertexInputAttributs.data();// nullptr;//attribute_desc;

            VkPipelineInputAssemblyStateCreateInfo ia_info = {};
            ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            VkPipelineViewportStateCreateInfo viewport_info = {};
            viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_info.viewportCount = 1;
            viewport_info.scissorCount = 1;

            VkPipelineRasterizationStateCreateInfo raster_info = {};
            raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            raster_info.polygonMode = VK_POLYGON_MODE_FILL;
            raster_info.cullMode = VK_CULL_MODE_NONE;
            raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            raster_info.lineWidth = 1.0f;

            VkPipelineMultisampleStateCreateInfo ms_info = {};
            ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // (MSAASamples != 0) ? MSAASamples : VK_SAMPLE_COUNT_1_BIT;

            VkPipelineColorBlendAttachmentState color_b_attachment[1] = {};
            color_b_attachment[0].blendEnable = VK_TRUE;
            color_b_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            color_b_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            color_b_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
            color_b_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_b_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            color_b_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
            color_b_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

            VkPipelineDepthStencilStateCreateInfo depth_info = {};
            depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

            VkPipelineColorBlendStateCreateInfo blend_info = {};
            blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            blend_info.attachmentCount = 1;
            blend_info.pAttachments = color_b_attachment;

            std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            VkPipelineDynamicStateCreateInfo dynamic_state = {};
            dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state.dynamicStateCount = (uint32_t)dynamicStateEnables.size();
            dynamic_state.pDynamicStates = dynamicStateEnables.data();


            // render pass
            VkAttachmentDescription attachment = {};
            attachment.format = surfaceFormat.format;
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;// wd->ClearEnable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference color_attachment = {};
            color_attachment.attachment = 0;
            color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color_attachment;

            VkSubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo rpInfo = {};
            rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            rpInfo.attachmentCount = 1;
            rpInfo.pAttachments = &attachment;
            rpInfo.subpassCount = 1;
            rpInfo.pSubpasses = &subpass;
            rpInfo.dependencyCount = 1;
            rpInfo.pDependencies = &dependency;
            err = vkCreateRenderPass(logicalGPU, &rpInfo, nullptr, &renderPass);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateRenderPass fail");
            }

            std::cout << "renderPass "<< renderPass << std::endl;

            // pipeline layout

            VkDescriptorSetLayoutBinding layoutBinding[2] = {};
            layoutBinding[0].binding = 0;
            layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;// VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            layoutBinding[0].descriptorCount = 1;
            layoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;// VK_SHADER_STAGE_FRAGMENT_BIT;// VK_SHADER_STAGE_VERTEX_BIT;
            layoutBinding[0].pImmutableSamplers = nullptr;

            //layoutBinding[1].binding = 1;
            //layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;// VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            //layoutBinding[1].descriptorCount = 1;
            //layoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;// VK_SHADER_STAGE_FRAGMENT_BIT;// VK_SHADER_STAGE_VERTEX_BIT;
            //layoutBinding[1].pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
            descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorLayout.pNext = nullptr;
            descriptorLayout.bindingCount = 1;
            descriptorLayout.pBindings = layoutBinding;

            err = vkCreateDescriptorSetLayout(logicalGPU, &descriptorLayout, nullptr, &descriptorSetLayout);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateDescriptorSetLayout fail");
            }

            VkPushConstantRange push_constants[1] = {};
            push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            push_constants[0].offset = sizeof(float) * 0;
            push_constants[0].size = sizeof(float) * 4;

            VkPipelineLayoutCreateInfo layout_info = {};
            layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layout_info.setLayoutCount = 1;
            layout_info.pSetLayouts = &descriptorSetLayout;
            layout_info.pushConstantRangeCount = 1;
            layout_info.pPushConstantRanges = push_constants;
            err = vkCreatePipelineLayout(logicalGPU, &layout_info, nullptr, &pipelineLayout);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreatePipelineLayout fail");
            }

            // pipeline
            VkGraphicsPipelineCreateInfo pplInfo = {};
            pplInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pplInfo.flags = 0;// bd->PipelineCreateFlags;
            pplInfo.stageCount = 2;
            pplInfo.pStages = stage;
            pplInfo.pVertexInputState = &vertex_info;
            pplInfo.pInputAssemblyState = &ia_info;
            pplInfo.pViewportState = &viewport_info;
            pplInfo.pRasterizationState = &raster_info;
            pplInfo.pMultisampleState = &ms_info;
            pplInfo.pDepthStencilState = &depth_info;
            pplInfo.pColorBlendState = &blend_info;
            pplInfo.pDynamicState = &dynamic_state;
            pplInfo.layout = pipelineLayout;
            pplInfo.renderPass = renderPass;
            pplInfo.subpass = 0;// subpass;
            err = vkCreateGraphicsPipelines(logicalGPU, VK_NULL_HANDLE, 1, &pplInfo, nullptr, &graphicsPipeline);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateGraphicsPipelines fail");
            }

        }

        // sync
        {
            // Semaphores (Used for correct command ordering)
            VkSemaphoreCreateInfo semaphoreCreateInfo = {};
            semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreCreateInfo.pNext = nullptr;

            // Semaphore used to ensure that image presentation is complete before starting to submit again
            err = vkCreateSemaphore(logicalGPU, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateSemaphore fail");
            }

            // Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
            err = vkCreateSemaphore(logicalGPU, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateSemaphore fail");
            }

            // Fences (Used to check draw command buffer completion)
            VkFenceCreateInfo fenceCreateInfo = {};
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            // Create in signaled state so we don't wait on first render of each command buffer
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            waitFences.resize(imageCount);
            for (auto& fence : waitFences)
            {
                std::cout << "vkCreateFence"<< std::endl;
                err = vkCreateFence(logicalGPU, &fenceCreateInfo, nullptr, &fence);
                if (err != VK_SUCCESS) {
                    throw std::runtime_error("vkCreateFence fail");
                }
            }
        }

        // framebuffer
        {
            VkImageView attachment[1];
            VkFramebufferCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.renderPass = renderPass;
            info.attachmentCount = 1;
            info.pAttachments = attachment;
            info.width = width;
            info.height = height;
            info.layers = 1;

            frameBuffers.resize(imageCount);

            for (uint32_t i = 0; i < imageCount; i++)
            {
                attachment[0] = swapChainImageViews[i];

                err = vkCreateFramebuffer(logicalGPU, &info, nullptr, &frameBuffers[i]);
                if (err != VK_SUCCESS) {
                    throw std::runtime_error("vkCreateFramebuffer fail");
                }
            }
        }

        // command pool
        {
            VkCommandPoolCreateInfo cmdPoolInfo = {};
            cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmdPoolInfo.queueFamilyIndex = queueFamily;
            cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            err = vkCreateCommandPool(logicalGPU, &cmdPoolInfo, nullptr, &cmdPool);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateCommandPool fail");
            }
        }

        // command buffer
        {
            cmdBuffers.resize(imageCount);

            VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
            commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocateInfo.commandPool = cmdPool;
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = imageCount;

            err = vkAllocateCommandBuffers(logicalGPU, &commandBufferAllocateInfo, cmdBuffers.data());
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkAllocateCommandBuffers fail");
            }
        }

        // recording
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            VkClearValue clearValues[2];
            clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
            clearValues[1].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.pNext = nullptr;
            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.renderArea.offset.x = 0;
            renderPassBeginInfo.renderArea.offset.y = 0;
            renderPassBeginInfo.renderArea.extent.width = width;
            renderPassBeginInfo.renderArea.extent.height = height;
            renderPassBeginInfo.clearValueCount = 2;
            renderPassBeginInfo.pClearValues = clearValues;

            for (int32_t i = 0; i < cmdBuffers.size(); ++i) {
                std::cout << "vkBeginCommandBuffer " << i << std::endl;

                if (vkBeginCommandBuffer(cmdBuffers[i], &beginInfo) != VK_SUCCESS) {
                    throw std::runtime_error("vkBeginCommandBuffer fail!");
                }

                renderPassBeginInfo.framebuffer = frameBuffers[i];

                // Start the first sub pass specified in our default render pass setup by the base class
                // This will clear the color and depth attachment
                vkCmdBeginRenderPass(cmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Update dynamic viewport state
                VkViewport viewport = {};
                viewport.height = (float)height;
                viewport.width = (float)width;
                viewport.minDepth = (float)0.0f;
                viewport.maxDepth = (float)1.0f;
                vkCmdSetViewport(cmdBuffers[i], 0, 1, &viewport);

                // Update dynamic scissor state
                VkRect2D scissor = {};
                scissor.extent.width = width;
                scissor.extent.height = height;
                scissor.offset.x = 0;
                scissor.offset.y = 0;
                vkCmdSetScissor(cmdBuffers[i], 0, 1, &scissor);

                vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

                vkCmdDraw(cmdBuffers[i], 3, 1, 0, 0);

                vkCmdEndRenderPass(cmdBuffers[i]);

                if (vkEndCommandBuffer(cmdBuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("vkEndCommandBuffer fail");
                }
            }
        }




        return 0;
    }

    int draw() {
        // Get next image in the swap chain (back/front buffer)
        VK_CHECK_RESULT(vkAcquireNextImageKHR(logicalGPU, swapChain, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, &currentBuffer));

        // Use a fence to wait until the command buffer has finished execution before using it again
        VK_CHECK_RESULT(vkWaitForFences(logicalGPU, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
        VK_CHECK_RESULT(vkResetFences(logicalGPU, 1, &waitFences[currentBuffer]));

        // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        // The submit info structure specifies a command buffer queue submission batch
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask = &waitStageMask;               // Pointer to the list of pipeline stages that the semaphore waits will occur at
        submitInfo.pWaitSemaphores = &presentCompleteSemaphore;      // Semaphore(s) to wait upon before the submitted command buffer starts executing
        submitInfo.waitSemaphoreCount = 1;                           // One wait semaphore
        submitInfo.pSignalSemaphores = &renderCompleteSemaphore;     // Semaphore(s) to be signaled when command buffers have completed
        submitInfo.signalSemaphoreCount = 1;                         // One signal semaphore
        submitInfo.pCommandBuffers = &cmdBuffers[currentBuffer]; // Command buffers(s) to execute in this batch (submission)
        submitInfo.commandBufferCount = 1;                           // One command buffer

        // Submit to the graphics queue passing a wait fence
        VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, waitFences[currentBuffer]));

        // Present the current buffer to the swap chain
        // Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
        // This ensures that the image is not presented to the windowing system until all commands have been submitted

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = NULL;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &currentBuffer;
        // Check if a wait semaphore has been specified to wait for before presenting the image
        if (renderCompleteSemaphore != VK_NULL_HANDLE)
        {
            presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
            presentInfo.waitSemaphoreCount = 1;
        }

        VkResult present = vkQueuePresentKHR(graphicsQueue, &presentInfo);
        if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR))) {
            VK_CHECK_RESULT(present);
        }

        return 0;
    }

    int loop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            draw();

            vkDeviceWaitIdle(logicalGPU);
        }



        return 0;
    }

    int clean() {
        std::cout << "VulkanApp clean"<< std::endl;

        vkFreeCommandBuffers(logicalGPU, cmdPool, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
        vkDestroyCommandPool(logicalGPU, cmdPool, nullptr);
        
        for (auto framebuffer : frameBuffers) {
            vkDestroyFramebuffer(logicalGPU, framebuffer, nullptr);
        }

        vkDestroyDescriptorSetLayout(logicalGPU, descriptorSetLayout, nullptr);

        vkDestroyPipeline(logicalGPU, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(logicalGPU, pipelineLayout, nullptr);

        vkDestroyRenderPass(logicalGPU, renderPass, nullptr);

        for (auto& fence : waitFences) {
            vkDestroyFence(logicalGPU, fence, nullptr);
        }

        vkDestroySemaphore(logicalGPU, presentCompleteSemaphore, nullptr);
        vkDestroySemaphore(logicalGPU, renderCompleteSemaphore, nullptr);

        vkDestroyShaderModule(logicalGPU, fragShader, nullptr);
        vkDestroyShaderModule(logicalGPU, vertShader, nullptr);

        for (auto view : swapChainImageViews) {
            vkDestroyImageView(logicalGPU, view, nullptr);
        }

        vkDestroySwapchainKHR(logicalGPU, swapChain, nullptr);

        vkDestroySurfaceKHR(instance, surface, nullptr);

        vkDestroyDevice(logicalGPU, nullptr);

        vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();

        return 0;
    }

    VkShaderModule loadSPIRVShader(std::string filename)
    {
        std::cout << "loadSPIRVShader " << filename << std::endl;
        size_t shaderSize;
        char* shaderCode = NULL;

        std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

        if (is.is_open())
        {
            shaderSize = is.tellg();
            is.seekg(0, std::ios::beg);
            // Copy file contents into a buffer
            shaderCode = new char[shaderSize];
            is.read(shaderCode, shaderSize);
            is.close();
            assert(shaderSize > 0);
        }

        if (shaderCode)
        {
            // Create a new shader module that will be used for pipeline creation
            VkShaderModuleCreateInfo moduleCreateInfo{};
            moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleCreateInfo.codeSize = shaderSize;
            moduleCreateInfo.pCode = (uint32_t*)shaderCode;

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(logicalGPU, &moduleCreateInfo, NULL, &shaderModule)) {
                throw std::runtime_error("vkCreateShaderModule fail");
            }

            delete[] shaderCode;

            return shaderModule;
        }
        else
        {
            std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
            return VK_NULL_HANDLE;
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
    VkDebugUtilsMessengerEXT debugUtilsMessenger;

    std::vector<std::string> supportedInstanceExtensions;
    std::vector<const char*> enabledInstanceExtensions;

    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice logicalGPU;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    VkShaderModule vertShader;
    VkShaderModule fragShader;

    VkDescriptorSetLayout descriptorSetLayout;


    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkRenderPass renderPass = VK_NULL_HANDLE;

    std::vector<VkFramebuffer>frameBuffers;
    // Active frame buffer index
    uint32_t currentBuffer = 0;

    VkCommandPool cmdPool;
    //VkCommandBuffer commandBuffer;
    std::vector<VkCommandBuffer> cmdBuffers;

    VkSemaphore presentCompleteSemaphore;
    VkSemaphore renderCompleteSemaphore;

    std::vector<VkFence> waitFences;

    ////uint32_t width = 1280;
    //uint32_t height = 720;

    // Stores physical device properties (for e.g. checking device limits)
    VkPhysicalDeviceProperties gpuProperties;
    // Stores the features available on the selected physical device (for e.g. checking if a feature is available)
    VkPhysicalDeviceFeatures gpuFeatures;
    // Stores all available memory (type) properties for the physical device
    VkPhysicalDeviceMemoryProperties gpuMemoryProperties;

    
};

int vulkan_main()
{
    std::cout<<"vulkan_main start"<<std::endl;

    VulkanApp app;

    app.init();
    app.loop();
    app.clean();

    std::cout << "vulkan_main over" << std::endl;
    return 0;
}