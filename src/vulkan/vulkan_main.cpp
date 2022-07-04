#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<memory>
#include<cassert>
#include <chrono>

// https://www.glfw.org/docs/latest/vulkan_guide.html
//#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include"vulkan_device.h"
#include"vulkan_swapchain.h"
#include"vulkan_ui.h"
#include"vulkan_tool.h"


class VulkanApp {
public:
    VulkanApp() {}

    int init() {
        std::cout << "VulkanApp init\n";

        if (!glfwInit())
            return 1;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        int width = 1200;
        int height = 1000;

        window = glfwCreateWindow(width, height, "Cranks Renderer Vulkan", nullptr, nullptr);

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

        device = std::make_shared<VulkanDevice>(instance);
        gpu = device->device;

        swapChain = std::make_shared<VulkanSwapchain>(device, instance, window);
        swapChain->Create();

        vkGetDeviceQueue(gpu, device->queueFamily, 0, &presentQueue);

        
        

        // pipeline

        struct Vertex {
            float position[3];
            float color[3];
        };

        {
            VkPipelineShaderStageCreateInfo stage[2] = {};
            stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            stage[0].module = vertShader = vktool::LoadSPIRVShader(gpu, "resource/shader/glsl/triangle.vert.spv");// vertShader;
            stage[0].pName = "main";
            stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            stage[1].module = fragShader = vktool::LoadSPIRVShader(gpu, "resource/shader/glsl/triangle.frag.spv");// fragShader;
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
            attachment.format = swapChain->surfaceFormat.format;
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
            err = vkCreateRenderPass(gpu, &rpInfo, nullptr, &renderPass);
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

            err = vkCreateDescriptorSetLayout(gpu, &descriptorLayout, nullptr, &descriptorSetLayout);
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
            err = vkCreatePipelineLayout(gpu, &layout_info, nullptr, &pipelineLayout);
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
            err = vkCreateGraphicsPipelines(gpu, VK_NULL_HANDLE, 1, &pplInfo, nullptr, &graphicsPipeline);
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
            err = vkCreateSemaphore(gpu, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateSemaphore fail");
            }

            // Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
            err = vkCreateSemaphore(gpu, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateSemaphore fail");
            }

            // Fences (Used to check draw command buffer completion)
            VkFenceCreateInfo fenceCreateInfo = {};
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            // Create in signaled state so we don't wait on first render of each command buffer
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            waitFences.resize(swapChain->imageCount);
            for (auto& fence : waitFences)
            {
                std::cout << "vkCreateFence"<< std::endl;
                err = vkCreateFence(gpu, &fenceCreateInfo, nullptr, &fence);
                if (err != VK_SUCCESS) {
                    throw std::runtime_error("vkCreateFence fail");
                }
            }
        }

        SetupFrameBuffer();

        // command pool
        {
            VkCommandPoolCreateInfo cmdPoolInfo = {};
            cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmdPoolInfo.queueFamilyIndex = device->queueFamily;
            cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            err = vkCreateCommandPool(gpu, &cmdPoolInfo, nullptr, &cmdPool);
            if (err != VK_SUCCESS) {
                throw std::runtime_error("vkCreateCommandPool fail");
            }
        }

        // The submit info structure specifies a command buffer queue submission batch
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask = &waitStageMask;               // Pointer to the list of pipeline stages that the semaphore waits will occur at
        submitInfo.pWaitSemaphores = &presentCompleteSemaphore;      // Semaphore(s) to wait upon before the submitted command buffer starts executing
        submitInfo.waitSemaphoreCount = 1;                           // One wait semaphore
        submitInfo.pSignalSemaphores = &renderCompleteSemaphore;     // Semaphore(s) to be signaled when command buffers have completed
        submitInfo.signalSemaphoreCount = 1;                         // One signal semaphore


        ui = std::make_shared<VulkanUI>(window, device, presentQueue, width, height);
        ui->InitResource();
        ui->SetupPipeline(renderPass);

        CreateCommandBuffers();

        //std::cout << "VulkanApp.BuildCommandBuffers() 1" << std::endl;
        //BuildCommandBuffers();

        prepared = true;

        this->renderImage.BuildImage(device, 256, 256);


        return 0;
    }

    int DestroyCommandBuffers() {
        vkFreeCommandBuffers(gpu, cmdPool, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());

        return 0;
    }

    int CreateCommandBuffers() {
        cmdBuffers.resize(swapChain->imageCount);

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = cmdPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = swapChain->imageCount;

        VK_CHECK_RESULT(vkAllocateCommandBuffers(gpu, &commandBufferAllocateInfo, cmdBuffers.data()));

        return 0;
    }

