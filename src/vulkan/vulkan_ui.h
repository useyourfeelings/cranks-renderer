#ifndef VULKAN_UI_H
#define VULKAN_UI_H

#include<iostream>
#include <vector>
#include <GLFW/glfw3.h>
#include"imgui.h"
#include "imgui_impl_glfw.h"

#include <vulkan/vulkan.h>
#include "vulkan_main.h"
#include"vulkan_device.h"
#include"vulkan_buffer.h"
#include"vulkan_tool.h"
//#include"../renderer_ui.h"

#include "../core/api.h"
#include "../base/events.h"
#include "../tool/logger.h"
#include"../base/json.h"
#include "../tool/image.h"

class VulkanUI {
public:
	VulkanUI(VulkanApp* app, GLFWwindow* window, std::shared_ptr<VulkanDevice> device, VkQueue queue, int width, int height):
		app(app),
		window(window),
		device(device),
		queue(queue),
		width(width),
		height(height) {

		ImGui::CreateContext();
		//ImGuiIO& io = ImGui::GetIO();
	};

	~VulkanUI() {
		if (ImGui::GetCurrentContext()) {
			ImGui::DestroyContext();
		}
	}

	int SetVulkanData(std::shared_ptr<VulkanDevice> new_device) {
		device = new_device;

		return 0;
	}

	int LoadShader() {

		return 0;
	}