    void BuildCommandBuffers() {
        // recording
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;// 0; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            VkClearValue clearValues[2];
            clearValues[0].color = { { 0.6f, 0.8f, 0.7f, 1.0f } };
            clearValues[1].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.pNext = nullptr;
            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.renderArea.offset.x = 0;
            renderPassBeginInfo.renderArea.offset.y = 0;
            renderPassBeginInfo.renderArea.extent.width = swapChain->width;
            renderPassBeginInfo.renderArea.extent.height = swapChain->height;
            renderPassBeginInfo.clearValueCount = 2;
            renderPassBeginInfo.pClearValues = clearValues;

            for (int32_t i = 0; i < cmdBuffers.size(); ++i) {
                if (imageIndex != i) // 源代码这里不知道他为啥要每个cmdBuffer都一样跑一遍。难道不该指定index吗。我先改成index。
                    continue;

                //std::cout << "vkBeginCommandBuffer " << i << std::endl;

                VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffers[i], &beginInfo));

                renderPassBeginInfo.framebuffer = frameBuffers[i];

                // Start the first sub pass specified in our default render pass setup by the base class
                // This will clear the color and depth attachment
                vkCmdBeginRenderPass(cmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                // 各家的draw流程
                // 比如画三角形是一套流程，有自己的pipeline/shader/vertex buffer等等。
                // imgui又有自己的一套配置。

                if(1)
                {
                    // 三角形流程
                    // 
                    // Update dynamic viewport state
                    VkViewport viewport = {};
                    viewport.height = (float)swapChain->height;
                    viewport.width = (float)swapChain->width;
                    viewport.minDepth = (float)0.0f;
                    viewport.maxDepth = (float)1.0f;
                    vkCmdSetViewport(cmdBuffers[i], 0, 1, &viewport);

                    // Update dynamic scissor state
                    VkRect2D scissor = {};
                    scissor.extent.width = swapChain->width;
                    scissor.extent.height = swapChain->height;
                    scissor.offset.x = 0;
                    scissor.offset.y = 0;
                    vkCmdSetScissor(cmdBuffers[i], 0, 1, &scissor);

                    vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

                    vkCmdDraw(cmdBuffers[i], 3, 1, 0, 0);
                }

                {
                    // ui流程
                    ui->VulkanDraw(cmdBuffers[i]);
                }

                vkCmdEndRenderPass(cmdBuffers[i]);

                VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffers[i]));
            }
        }
    }

    void FrontendUI(uint32_t lastFPS, float frameDuration) {
        ui->FrontendDraw(lastFPS, frameDuration, (void *)renderImage.descriptorSet);

        if (ui->UpdateBuffer()) {// || UIOverlay.updated) {
            
            //UIOverlay.updated = false;
            //std::cout << "VulkanApp.BuildCommandBuffers() 2" << std::endl;
            //BuildCommandBuffers();
        }

        //BuildCommandBuffers(); // 总是刷新
    }

    int VulkanDraw() {
        // Get next image in the swap chain (back/front buffer)

        // 获取当前的image index
        VkResult res = vkAcquireNextImageKHR(gpu, swapChain->swapChain, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (res != VK_SUCCESS) {
            if (res == VK_ERROR_OUT_OF_DATE_KHR) {
                resized = true;
                std::cout << "VulkanApp resized 1" << std::endl;
                return 0;
                //OnWindowResize();
            }
            else {
                throw std::runtime_error("vkAcquireNextImageKHR fail");
            }

        }

        //std::cout << "VulkanApp.draw() imageIndex = "<< imageIndex << std::endl;

        // Use a fence to wait until the command buffer has finished execution before using it again
        VK_CHECK_RESULT(vkWaitForFences(gpu, 1, &waitFences[imageIndex], VK_TRUE, UINT64_MAX));
        VK_CHECK_RESULT(vkResetFences(gpu, 1, &waitFences[imageIndex]));

        BuildCommandBuffers();
        
        submitInfo.pCommandBuffers = &cmdBuffers[imageIndex];
        submitInfo.commandBufferCount = 1;

        // Submit to the graphics queue passing a wait fence
        VK_CHECK_RESULT(vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, waitFences[imageIndex]));

        // Present the current buffer to the swap chain
        // Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
        // This ensures that the image is not presented to the windowing system until all commands have been submitted

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = NULL;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain->swapChain;
        presentInfo.pImageIndices = &imageIndex;
        // Check if a wait semaphore has been specified to wait for before presenting the image
        if (renderCompleteSemaphore != VK_NULL_HANDLE)
        {
            presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
            presentInfo.waitSemaphoreCount = 1;
        }

        VkResult present = vkQueuePresentKHR(device->graphicsQueue, &presentInfo);
        if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR))) {
            if (present == VK_ERROR_OUT_OF_DATE_KHR) {
                resized = true;
                std::cout << "VulkanApp resized 2" << std::endl;
                //OnWindowResize();
            }
            else {
                VK_CHECK_RESULT(present);
            }
            
        }

        VK_CHECK_RESULT(vkQueueWaitIdle(device->graphicsQueue));

        return 0;
    }

    void OnWindowResize()
    {
        if (!prepared)
        {
            return;
        }
        prepared = false;
        //resized = true;

        std::cout << "VulkanApp.OnWindowResize()" << std::endl;

        // https://stackoverflow.com/questions/44719635/what-is-the-difference-between-glfwgetwindowsize-and-glfwgetframebuffersize
        // glfwGetWindowSize vs glfwGetFramebufferSize

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);  // glfwGetWindowSize(window, &width, &height);

        /*while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }*/

        // Ensure all operations on the device have been finished before destroying resources
        vkDeviceWaitIdle(gpu);

        // Recreate swap chain
        swapChain->Create();

        // Recreate the frame buffers
        //vkDestroyImageView(device, depthStencil.view, nullptr);
        //vkDestroyImage(device, depthStencil.image, nullptr);
        //vkFreeMemory(device, depthStencil.mem, nullptr);
        //setupDepthStencil();

        DestroyFrameBuffer();
        SetupFrameBuffer();

        if ((width > 0.0f) && (height > 0.0f)) {
            ui->Resize(width, height);
        }

        // Command buffers need to be recreated as they may store
        // references to the recreated frame buffer
        DestroyCommandBuffers();
        CreateCommandBuffers();
        imageIndex = 0;
        //BuildCommandBuffers();

        vkDeviceWaitIdle(gpu);

        /*if ((width > 0.0f) && (height > 0.0f)) {
            camera.updateAspectRatio((float)width / (float)height);
        }*/

        // Notify derived class
        /////windowResized();
        //viewChanged();

        prepared = true;
    }

    void DestroyFrameBuffer() {
        for (auto framebuffer : frameBuffers) {
            vkDestroyFramebuffer(gpu, framebuffer, nullptr);
        }
    }

    void SetupFrameBuffer() {
        {
            VkImageView attachment[1];
            VkFramebufferCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.renderPass = renderPass;
            info.attachmentCount = 1;
            info.pAttachments = attachment;
            info.width = swapChain->width;
            info.height = swapChain->height;
            info.layers = 1;

            frameBuffers.resize(swapChain->imageCount);

            for (uint32_t i = 0; i < swapChain->imageCount; i++)
            {
                attachment[0] = swapChain->imageViews[i];

                VK_CHECK_RESULT(vkCreateFramebuffer(gpu, &info, nullptr, &frameBuffers[i]));
            }
        }
    }

    int loop() {
        auto lastTimestamp = std::chrono::high_resolution_clock::now();
        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        uint32_t frameCounter = 0;
        uint32_t lastFPS = 0;
        
        while (!glfwWindowShouldClose(window)) {
            frameCounter++;
            auto nowTime = std::chrono::high_resolution_clock::now();
            float frameDuration = (float)(std::chrono::duration<double, std::milli>(nowTime - lastFrameTime).count());
            lastFrameTime = nowTime;

            glfwPollEvents();

            // minimized
            int iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
            while (iconified) {
                glfwWaitEvents();
                iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
            }

            //auto tStart = std::chrono::high_resolution_clock::now();

            //UpdateFPS();

            if (resized) {
                OnWindowResize();
                resized = false;
            }

            // lastFPS是我们自己算的fps。把frameDuration喂给imgui，它也会自己算一个fps。
            FrontendUI(lastFPS, frameDuration / 1000);

            VulkanDraw();

            // fps
            float duration = (float)(std::chrono::duration<double, std::milli>(nowTime - lastTimestamp).count());
            if (duration > 1000.0f)
            {
                lastFPS = static_cast<uint32_t>((float)frameCounter * (1000.0f / duration));

                frameCounter = 0;
                lastTimestamp = nowTime;
            }
            //std::cout << "lastFPS = " << lastFPS << std::endl;

            //vkDeviceWaitIdle(gpu);
        }

        return 0;
    }

    int clean() {
        std::cout << "VulkanApp.clean()"<< std::endl;

        DestroyCommandBuffers();
        vkDestroyCommandPool(gpu, cmdPool, nullptr);
        
        for (auto framebuffer : frameBuffers) {
            vkDestroyFramebuffer(gpu, framebuffer, nullptr);
        }

        vkDestroyDescriptorSetLayout(gpu, descriptorSetLayout, nullptr);

        vkDestroyPipeline(gpu, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(gpu, pipelineLayout, nullptr);

        vkDestroyRenderPass(gpu, renderPass, nullptr);

        for (auto& fence : waitFences) {
            vkDestroyFence(gpu, fence, nullptr);
        }

        vkDestroySemaphore(gpu, presentCompleteSemaphore, nullptr);
        vkDestroySemaphore(gpu, renderCompleteSemaphore, nullptr);

        vkDestroyShaderModule(gpu, fragShader, nullptr);
        vkDestroyShaderModule(gpu, vertShader, nullptr);

        
        renderImage.Clean();
        swapChain->Clean();
        ui->FreeResources();
        device->clean();

        vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();

        return 0;
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
    VkSubmitInfo submitInfo; // 可复用

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
};

inline VulkanApp app;

int vulkan_main()
{
    std::cout<<"vulkan_main start"<<std::endl;

    app.init();
    app.loop();
    app.clean();

    std::cout << "vulkan_main over" << std::endl;
    return 0;
}