	int Resize(uint32_t width, uint32_t height) {
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)(width), (float)(height));

		return 0;
	}

	int SetStyle() {
		ImGui::StyleColorsLight();
		ImGuiStyle& style = ImGui::GetStyle();
		style.ItemSpacing = ImVec2(8, 8);
		style.FrameRounding = 2;
		style.FrameBorderSize = 1;
		style.TabBorderSize = 1;
		style.TabRounding = 2;
		style.WindowRounding = 1;
		style.ScrollbarSize = 16;
		style.WindowMenuButtonPosition = 1; // right
		style.GrabRounding = 1;

		return 0;
	}

	int InitResource() {

		std::cout << "VulkanUI.InitResource()" << std::endl;

		ImGuiIO& io = ImGui::GetIO();

		// font data
		unsigned char* fontData;
		int texWidth, texHeight;

		const std::string filename = "resource/fonts/consolas.ttf";
		io.Fonts->AddFontFromFileTTF(filename.c_str(), 20.0f);
		//io.Fonts->GetGlyphRangesChineseSimplifiedCommon();
		io.Fonts->GetGlyphRangesChineseFull();

		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

		// Create target image for copy
		VkImageCreateInfo imageInfo = vktool::imageCreateInfo();
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.extent.width = texWidth;
		imageInfo.extent.height = texHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VK_CHECK_RESULT(vkCreateImage(device->device, &imageInfo, nullptr, &fontImage));

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device->device, fontImage, &memReqs);
		VkMemoryAllocateInfo memAllocInfo = vktool::memoryAllocateInfo();
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->device, &memAllocInfo, nullptr, &fontMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->device, fontImage, fontMemory, 0));

		// Image view
		VkImageViewCreateInfo viewInfo = vktool::imageViewCreateInfo();
		viewInfo.image = fontImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		VK_CHECK_RESULT(vkCreateImageView(device->device, &viewInfo, nullptr, &fontView));


		// Staging buffers for font data upload
		VulkanBuffer stagingBuffer;
		VK_CHECK_RESULT(device->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer,
			uploadSize));

		// map以后copy fontData数据
		stagingBuffer.map();
		memcpy(stagingBuffer.mapped, fontData, uploadSize);
		stagingBuffer.unmap();

		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// Prepare for transfer
		vktool::setImageLayout(
			copyCmd,
			fontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Copy
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = texWidth;
		bufferCopyRegion.imageExtent.height = texHeight;
		bufferCopyRegion.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer.buffer,
			fontImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion
		);

		// Prepare for shader read
		vktool::setImageLayout(
			copyCmd,
			fontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		device->flushCommandBuffer(copyCmd, queue, true);

		stagingBuffer.destroy();

		// Font texture Sampler
		VkSamplerCreateInfo samplerInfo = vktool::samplerCreateInfo();
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device->device, &samplerInfo, nullptr, &sampler));


		// Descriptor pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vktool::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vktool::descriptorPoolCreateInfo(poolSizes, 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device->device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Descriptor set layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vktool::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vktool::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->device, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Descriptor set
		VkDescriptorSetAllocateInfo allocInfo = vktool::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet));
		VkDescriptorImageInfo fontDescriptor = vktool::descriptorImageInfo(
			sampler,
			fontView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			vktool::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor)
		};
		vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		SetStyle();

		ImGui_ImplGlfw_InitForVulkan(window, true);

		return 0;
	}

	void Clean()
	{
		std::cout << "VulkanUI.Clean()" << std::endl;
		vertexBuffer.destroy();
		indexBuffer.destroy();
		vkDestroyImageView(device->device, fontView, nullptr);
		vkDestroyImage(device->device, fontImage, nullptr);
		vkFreeMemory(device->device, fontMemory, nullptr);
		vkDestroySampler(device->device, sampler, nullptr);
		vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);
		vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
		vkDestroyPipeline(device->device, pipeline, nullptr);

		vkDestroyShaderModule(device->device, fragShader, nullptr);
		vkDestroyShaderModule(device->device, vertShader, nullptr);
	}

	void SetupPipeline(const VkRenderPass renderPass)
	{
		// Pipeline layout
		// Push constants for UI rendering parameters
		VkPushConstantRange pushConstantRange = vktool::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vktool::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// Setup graphics pipeline for UI rendering
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vktool::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterizationState =
			vktool::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		// Enable blending
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlendState =
			vktool::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

		VkPipelineDepthStencilStateCreateInfo depthStencilState =
			vktool::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

		VkPipelineViewportStateCreateInfo viewportState =
			vktool::pipelineViewportStateCreateInfo(1, 1, 0);

		VkPipelineMultisampleStateCreateInfo multisampleState =
			vktool::pipelineMultisampleStateCreateInfo(rasterizationSamples);

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState =
			vktool::pipelineDynamicStateCreateInfo(dynamicStateEnables);

		VkPipelineShaderStageCreateInfo stage[2] = {};
		stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage[0].module = vertShader = vktool::LoadSPIRVShader(device->device, "resource/shader/glsl/ui.vert.spv");// vertShader;
		stage[0].pName = "main";
		stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage[1].module = fragShader = vktool::LoadSPIRVShader(device->device, "resource/shader/glsl/ui.frag.spv");// fragShader;
		stage[1].pName = "main";

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = vktool::pipelineCreateInfo(pipelineLayout, renderPass);

		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = 2;// static_cast<uint32_t>(shaders.size());
		pipelineCreateInfo.pStages = stage;
		pipelineCreateInfo.subpass = subpass;

		// Vertex bindings an attributes based on ImGui vertex definition
		std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
			vktool::vertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
		};
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			vktool::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),	// Location 0: Position
			vktool::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),	// Location 1: UV
			vktool::vertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),	// Location 0: Color
		};
		VkPipelineVertexInputStateCreateInfo vertexInputState = vktool::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		pipelineCreateInfo.pVertexInputState = &vertexInputState;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device->device, 0, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}

	/** Update vertex and index buffer containing the imGui elements when required */
	bool UpdateBuffer()
	{
		// imgui的drawdata是否有改变。

		ImDrawData* imDrawData = ImGui::GetDrawData();
		bool updateCmdBuffers = false;

		if (!imDrawData) { 
			return false; 
		};

		// Note: Alignment is done inside buffer creation
		VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
		VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

		// Update buffers only if vertex or index count has been changed compared to current buffer size
		if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
			return false;
		}

		// Vertex buffer
		if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) {
			vertexBuffer.unmap();
			vertexBuffer.destroy();
			VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vertexBuffer, vertexBufferSize));
			vertexCount = imDrawData->TotalVtxCount;
			vertexBuffer.unmap();
			vertexBuffer.map();
			updateCmdBuffers = true;
		}

		// Index buffer
		VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
		if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) {
			indexBuffer.unmap();
			indexBuffer.destroy();
			VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &indexBuffer, indexBufferSize));
			indexCount = imDrawData->TotalIdxCount;
			indexBuffer.map();
			updateCmdBuffers = true;
		}

		// Upload data
		ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer.mapped;
		ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer.mapped;

		for (int n = 0; n < imDrawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		// Flush to make writes visible to GPU
		vertexBuffer.flush();
		indexBuffer.flush();

		return updateCmdBuffers;
	}

	void VulkanDraw(const VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		ImGuiIO& io = ImGui::GetIO();

		const VkViewport viewport = vktool::viewport(io.DisplaySize.x, io.DisplaySize.y, 0.0f, 1.0f);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		//const VkRect2D scissor = vktool::rect2D(width, height, 0, 0);
		//vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		pushConstBlock.scale = vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		pushConstBlock.translate = vec2(-1.0f, -1.0f);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;

		if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
			return;
		}

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

		for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
		{
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
				VkRect2D scissorRect;
				scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
				scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
				scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);

				// 默认的descriptorSet创建的是font。额外创建的图片存在pcmd->TextureId。如果pcmd带着有效的TextureId，就要用。
				// Bind DescriptorSet with font or user texture
				VkDescriptorSet desc_set[1] = { (VkDescriptorSet)pcmd->TextureId };
				if (pcmd->TextureId == 0 || sizeof(ImTextureID) < sizeof(ImU64))
				{
					// We don't support texture switches if ImTextureID hasn't been redefined to be 64-bit. Do a flaky check that other textures haven't been used.
					//IM_ASSERT(pcmd->TextureId == (ImTextureID)bd->FontDescriptorSet);
					desc_set[0] = descriptorSet;// bd->FontDescriptorSet;
				}
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, desc_set, 0, nullptr);
				//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

				// vkCmdSetScissor设置一个矩形。外部的点全部丢弃。
				vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
				vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}

	void UpdateMouseCursor()
	{
		ImGuiIO& io = ImGui::GetIO();
		//ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
		if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
			return;

		double mouse_x, mouse_y;
		glfwGetCursorPos(window, &mouse_x, &mouse_y);
		io.AddMousePosEvent((float)mouse_x, (float)mouse_y);

		int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		if(state == GLFW_PRESS) {
			io.AddMouseButtonEvent(0, true);
		} else if(state == GLFW_RELEASE) {
			io.AddMouseButtonEvent(0, false);
		}
	}

	void FrontendDraw(uint32_t lastFPS, float frameDuration, void* renderImageID) {
		ImGuiIO& io = ImGui::GetIO();

		// Setup display size (every frame to accommodate for window resizing)
		int w, h;
		int display_w, display_h;
		glfwGetWindowSize(window, &w, &h);
		glfwGetFramebufferSize(window, &display_w, &display_h);
		io.DisplaySize = ImVec2((float)w, (float)h);
		if (w > 0 && h > 0)
			io.DisplayFramebufferScale = ImVec2((float)display_w / (float)w, (float)display_h / (float)h);

		//const bool is_app_focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
		//UpdateMouseCursor();

		// must feed to imgui to get correct fps and ui behavior
		io.DeltaTime = frameDuration;// 1.0f / 60.0f;

		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		RendererUI();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		/*ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);*/

		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 20, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

		{
			ImGui::Begin("Cranks Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
			ImGui::Text("FPS:%d", lastFPS);
			ImGui::TextUnformatted(device->properties.deviceName);
			//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

			//ImGui::PushItemWidth(110.0f * scale);
			//OnUpdateUIOverlay(&UIOverlay);
			//ImGui::PopItemWidth();

			ImGui::End();
		}

		ImGui::PopStyleVar();

		//ImGui::EndFrame();
		ImGui::Render();

	}

	void RendererUI() {
		static int registered = 0;

		if (registered == 0) {
			RENDER_TASK_ID = RegisterEvent(PBR_API_render);
			registered = 1;
		}

		static int init_status = 0;

		static CameraSetting cs;

		std::string hdr_file = "crosswalk_1k.hdr"; // alps_field_1k.hdr crosswalk_1k.hdr tv_studio_1k delta_2_1k

		if (init_status == 0) {
			PBR_API_get_camera_setting(cs);
			PBR_API_set_perspective_camera(cs);
			init_status = 1;

			// hdr test. mipmap test.
			//int res_x, res_y;
			//auto image_data = ReadHDRRaw("crosswalk_1k.hdr", &res_x, &res_y);

			//app->BuildTestImage(res_x, res_y, image_data, 0);

			PBR_API_make_test_mipmap(hdr_file);  // alps_field_1k.hdr crosswalk_1k.hdr

			int res_x, res_y;
			std::vector<unsigned char> image_data;
			PBR_API_get_mipmap_image(0, image_data, res_x, res_y);
			app->BuildTestImage(res_x, res_y, image_data.data(), 0);

			PBR_API_get_mipmap_image(1, image_data, res_x, res_y);
			app->BuildTestImage(res_x, res_y, image_data.data(), 1);

			PBR_API_get_mipmap_image(2, image_data, res_x, res_y);
			app->BuildTestImage(res_x, res_y, image_data.data(), 2);

			PBR_API_get_mipmap_image(3, image_data, res_x, res_y);
			app->BuildTestImage(res_x, res_y, image_data.data(), 3);

			PBR_API_get_mipmap_image(4, image_data, res_x, res_y);
			app->BuildTestImage(res_x, res_y, image_data.data(), 4);

			PBR_API_get_mipmap_image(5, image_data, res_x, res_y);
			app->BuildTestImage(res_x, res_y, image_data.data(), 5);
		}

		//int progress_now, progress_total,
		static int render_status, has_new_photo;
		static std::vector<int> progress_now, progress_total;
		PBR_API_get_render_progress(&render_status, progress_now, progress_total, &has_new_photo);

		if (has_new_photo) {
			std::vector<unsigned char> image_data(cs.resolution[0] * cs.resolution[1] * 4);
			PBR_API_get_new_image(image_data.data());
			app->BuildImage(cs.resolution[0], cs.resolution[1], image_data.data());
		}

		// ui

		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);

		LoggerUI();

		ImGui::SetNextWindowSize(ImVec2(480, 600), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowBgAlpha(1);

		if (!ImGui::Begin("Hello Crank!民科", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::End();
			return;
		}

		if (render_status == 1) {
			ImGui::BeginDisabled();
		}

		ImGui::Text("camera setting");
		ImGui::SameLine();
		if (ImGui::Button("load default")) {
			PBR_API_get_defualt_camera_setting(cs);
			PBR_API_set_perspective_camera(cs);
		}

		ImGui::SameLine();
		if (ImGui::Button("save setting")) {
			PBR_API_save_setting();
		}

		ImGui::PushItemWidth(400);

		bool cameraChanged = false;
		ImGui::SliderFloat3("pos", cs.pos, -100, 100);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			cameraChanged = true;
		}

		ImGui::SliderFloat3("look", cs.look, -100, 100);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			cameraChanged = true;
		}

		ImGui::SliderFloat3("up", cs.up, -100, 100);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			cameraChanged = true;
		}

		ImGui::PopItemWidth();

		ImGui::PushItemWidth(160);

		ImGui::SliderFloat("fov", &cs.fov, 0, 180);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			cameraChanged = true;
		}

		ImGui::SameLine();

		ImGui::SliderFloat("aspect_ratio", &cs.asp, 0.5f, 2, "%.1f");
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			cameraChanged = true;
		}

		ImGui::PopItemWidth();

		ImGui::PushItemWidth(400);

		ImGui::SliderFloat2("near_far", cs.near_far, 0.001f, 1000);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			cameraChanged = true;
		}

		ImGui::SliderInt2("resolution", cs.resolution, 0, 1000);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			cameraChanged = true;
		}

		ImGui::PopItemWidth();

		ImGui::SliderInt("ray_sample_no", &cs.ray_sample_no, 1, 100);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			cameraChanged = true;
		}

		ImGui::SliderInt("ray_bounce_no", &cs.ray_bounce_no, 0, 10);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			cameraChanged = true;
		}

		if (ImGui::SliderInt("scale", &cs.image_scale, 1, 16)) {
			cs.resolution[0] = cs.image_scale * 128;
			cs.resolution[1] = cs.image_scale * 128;
			cameraChanged = true;
		}

		ImGui::SliderInt("render_threads_count", &cs.render_threads_count, 1, 6);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			cameraChanged = true;
		}

		if (cameraChanged) {
			Log("cameraChanged");
			PBR_API_set_perspective_camera(cs);
		}

		//ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		//ImGui::Checkbox("Another Window", &show_another_window);

		//ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		//ImGui::SameLine();
		//ImGui::Text("counter = %d", counter);

		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		//ImGui::Dummy(ImVec2(30, 30));

		

		for (int i = 0; i < progress_now.size(); ++i) {
			char buf[32];
			sprintf(buf, "%d/%d", progress_now[i], progress_total[i]);
			ImGui::ProgressBar(float(progress_now[i]) / progress_total[i], ImVec2(0.f, 0.f), buf);
		}
		

		if (ImGui::Button("Render", ImVec2(200, 120))) {

			static int set_default_scene = 0;
			if (set_default_scene == 0) {
				set_default_scene = 1;

#if 1
				PBR_API_add_sphere("wtfSphere 1", 6, 0, 0, 0, json(
					{
						{ "name", "glass" }, // glass mirror matte
						{ "kd", {0.9, 0.4, 0.2} },
						{ "sigma", 0.8 },
						{ "kr", {1.0, 1.0, 1.0} },
						{ "kt", {1.0, 1.0, 1.0} },
						{ "eta", 1.6},
						{ "uroughness", 0},
						{ "vroughness", 0},
						{ "bumpmap", 0},
						{ "remaproughness", false}
					}
				));
				PBR_API_add_sphere("wtfSphere 2 green", 5, -10, 0, 12, json({ { "name", "matte" }, { "kd", {0.2, 0.7, 0.2} }, {"sigma", 0.8} }));
				PBR_API_add_sphere("wtfSphere 3", 20, 30, 30, 30, json({ { "name", "matte" }, { "kd", {0.9, 0.4, 0.12} }, {"sigma", 0.8} }));
				PBR_API_add_sphere("wtfSphere 4", 500, 0, 0, -518, 
					json({
						{ "name", "matte" },
						{ "kd", {0.8, 0.6, 0.6} },
						{"sigma", 0.8}

						//{ "name", "glass" }, // glass mirror
						//{ "kr", {1.0, 1.0, 1.0} },
						//{ "kt", {1.0, 1.0, 1.0} },
						//{ "eta", 1.6},
						//{ "uroughness", 0},
						//{ "vroughness", 0},
						//{ "bumpmap", 0},
						//{ "remaproughness", false}
					})
				);

				PBR_API_add_sphere("wtfSphere 55", 2, 7.2, -2, -3, json({
					{ "name", "matte" },
					{ "kd", {0.8, 0.0, 0.4} },
					{"sigma", 0.8}
					/*{ "name", "mirror" },
					{ "kr", {1.0, 1.0, 1.0} },
					{ "bumpmap", 0},*/
				}));

				PBR_API_add_sphere("wtfSphere 5", 5, 10, 8, 5, json({
					/*{ "name", "matte" },
					{ "kd", {0.8, 0.0, 0.4} },
					{"sigma", 0.8}*/
					{ "name", "mirror" },
					{ "kr", {1.0, 1.0, 1.0} },
					{ "bumpmap", 0},
				}));

				PBR_API_add_sphere("wtfSphere 6", 3, 14, 0, -5, json(
					{
						{ "name", "glass" },
						{ "kr", {1.0, 1.0, 1.0} },
						{ "kt", {1.0, 1.0, 1.0} },
						{ "eta", 1.4},
						{ "uroughness", 0},
						{ "vroughness", 0},
						{ "bumpmap", 0},
						{ "remaproughness", false}
					}
				));

				PBR_API_add_sphere("wtfSphere 7", 4, -10, -4, 0, json(
					{
						{ "name", "mirror" },
						{ "kr", {1.0, 1.0, 1.0} },
						{ "bumpmap", 0},
					}
				));

				/*PBR_API_add_sphere("wtfSphere 8", 4, 0, 20, 0, json(
					{
						{ "name", "mirror" },
						{ "kr", {1.0, 1.0, 1.0} },
						{ "bumpmap", 0},
					}
				));*/

				PBR_API_add_sphere("wtfSphere 9",  4, 0, 16, 3, json({ { "name", "matte" }, { "kd", {1, 0.0, 0.1} }, {"sigma", 0.8} }));
				PBR_API_add_sphere("wtfSphere 10", 2, 0, 16, -6, json({ { "name", "matte" }, { "kd", {0, 1, 0.5} }, {"sigma", 0.8} }));

				PBR_API_add_sphere("wtfSphere 11", 2, -5, -10, -4, json({
					{ "name", "matte" },
					{ "kd", {0.2, 0.3, 0.8} },
					{"sigma", 0.8}
					/*{ "name", "mirror" },
					{ "kr", {1.0, 1.0, 1.0} },
					{ "bumpmap", 0},*/
					}));
#endif
				//PBR_API_add_point_light("wtf Light 2", 10, 30, 10);
				PBR_API_add_point_light("wtf Light", 0, 0, 20);

				PBR_API_add_infinite_light("inf light", 0, 0, 100, 1, 1, 1, 1.4, 4, hdr_file); // alps_field_1k.hdr crosswalk_1k.hdr
			}

			SendEvent(RENDER_TASK_ID);
		}

		if (render_status == 1) {
			ImGui::EndDisabled();
		}

		if (render_status == 0) {
			ImGui::BeginDisabled();
		}

		ImGui::SameLine();
		if (ImGui::Button("Stop", ImVec2(200, 120))) {
			PBR_API_stop_rendering();
		}

		if (render_status == 0) {
			ImGui::EndDisabled();
		}

		ImGui::End();

		{
			//Log("resolution %d, %d", resolution[0], resolution[1]);
			ImGui::SetNextWindowSize(ImVec2(cs.resolution[0] + 20, cs.resolution[1] + 50));
			ImGui::SetNextWindowPos(ImVec2(20, 140));

			ImGui::Begin("Scene");

			//ImGui::Image((ImTextureID)app.renderImage.descriptorSet);
			//ImGui::Image((ImTextureID)renderImageID, ImVec2(80, 80), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			//Log("renderImageID %d", (ImTextureID)renderImageID);

			//std::cout << "renderImageID = " << renderImageID << std::endl;
			ImGui::Image((ImTextureID)app->renderImage.descriptorSet, ImVec2(cs.resolution[0], cs.resolution[1]), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

			ImGui::End();
		}

		{
			//ImGui::SetNextWindowSize(ImVec2(1024 + 20, 1024 + 50));
			//ImGui::SetNextWindowPos(ImVec2(512, 20));

			ImGui::Begin("mipmap");

			//ImGui::Image((ImTextureID)app.renderImage.descriptorSet);
			//ImGui::Image((ImTextureID)renderImageID, ImVec2(80, 80), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			//Log("renderImageID %d", (ImTextureID)renderImageID);

			ImGui::Image((ImTextureID)app->testImages[0]->descriptorSet, ImVec2(app->testImages[0]->width, app->testImages[0]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::Image((ImTextureID)app->testImages[1]->descriptorSet, ImVec2(app->testImages[1]->width, app->testImages[1]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::Image((ImTextureID)app->testImages[2]->descriptorSet, ImVec2(app->testImages[2]->width, app->testImages[2]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::Image((ImTextureID)app->testImages[3]->descriptorSet, ImVec2(app->testImages[3]->width, app->testImages[3]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::Image((ImTextureID)app->testImages[4]->descriptorSet, ImVec2(app->testImages[4]->width, app->testImages[4]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::Image((ImTextureID)app->testImages[5]->descriptorSet, ImVec2(app->testImages[5]->width, app->testImages[5]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));




			ImGui::End();
		}
	}

	struct vec2 {
		float x, y;
	};

	struct PushConstBlock {
		vec2 scale; //glm::vec2 scale;
		vec2 translate; //glm::vec2 translate;
	} pushConstBlock;

	float scale = 1.0f;

	int width, height;

	GLFWwindow* window;

	VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VulkanApp* app;

	//VkDevice device;
	std::shared_ptr<VulkanDevice> device;

	VkImage fontImage = VK_NULL_HANDLE;
	VkDeviceMemory fontMemory = VK_NULL_HANDLE;
	VkImageView fontView = VK_NULL_HANDLE;
	VkSampler sampler;
	VkQueue queue;

	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkShaderModule vertShader;
	VkShaderModule fragShader;

	uint32_t subpass = 0;

	VulkanBuffer vertexBuffer;
	VulkanBuffer indexBuffer;
	int32_t vertexCount = 0;
	int32_t indexCount = 0;

	int RENDER_TASK_ID = -1;
};

#endif