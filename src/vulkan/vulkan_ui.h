#ifndef VULKAN_UI_H
#define VULKAN_UI_H

#include<iostream>
#include <vector>
#include <GLFW/glfw3.h>
#include"imgui.h"
#include"imgui_internal.h"
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
#include"../tool/json.h"
#include "../tool/image.h"

#include "../../third_party/imgui_file_dialog/ImGuiFileDialog.h"

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
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
		style.FrameRounding = 3;
		style.FrameBorderSize = 1;
		style.TabBorderSize = 1;
		style.TabRounding = 2;
		style.WindowRounding = 1;
		style.ScrollbarSize = 16;
		style.WindowMenuButtonPosition = ImGuiDir_None;
		style.GrabRounding = 1;
		style.WindowPadding = ImVec2(4, 4);
		style.ItemSpacing = ImVec2(6, 6);
		style.WindowTitleAlign = ImVec2(0.01, 0.26);

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
				

				// imgui problem for modal
				// https://github.com/ocornut/imgui/releases/tag/v1.86

				vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset + pcmd->IdxOffset, vertexOffset + pcmd->VtxOffset, 0);
				//indexOffset += pcmd->ElemCount;
			}
			indexOffset += cmd_list->IdxBuffer.Size;
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

	void FrontendDraw(MultiTaskCtx& thread_ctx, uint32_t lastFPS, float frameDuration, void* renderImageID) {
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

		//ImGui::ShowDemoWindow();

		RendererUI(thread_ctx, lastFPS);

		//ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		/*ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);*/

		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 10, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

		if(0)
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

		//ImGui::PopStyleVar();

		//ImGui::EndFrame();
		ImGui::Render();

	}

	void AboutWindow(bool* p_open) {
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(300, 240), ImGuiCond_FirstUseEver);
		ImGui::Begin("Cranks Renderer(Vulkan)", p_open,
			ImGuiWindowFlags_NoResize | 
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove);
		ImGui::TextUnformatted(device->properties.deviceName);
		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

		//ImGui::PushItemWidth(110.0f * scale);
		//OnUpdateUIOverlay(&UIOverlay);
		//ImGui::PopItemWidth();
		ImGui::End();
	}

	void RendererUI(MultiTaskCtx & thread_ctx, uint32_t lastFPS) {
		static uint32_t currentFrame = 0;
		static uint32_t lastProgressUpdatingFrame = 0;

		if (currentFrame == 0) {
			RENDER_TASK_ID = RegisterEvent(thread_ctx, PBR_API_render);
		}

		static CameraSetting cs;
		static SceneOptions scene_options;
		static SystemConfig system_config;
		static WhittedIntConfig whitted_int_config;
		static PathIntConfig path_int_config;
		static VpathIntConfig vpath_int_config;
		static PMIntConfig pm_int_config;
		static PPMIntConfig ppm_int_config;

		static bool show_about_window = false;
		static bool show_demo_window = false;

		std::string hdr_file = "crosswalk_1k.hdr"; // alps_field_1k.hdr crosswalk_1k.hdr tv_studio_1k delta_2_1k

		if (currentFrame == 0) {
			PBR_API_get_camera_setting(cs);
			PBR_API_set_perspective_camera(cs);

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

			PbrApiGetSceneConfig(scene_options);
			PbrApiGetSystemConfig(system_config);
			PbrApiGetIntegratorsConfig(whitted_int_config, path_int_config, vpath_int_config, pm_int_config, ppm_int_config);
		}

		currentFrame++;

		static json render_status_info({ {"currentIteration", 1}, {"render_duration", 0} });
		static int render_status, has_new_photo;
		static std::vector<int> progress_now, progress_total;
		static std::vector<float> progress_per;
		static float render_duaration;

		// 不用每帧都更新
		if (currentFrame == 0 ||
			currentFrame > (lastProgressUpdatingFrame + lastFPS / 20)) {
			PBR_API_get_render_progress(&render_status, progress_now, progress_total, progress_per, &has_new_photo, render_status_info);
			lastProgressUpdatingFrame = currentFrame;
		}

		if (has_new_photo) {
			std::vector<unsigned char> image_data(cs.resolution[0] * cs.resolution[1] * 4);
			PBR_API_get_new_image(image_data.data());
			app->BuildImage(cs.resolution[0], cs.resolution[1], image_data.data());
			has_new_photo = 0;
		}

		// ui

		if (show_about_window) {
			AboutWindow(&show_about_window);
		}

		if (show_demo_window) {
			ImGui::ShowDemoWindow(&show_demo_window);
		}

		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

		ImVec2 main_menu_bar_size;

		if (0)
		{
			// main menu

			if (ImGui::BeginMainMenuBar())
			//if (ImGui::BeginMenuBar())
			{
				main_menu_bar_size = ImGui::GetWindowSize();
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("New Project")) {
					}

					if (ImGui::MenuItem("Open", "Ctrl+O")) {
						ImGuiFileDialog::Instance()->OpenDialog("Open Project", "Choose File", ".cranks",
							".", 1, nullptr,
							ImGuiFileDialogFlags_Modal |
							ImGuiFileDialogFlags_ConfirmOverwrite);
					}
					if (ImGui::BeginMenu("Open Recent"))
					{
						ImGui::MenuItem("fish_hat.c");
						ImGui::MenuItem("fish_hat.inl");
						ImGui::MenuItem("fish_hat.h");
						if (ImGui::BeginMenu("More.."))
						{
							ImGui::MenuItem("Hello");
							ImGui::MenuItem("Sailor");
							ImGui::EndMenu();
						}
						ImGui::EndMenu();
					}
					if (ImGui::MenuItem("Save", "Ctrl+S")) {}
					if (ImGui::MenuItem("Save As..")) {
						ImGuiFileDialog::Instance()->OpenDialog("Save Project As", "Save As", ".cranks",
							".", 1, nullptr,
							ImGuiFileDialogFlags_Modal |
							ImGuiFileDialogFlags_ConfirmOverwrite);
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Options"))
				{
					static bool enabled = true;
					ImGui::MenuItem("Enabled", "", &enabled);
					ImGui::BeginChild("child", ImVec2(0, 60), true);
					for (int i = 0; i < 10; i++)
						ImGui::Text("Scrolling Text %d", i);
					ImGui::EndChild();
					static float f = 0.5f;
					static int n = 0;
					ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
					ImGui::InputFloat("Input", &f, 0.1f);
					ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Help"))
				{
					ImGui::MenuItem("About", NULL, &show_about_window);
					ImGui::MenuItem("Imgui Demo", NULL, &show_demo_window);
					ImGui::EndMenu();
				}
			}
			ImGui::EndMainMenuBar();
			//ImGui::EndMenuBar();
		}

		if(1)
		{
			//std::cout << std::format("{} {} {} {} {}\n", main_viewport->WorkPos.x, main_viewport->WorkPos.y, 
			//	main_viewport->WorkSize.x, main_viewport->WorkSize.y, main_menu_bar_size.y);
			ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y + main_menu_bar_size.y), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x, main_viewport->WorkSize.y - main_menu_bar_size.y), ImGuiCond_Always);

			ImGui::Begin("main", nullptr,
				//ImGuiWindowFlags_AlwaysAutoResize |
				//ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_NoDocking |
				ImGuiWindowFlags_MenuBar |


				ImGuiWindowFlags_NoMove |
				//ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoSavedSettings |
				//ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_NoDecoration);

			if (1) {
				if (ImGui::BeginMenuBar()) {
					if (ImGui::BeginMenu("File"))
					{
						if (ImGui::MenuItem("New Project")) {
						}

						if (ImGui::MenuItem("Open", "Ctrl+O")) {
							ImGuiFileDialog::Instance()->OpenDialog("Open Project", "Choose File", ".cranks",
								".", 1, nullptr,
								ImGuiFileDialogFlags_Modal |
								ImGuiFileDialogFlags_ConfirmOverwrite);
						}
						if (ImGui::BeginMenu("Open Recent"))
						{
							ImGui::MenuItem("fish_hat.c");
							ImGui::MenuItem("fish_hat.inl");
							ImGui::MenuItem("fish_hat.h");
							if (ImGui::BeginMenu("More.."))
							{
								ImGui::MenuItem("Hello");
								ImGui::MenuItem("Sailor");
								ImGui::EndMenu();
							}
							ImGui::EndMenu();
						}
						if (ImGui::MenuItem("Save", "Ctrl+S")) {}
						if (ImGui::MenuItem("Save As..")) {
							ImGuiFileDialog::Instance()->OpenDialog("Save Project As", "Save As", ".cranks",
								".", 1, nullptr,
								ImGuiFileDialogFlags_Modal |
								ImGuiFileDialogFlags_ConfirmOverwrite);
						}
						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Options"))
					{
						static bool enabled = true;
						ImGui::MenuItem("Enabled", "", &enabled);
						ImGui::BeginChild("child", ImVec2(0, 60), true);
						for (int i = 0; i < 10; i++)
							ImGui::Text("Scrolling Text %d", i);
						ImGui::EndChild();
						static float f = 0.5f;
						static int n = 0;
						ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
						ImGui::InputFloat("Input", &f, 0.1f);
						ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Help"))
					{
						ImGui::MenuItem("About", NULL, &show_about_window);
						ImGui::MenuItem("Imgui Demo", NULL, &show_demo_window);
						ImGui::EndMenu();
					}

					ImGui::EndMenuBar();
				}
			}

			if (1) {

				if (ImGui::DockBuilderGetNode(ImGui::GetID("dockspace")) == NULL)
				{
					std::cout << "DockBuilder\n";
					ImGuiID dockspace_id = ImGui::GetID("dockspace");
					ImGuiViewport* viewport = ImGui::GetMainViewport();
					ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
					ImGui::DockBuilderAddNode(dockspace_id, 0); // Add empty node

					ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
					ImGuiID scene_docker = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.30f, NULL, &dock_main_id); 
					ImGuiID control_docker = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.40f, NULL, &dock_main_id);
					ImGuiID object_dock = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.30f, NULL, &dock_main_id);
					ImGuiID property_dock = ImGui::DockBuilderSplitNode(object_dock, ImGuiDir_Down, 0.60f, NULL, &object_dock);
					ImGuiID material_dock = ImGui::DockBuilderSplitNode(object_dock, ImGuiDir_Right, 0.50f, NULL, &object_dock);
					ImGuiID medium_dock = ImGui::DockBuilderSplitNode(object_dock, ImGuiDir_Right, 0.50f, NULL, &object_dock);

					ImGui::DockBuilderDockWindow("Scene", scene_docker);
					ImGui::DockBuilderDockWindow("Control", control_docker);
					ImGui::DockBuilderDockWindow("Object", object_dock);
					ImGui::DockBuilderDockWindow("Material", material_dock);
					ImGui::DockBuilderDockWindow("Medium", medium_dock);
					ImGui::DockBuilderDockWindow("Property", property_dock);
					ImGui::DockBuilderFinish(dockspace_id);
				}
			}

		}

		ImGuiID dockspace_id = ImGui::GetID("dockspace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);

		const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
		const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		static int need_to_update_scene_tree = 1;
		static int need_to_load_scene_config = 0;
		static int need_to_load_system_config = 1;
		static int need_to_load_integrators_config = 0;
		static int need_to_update_material_tree = 1;
		static int need_to_update_medium_tree = 1;
		static json scene_tree = PBR_API_get_scene_tree();
		static json material_tree = PBR_API_get_material_tree();
		static json medium_tree = PBR_API_get_medium_tree();

		static std::map<int, std::string> material_name_map;
		static int material_combo_selected_id = 0;
		static int object_medium_combo_selected_id = -1;

		static std::map<int, std::string> medium_name_map;
		static int medium_combo_selected_id = 0;

		{
			if (ImGuiFileDialog::Instance()->Display("Open Project", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400), ImVec2(1200, 800)))
			{
				// action if OK
				if (ImGuiFileDialog::Instance()->IsOk())
				{
					//std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
					std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
					// action

					std::cout << "load project " << filePath << std::endl;
					int result = PBR_API_load_project(filePath);
					std::cout << "result = " << result << std::endl;

					need_to_update_scene_tree = 1;
					need_to_update_material_tree = 1;
					need_to_update_medium_tree = 1;
					need_to_load_scene_config = 1;
					need_to_load_system_config = 1;
					need_to_load_integrators_config = 1;
				}

				// close
				ImGuiFileDialog::Instance()->Close();
			}

			if (ImGuiFileDialog::Instance()->Display("Save Project As", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400), ImVec2(1200, 800)))
			{
				// action if OK
				if (ImGuiFileDialog::Instance()->IsOk())
				{
					std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
					// action
					fileName.erase(fileName.rfind('.'));
					std::cout << "save project " << filePath << " " << fileName << std::endl;
					int result = PBR_API_save_project(filePath, fileName);
					std::cout << "result = " << result << std::endl;
				}

				// close
				ImGuiFileDialog::Instance()->Close();
			}
		}

		if (need_to_update_scene_tree) {
			scene_tree = PBR_API_get_scene_tree();
			need_to_update_scene_tree = 0;
		}

		if (need_to_load_scene_config) {
			PbrApiGetSceneConfig(scene_options);
			need_to_load_scene_config = 0;
		}

		if (need_to_load_system_config) {
			PbrApiGetSystemConfig(system_config);
			need_to_load_system_config = 0;
		}

		if (need_to_load_integrators_config) {
			PbrApiGetIntegratorsConfig(whitted_int_config, path_int_config, vpath_int_config, pm_int_config, ppm_int_config);
			need_to_load_integrators_config = 0;
		}

		if (need_to_update_material_tree) {
			material_tree = PBR_API_get_material_tree();
			need_to_update_material_tree = 0;

			std::cout << "material_tree " << material_tree << std::endl;

			material_name_map.clear();
			for (auto& [material_id, material] : material_tree.items()) {
				//std::cout << "material_tree id  " << material["id"] <<" name "<< material["name"] << std::endl;
				material_name_map[material["id"]] = material["name"];
			}
		}

		if (need_to_update_medium_tree) {
			medium_tree = PBR_API_get_medium_tree();
			need_to_update_medium_tree = 0;

			std::cout << "medium_tree " << medium_tree << std::endl;

			medium_name_map.clear();
			medium_name_map[0] = "None"; // empty medium

			for (auto& [medium_id, medium] : medium_tree.items()) {
				//std::cout << "medium_tree id  " << medium["id"] << " name " << medium["name"] << std::endl;
				medium_name_map[medium["id"]] = medium["name"];
			}
		}



		//ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
		//LoggerUI();

		// Scene
		{
			//Log("resolution %d, %d", resolution[0], resolution[1]);
			ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 540, main_viewport->WorkPos.y + 40), ImGuiCond_FirstUseEver);

			ImGui::SetNextWindowSize(ImVec2(cs.resolution[0] + 20, cs.resolution[1] + 50));
			//ImGui::SetNextWindowPos(ImVec2(20, 140));

			if (ImGui::Begin("Scene", nullptr,
				//ImGuiWindowFlags_AlwaysAutoResize |
				//ImGuiWindowFlags_NoMove |
				//ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_NoCollapse)) {
				//ImGui::Image((ImTextureID)app.renderImage.descriptorSet);
				//ImGui::Image((ImTextureID)renderImageID, ImVec2(80, 80), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
				//Log("renderImageID %d", (ImTextureID)renderImageID);

				//std::cout << "renderImageID = " << renderImageID << std::endl;
				ImGui::Image((ImTextureID)app->renderImage.descriptorSet, ImVec2(cs.resolution[0], cs.resolution[1]),
					ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 1));
			}
			ImGui::End();
		}
		
		static int selected_obj_id = -1;
		static json selected_obj;
		static int selected_material_id = -1;
		static json selected_material;
		static int edit_material_name_status = 0;

		static int selected_medium_id = -1;
		static json selected_medium;
		static int edit_medium_name_status = 0;

		static int edit_obj_name_status = 0;
		static int open_new_object_dialog = 0;

		static int open_delete_pop = 0;
		static json op_node;
		static int delete_node_type = 0;

		static char object_name_buffer[64] = "default";
		static char material_name_buffer[64] = "default";
		static char medium_name_buffer[64] = "default";
		static float pos[3] = { 0, 0, 0 };
		static int type = 0;
		static std::string type_name[] = { "mesh", "sphere", "point_light", "infinite_light" };
		static std::map<std::string, int> type_name_id_map = { {"mesh", 0}, {"sphere", 1}, {"point_light", 2}, {"infinite_light", 3} };
		static char file_name_buffer[64] = "cube10.gltf";
		static float radius = 5.f, strength = 1.2f;
		static float power[3] = { 1.f, 1.f, 1.f };

		// material
		static int material_type = 0;
		static float kd[3] = { 1, 1, 1 };
		static float kr[3] = { 1, 1, 1 };
		static float kt[3] = { 1, 1, 1 };
		static float ks[3] = { 1, 1, 1 };
		static float eta3[3] = { 1.8f, 1.8f, 1.8f };
		static float uvrough[2] = { 0.1f, 0.1f };
		static float sigma = 0.5f, eta = 1.8f, rough = 0.1f;
		static bool remaproughness = false;

		// medium
		static int medium_type = 0;
		static float sigma_s[3] = { 1, 1, 1 };
		static float sigma_a[3] = { 1, 1, 1 };
		static float medium_g = 0.3f;

		static std::map<std::string, int> medium_type_id_map = { {"homo", 0} };

		static std::map<std::string, int> material_type_id_map = { {"matte", 0}, {"glass", 1}, {"mirror", 2}, {"plastic", 3}, {"metal", 4} };

		const auto dump_node_to_buffer = [&](const json& node) -> void {
			sprintf_s(object_name_buffer, "%s", (std::string(node["name"])).c_str());

			pos[0] = node["world_pos"][0];
			pos[1] = node["world_pos"][1];
			pos[2] = node["world_pos"][2];

			type = type_name_id_map[node["type"]];

			if (type == 0) {
				sprintf_s(file_name_buffer, "%s", (std::string(node["file_name"])).c_str());
			}
			else if (type == 1) {
				radius = node["radius"];
			}
			else if (type == 2) {
				power[0] = node["power"][0];
				power[1] = node["power"][1];
				power[2] = node["power"][2];
			}
			else if (type == 3) {
				sprintf_s(file_name_buffer, "%s", (std::string(node["file_name"])).c_str());

				power[0] = node["power"][0];
				power[1] = node["power"][1];
				power[2] = node["power"][2];
				strength = node["strength"];
			}

			if (type == 0 || type == 1) {
				material_combo_selected_id = node["material_id"];

				if (node.contains("medium_id")) {
					object_medium_combo_selected_id = node["medium_id"];
				}
				else {
					object_medium_combo_selected_id = -1;
				}
					
			}
		};

		const auto dump_material_node_to_buffer = [&](const json& node) -> void {
			sprintf_s(material_name_buffer, "%s", (std::string(node["name"])).c_str());

			//pos[0] = node["world_pos"][0];
			//pos[1] = node["world_pos"][1];
			//pos[2] = node["world_pos"][2];

			material_type = material_type_id_map[node["type"]];

			switch (material_type) {
			case 0:
				kd[0] = node["kd"][0];
				kd[1] = node["kd"][1];
				kd[2] = node["kd"][2];
				sigma = node["sigma"];

				break;
			case 1:
				kr[0] = node["kr"][0];
				kr[1] = node["kr"][1];
				kr[2] = node["kr"][2];
				kt[0] = node["kt"][0];
				kt[1] = node["kt"][1];
				kt[2] = node["kt"][2];
				eta = node["eta"];
				uvrough[0] = node["uroughness"];
				uvrough[1] = node["vroughness"];

				break;
			case 2:
				kr[0] = node["kr"][0];
				kr[1] = node["kr"][1];
				kr[2] = node["kr"][2];

				break;
			case 3:
				kd[0] = node["kd"][0];
				kd[1] = node["kd"][1];
				kd[2] = node["kd"][2];
				ks[0] = node["ks"][0];
				ks[1] = node["ks"][1];
				ks[2] = node["ks"][2];
				rough = node["roughness"];

				break;
			case 4:
				kd[0] = node["k"][0];
				kd[1] = node["k"][1];
				kd[2] = node["k"][2];
				eta3[0] = node["eta"][0];
				eta3[1] = node["eta"][1];
				eta3[2] = node["eta"][2];
				rough = node["roughness"];

				break;
			}
		};

		const auto dump_medium_node_to_buffer = [&](const json& node) -> void {
			sprintf_s(medium_name_buffer, "%s", (std::string(node["name"])).c_str());

			//pos[0] = node["world_pos"][0];
			//pos[1] = node["world_pos"][1];
			//pos[2] = node["world_pos"][2];

			medium_type = medium_type_id_map[node["type"]];

			switch (medium_type) {
			case 0:
				sigma_a[0] = node["sigma_a"][0];
				sigma_a[1] = node["sigma_a"][1];
				sigma_a[2] = node["sigma_a"][2];

				sigma_s[0] = node["sigma_s"][0];
				sigma_s[1] = node["sigma_s"][1];
				sigma_s[2] = node["sigma_s"][2];

				medium_g = node["g"];

				break;
			}
		};

		// Objects
		{
			const auto new_object_popup = [&](const json& on_node) -> void {
				static int material_combo_selected_id = 1; // default material

				//std::cout << on_node << std::endl;
				ImGui::OpenPopup("New Object");
				//ImVec2 center = ImGui::GetMainViewport()->GetCenter();
				//ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
				if (ImGui::BeginPopupModal("New Object", NULL,
					ImGuiWindowFlags_AlwaysAutoResize
				))
				{
					ImGui::Combo("type", &type, "mesh\0sphere\0polit_light\0infinite_light\0\0");
					ImGui::InputText("name", object_name_buffer, 64);
					ImGui::SliderFloat3("pos", pos, -100, 100);

					if (type == 0) {
						ImGui::InputText("file name", file_name_buffer, 64);
					} else if (type == 1) {
						
						ImGui::SliderFloat("radius", &radius, 0, 100);
					} else if (type == 2) {
						ImGui::ColorEdit3("power", power,
							ImGuiColorEditFlags_Float |
							ImGuiColorEditFlags_NoDragDrop
						);
					} else if (type == 3) {
						ImGui::InputText("file name", file_name_buffer, 64);
						ImGui::ColorEdit3("power", power,
							ImGuiColorEditFlags_Float |
							ImGuiColorEditFlags_NoDragDrop
						);
						ImGui::SliderFloat("strength", &strength, 0, 10);
					}

					if (type == 0 || type == 1) {
						//material_combo_selected_id = 1; // default material

						static std::string selected = "";
						if (material_name_map.contains(material_combo_selected_id))
							selected = material_name_map[material_combo_selected_id];

						if (ImGui::BeginCombo("material", selected.c_str()))
						{
							for (const auto& [material_id, material_name] : material_name_map) {
								if (ImGui::Selectable(material_name.c_str(), material_id == material_combo_selected_id))
									material_combo_selected_id = material_id;
							}
							ImGui::EndCombo();
						}

					}


					if (ImGui::Button("Yes", ImVec2(120, 0))) {
						ImGui::CloseCurrentPopup();
						open_new_object_dialog = 0;

						json obj_info;
						obj_info["name"] = object_name_buffer;
						obj_info["world_pos"] = pos;
						obj_info["type"] = type_name[type];

						// set tree_path
						json tree_path = on_node["tree_path"];
						if (on_node["type"] == "folder") {
							tree_path.push_back(on_node["id"]);
						}
						obj_info["tree_path"] = tree_path;

						if (type == 0) {
							obj_info["file_name"] = file_name_buffer;
						}

						if (type == 1) {
							obj_info["radius"] = radius;
						}

						if (type == 0 || type == 1) {
							obj_info["material_id"] = material_combo_selected_id;
						}
						else if (type == 2) {
							obj_info["power"] = power;
						}
						else if (type == 3) {
							obj_info["power"] = power;
							obj_info["strength"] = strength;
							obj_info["samples"] = 4;
							obj_info["file_name"] = file_name_buffer;
						}

						auto [result, info] = PBR_API_add_object_to_scene(obj_info);
						selected_obj_id = info["id"];
						need_to_update_scene_tree = 1;
						dump_node_to_buffer(info);
					}

					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();
					if (ImGui::Button("No", ImVec2(120, 0))) {
						open_new_object_dialog = 0;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}
			};

			const auto delete_popup = [&](const json& node) -> void {
				ImGui::OpenPopup("Delete Object");
				std::string node_name = node["name"];
				ImVec2 center = ImGui::GetMainViewport()->GetCenter();
				ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
				if (ImGui::BeginPopupModal("Delete Object", NULL, 
					ImGuiWindowFlags_AlwaysAutoResize |
					ImGuiWindowFlags_NoMove
				))
				{

					ImGui::Text("Delete \"%s\" from scene! Are you sure?\n\n", node_name.c_str());
					ImGui::Separator();

					if (ImGui::Button("Yes", ImVec2(120, 0))) {
						ImGui::CloseCurrentPopup();
						open_delete_pop = 0;

						if (delete_node_type == 0) {
							PBR_API_delete_object_from_scene(node);
							need_to_update_scene_tree = 1;
						}
						else if(delete_node_type == 1) {
							PBR_API_delete_material(node);
							need_to_update_material_tree = 1;
							need_to_update_scene_tree = 1;
						}
						else if (delete_node_type == 2) {
							PBR_API_delete_medium(node);
							need_to_update_medium_tree = 1;
							need_to_update_scene_tree = 1;
						}
						
					}

					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();
					if (ImGui::Button("No", ImVec2(120, 0))) {
						ImGui::CloseCurrentPopup();
						open_delete_pop = 0;
					}

					ImGui::EndPopup();
				}
			};

			// recursive lambda 
			// https://stackoverflow.com/a/45824777

			static int set_focus = 0;

			const auto make_object_tree_table = [&](const auto& myself, const json& parent) -> void {

				for (auto& [node_id, node] : parent.items()) {
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					std::string node_name = node["name"];
					std::string node_type = node["type"];
					if (node["type"] == "folder") {
						
						bool open = ImGui::TreeNodeEx(node_name.c_str(), 
							ImGuiTreeNodeFlags_SpanFullWidth |
							ImGuiTreeNodeFlags_OpenOnDoubleClick |
							ImGuiTreeNodeFlags_OpenOnArrow
						);

						if (!edit_obj_name_status) {
							if (ImGui::BeginPopupContextItem())
							{
								ImGui::Text("This a popup for \"%s\"!", node_name.c_str());
								if (ImGui::Button("Delete")) {
									ImGui::CloseCurrentPopup();
									open_delete_pop = 1;
									delete_node_type = 0;
									op_node = node;
								}

								if (ImGui::Button("Add")) {
									ImGui::CloseCurrentPopup();
									open_new_object_dialog = 1;
									op_node = node;
								}
								ImGui::EndPopup();
							}
						}

						ImGui::TableNextColumn();
						ImGui::TextDisabled("--");
						//ImGui::TableNextColumn();
						//ImGui::TextUnformatted(node->Type);

						if (open)
						{
							myself(myself, node["children"]);
							ImGui::TreePop();
						}
					}
					else {
						//ImGui::TreeNodeEx(node_name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);
						
						if (edit_obj_name_status && selected_obj_id == node["id"]) {
							//ImGui::ActivateItem();
							if (set_focus) {
								ImGui::SetKeyboardFocusHere();
								set_focus = 0;
							}
							if (ImGui::InputText("##obj_name", object_name_buffer, 64,
								0//ImGuiInputTextFlags_EnterReturnsTrue
							)) {
								//std::cout << "name 1\n";
							}
							else {
								//std::cout << "name 2\n";
							}

							if (ImGui::IsItemDeactivated()) {
								//std::cout << "name 11\n";
								edit_obj_name_status = 0;

								if (ImGui::IsItemDeactivatedAfterEdit()) { //IsItemDeactivated   IsItemDeactivatedAfterEdit
									//std::cout << "name 22\n";
									auto new_name = PBR_API_rename_object(node["id"], object_name_buffer);

									sprintf_s(object_name_buffer, "%s", new_name.c_str());

									need_to_update_scene_tree = 1;
								}
							}
						}
						else {
							if (ImGui::Selectable(node_name.c_str(), selected_obj_id == node["id"], ImGuiSelectableFlags_AllowDoubleClick)) {
								// click object

								selected_obj_id = node["id"];
								selected_obj = node;
								dump_node_to_buffer(node);

								if (ImGui::IsMouseDoubleClicked(0)) {
									edit_obj_name_status = 1;
									set_focus = 1;
								}
							}
						}

						if (!edit_obj_name_status) {
							if (ImGui::BeginPopupContextItem())
							{
								ImGui::Text("This a popup for \"%s\"!", node_name.c_str());
								if (ImGui::Button("Delete")) {
									ImGui::CloseCurrentPopup();
									open_delete_pop = 1;
									delete_node_type = 0;
									op_node = node;
								}

								if (ImGui::Button("Add")) {
									ImGui::CloseCurrentPopup();
									open_new_object_dialog = 1;
									op_node = node;
								}
								ImGui::EndPopup();
							}
						}

						ImGui::TableNextColumn();
						ImGui::Text(node_type.c_str());
						//ImGui::TableNextColumn();
						//ImGui::TextUnformatted(node->Type);
					}
				}
			};

			if (ImGui::Begin("Object", nullptr,
				//ImGuiWindowFlags_AlwaysAutoResize |
				//ImGuiWindowFlags_NoMove |
				//ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_NoCollapse)) {

				static ImGuiTableFlags flags = 
					//ImGuiTableFlags_BordersV | 
					ImGuiTableFlags_BordersOuterH | 
					ImGuiTableFlags_Resizable | 
					ImGuiTableFlags_RowBg | 
					ImGuiTableFlags_NoBordersInBody;

				if (ImGui::BeginTable("ObjectTable", 2, flags)) {
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
					//ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
					ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
					ImGui::TableHeadersRow();

					make_object_tree_table(make_object_tree_table, scene_tree);
				}
				ImGui::EndTable();
				
			}
			ImGui::End();

			if (open_delete_pop) {
				delete_popup(op_node);
			}

			if (open_new_object_dialog) {
				new_object_popup(op_node);
			}
		}

		// Property
		{
		//ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 540, main_viewport->WorkPos.y + 40), ImGuiCond_FirstUseEver);
		//ImGui::SetNextWindowSize(ImVec2(cs.resolution[0] + 20, cs.resolution[1] + 50));

		if (ImGui::Begin("Property", nullptr,
			//ImGuiWindowFlags_AlwaysAutoResize |
			//ImGuiWindowFlags_NoMove |
			//ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoCollapse)) {
			//ImGui::Image((ImTextureID)app.renderImage.descriptorSet);
			//ImGui::Image((ImTextureID)renderImageID, ImVec2(80, 80), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			//Log("renderImageID %d", (ImTextureID)renderImageID);

			//std::cout << "renderImageID = " << renderImageID << std::endl;

			/*ImGui::Combo("type", &type, "mesh\0sphere\0polit_light\0infinite_light\0\0");
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				PBR_API_update_scene_object(json({
					{"id",selected_obj_id},
					{"world_pos", pos}
					}));
			}*/


			if (selected_obj_id != -1) {

				ImGui::Text(object_name_buffer);
				//ImGui::InputText("name", object_name_buffer, 64);

				int update_object = 0;

				ImGui::SliderFloat3("world_position", pos, -100, 100, "%0.5f");
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					update_object = 1;
				}

				if(ImGui::Combo("type", &type, "mesh\0sphere\0polit_light\0infinite_light\0\0")){
					update_object = 1;
				}

				if (type == 0) {
					ImGui::InputText("file name", file_name_buffer, 64);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						update_object = 1;
					}
				}
				else if (type == 1) {
					ImGui::SliderFloat("radius", &radius, 0, 100);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						update_object = 1;
					}
				}
				else if (type == 2) {
					ImGui::ColorEdit3("power", power,
						ImGuiColorEditFlags_Float |
						ImGuiColorEditFlags_NoDragDrop
					);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						update_object = 1;
					}
				}
				else if (type == 3) {
					ImGui::InputText("file name", file_name_buffer, 64);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						update_object = 1;
					}

					ImGui::ColorEdit3("power", power,
						ImGuiColorEditFlags_Float |
						ImGuiColorEditFlags_NoDragDrop
					);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						update_object = 1;
					}

					ImGui::SliderFloat("strength", &strength, 0, 10);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						update_object = 1;
					}
				}

				if (type == 0 || type == 1) {

					// object material
					std::string material_string = "None";
					if (material_name_map.contains(material_combo_selected_id)) {
						material_string = material_name_map[material_combo_selected_id];
					}

					if (ImGui::BeginCombo("material", material_string.c_str())) {
						for (const auto& [material_id, material_name] : material_name_map) {
							if (ImGui::Selectable(material_name.c_str(), material_id == material_combo_selected_id)) {
								material_combo_selected_id = material_id;
								update_object = 1;
							}
						}
						ImGui::EndCombo();
					}

					// object medium
					std::string medium_string = "";
					if (medium_name_map.contains(object_medium_combo_selected_id)) {
						medium_string = medium_name_map[object_medium_combo_selected_id];
					}

					if (ImGui::BeginCombo("medium", medium_string.c_str())) {
						for (const auto& [medium_id, medium_name] : medium_name_map) {
							if (ImGui::Selectable(medium_name.c_str(), medium_id == object_medium_combo_selected_id)) {
								object_medium_combo_selected_id = medium_id;
								update_object = 1;
							}
						}
						ImGui::EndCombo();
					}
				}


				if (update_object) {

					json obj_info;
					obj_info["id"] = selected_obj_id;
					obj_info["name"] = object_name_buffer;
					obj_info["world_pos"] = pos;
					obj_info["type"] = type_name[type];

					// set tree_path
					json tree_path = selected_obj["tree_path"];
					if (selected_obj["type"] == "folder") {
						tree_path.push_back(selected_obj["id"]);
					}
					obj_info["tree_path"] = tree_path;

					if (type == 0) {
						obj_info["file_name"] = file_name_buffer;
					}

					if (type == 1) {
						obj_info["radius"] = radius;
					}

					if (type == 0 || type == 1) {
						obj_info["material_id"] = material_combo_selected_id;
						obj_info["medium_id"] = object_medium_combo_selected_id;
					}
					else if (type == 2) {
						obj_info["power"] = power;
					}
					else if (type == 3) {
						obj_info["power"] = power;
						obj_info["strength"] = strength;
						obj_info["samples"] = 4;
						obj_info["file_name"] = file_name_buffer;
					}

					PBR_API_update_scene_object(obj_info);
					need_to_update_scene_tree = 1;
				}
			}
		}
		ImGui::End();
		}

		// Material
		{
			//static int open_delete_pop = 0;
			//static json op_node;

			static int set_focus = 0;

			const auto make_material_tree_table = [&](const auto& myself, const json& parent) -> void {

				for (auto& [node_id, node] : parent.items()) {
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					std::string node_name = node["name"];
					std::string node_type = node["type"];

					if(0) {// (node["type"] == "folder") {
						bool open = ImGui::TreeNodeEx(node_name.c_str(),
							ImGuiTreeNodeFlags_SpanFullWidth |
							ImGuiTreeNodeFlags_OpenOnDoubleClick |
							ImGuiTreeNodeFlags_OpenOnArrow
						);

						if (ImGui::BeginPopupContextItem())
						{
							ImGui::Text("This a popup for \"%s\"!", node_name.c_str());
							if (ImGui::Button("Delete")) {
								ImGui::CloseCurrentPopup();
								open_delete_pop = 1;
								op_node = node;
								delete_node_type = 1;
							}

							/*if (ImGui::Button("Add")) {
								ImGui::CloseCurrentPopup();
								open_new_object_dialog = 1;
								op_node = node;
							}*/
							ImGui::EndPopup();
						}

						ImGui::TableNextColumn();
						ImGui::TextDisabled("--");
						//ImGui::TableNextColumn();
						//ImGui::TextUnformatted(node->Type);



						if (open)
						{
							myself(myself, node["children"]);
							ImGui::TreePop();
						}
					}
					else {
						//ImGui::TreeNodeEx(node_name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);

						if (edit_material_name_status && selected_material_id == node["id"]) {
							//ImGui::ActivateItem();
							if (set_focus) {
								ImGui::SetKeyboardFocusHere();
								set_focus = 0;
							}
							if (ImGui::InputText("##material_name", material_name_buffer, 64, 
								0//ImGuiInputTextFlags_EnterReturnsTrue
							)) {
								//std::cout << "name 1\n";
							}
							else {
								//std::cout << "name 2\n";
							}

							if (ImGui::IsItemDeactivated()) {
								//std::cout << "name 11\n";
								edit_material_name_status = 0;

								if (ImGui::IsItemDeactivatedAfterEdit()) { //IsItemDeactivated   IsItemDeactivatedAfterEdit
									//std::cout << "name 22\n";
									auto new_name = PBR_API_rename_material(node["id"], material_name_buffer);

									sprintf_s(material_name_buffer, "%s", new_name.c_str());

									need_to_update_material_tree = 1;
								}
							}
						}
						else {
							if (ImGui::Selectable(node_name.c_str(), selected_material_id == node["id"], ImGuiSelectableFlags_AllowDoubleClick)) {
								selected_material_id = node["id"];
								selected_material = node;
								dump_material_node_to_buffer(node);

								if (ImGui::IsMouseDoubleClicked(0)) {
									//std::cout << "db click\n";
									edit_material_name_status = 1;
									set_focus = 1;
								}
							}
						}
						
						if (!edit_material_name_status) {
							if (ImGui::BeginPopupContextItem())
							{
								ImGui::Text("This a popup for \"%s\"!", node_name.c_str());
								if (ImGui::Button("Delete")) {
									ImGui::CloseCurrentPopup();
									open_delete_pop = 1;
									op_node = node;
									delete_node_type = 1;
								}

								if (ImGui::Button("Add")) {
									ImGui::CloseCurrentPopup();
									open_new_object_dialog = 1;
									op_node = node;
								}
								ImGui::EndPopup();
							}
						}
						

						ImGui::TableNextColumn();
						ImGui::Text(node_type.c_str());
						//ImGui::TableNextColumn();
						//ImGui::TextUnformatted(node->Type);
					}
				}
			};

			if (ImGui::Begin("Material", nullptr,
				//ImGuiWindowFlags_AlwaysAutoResize |
				//ImGuiWindowFlags_NoMove |
				//ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_NoCollapse)) {

				//ImGui::SetNextItemWidth();
				if (ImGui::Button("new material", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
					selected_material = PBR_API_new_material();
					selected_material_id = selected_material["id"];
					need_to_update_material_tree = 1;
					dump_material_node_to_buffer(selected_material);
				}

				static ImGuiTableFlags flags =
					//ImGuiTableFlags_BordersV | 
					ImGuiTableFlags_BordersOuterH |
					ImGuiTableFlags_Resizable |
					ImGuiTableFlags_RowBg |
					ImGuiTableFlags_NoBordersInBody;

				if (ImGui::BeginTable("MaterialTable", 2, flags)) {
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
					//ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
					ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
					ImGui::TableHeadersRow();

					make_material_tree_table(make_material_tree_table, material_tree);
				}
				ImGui::EndTable();

				if (ImGui::CollapsingHeader("Property")) {
					if (selected_material_id != -1) {

						int update_material = 0;

						//ImGui::InputText("name", material_name_buffer, 64, ImGuiInputTextFlags_ReadOnly);
						ImGui::Text(material_name_buffer);

						if (ImGui::Combo("material", &material_type, "matte\0glass\0mirror\0plastic\0metal\0\0"))
						{
							/*PBR_API_update_scene_object(json({
								{"id",selected_obj_id},
								{"world_pos", pos}
								}));*/
							update_material = 1;
						}


						switch (material_type) {
						case 0:
							ImGui::ColorEdit3("kd", kd,
								ImGuiColorEditFlags_Float |
								ImGuiColorEditFlags_NoDragDrop
							);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							ImGui::SliderFloat("sigma", &sigma, 0, 1);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							break;
						case 1:
							ImGui::ColorEdit3("kr", kr,
								ImGuiColorEditFlags_Float |
								ImGuiColorEditFlags_NoDragDrop
							);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							ImGui::ColorEdit3("kt", kt,
								ImGuiColorEditFlags_Float |
								ImGuiColorEditFlags_NoDragDrop
							);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							ImGui::SliderFloat("eta", &eta, 0, 3);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							ImGui::SliderFloat2("uvrough", uvrough, 0, 1);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}
							break;
						case 2:
							ImGui::ColorEdit3("kr", kr,
								ImGuiColorEditFlags_Float |
								ImGuiColorEditFlags_NoDragDrop
							);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							break;
						case 3:
							ImGui::ColorEdit3("kd", kd,
								ImGuiColorEditFlags_Float |
								ImGuiColorEditFlags_NoDragDrop
							);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							ImGui::ColorEdit3("ks", ks,
								ImGuiColorEditFlags_Float |
								ImGuiColorEditFlags_NoDragDrop
							);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							ImGui::SliderFloat("rough", &rough, 0, 1);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							break;
						case 4:
							ImGui::ColorEdit3("kd", kd,
								ImGuiColorEditFlags_Float |
								ImGuiColorEditFlags_NoDragDrop
							);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							ImGui::SliderFloat3("eta", eta3, 0, 3);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}

							ImGui::SliderFloat("rough", &rough, 0, 1);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_material = 1;
							}
							break;
						}

						if (update_material) {

							json material_info = json({
								{"id", selected_material_id},
								{"name", selected_material["name"]}
								});

							switch (material_type) {
							case 0:
								material_info["type"] = "matte";
								material_info["bumpmap"] = 0;
								material_info["remaproughness"] = false;
								material_info["kd"] = kd;
								material_info["sigma"] = sigma;

								break;
							case 1:
								material_info["type"] = "glass";
								material_info["bumpmap"] = 0;
								material_info["remaproughness"] = false;
								material_info["kr"] = kr;
								material_info["kt"] = kt;
								material_info["eta"] = eta;
								material_info["uroughness"] = uvrough[0];
								material_info["vroughness"] = uvrough[1];

								break;
							case 2:
								material_info["type"] = "mirror";
								material_info["bumpmap"] = 0;
								material_info["remaproughness"] = false;
								material_info["kr"] = kr;

								break;
							case 3:
								material_info["type"] = "plastic";
								material_info["bumpmap"] = 0;
								material_info["remaproughness"] = false;
								material_info["kd"] = kd;
								material_info["ks"] = ks;
								material_info["roughness"] = rough;

								break;
							case 4:
								material_info["type"] = "metal";
								material_info["bumpmap"] = 0;
								material_info["remaproughness"] = false;
								material_info["k"] = kd;
								material_info["eta"] = eta3;
								material_info["roughness"] = rough;

								break;
							}

							PBR_API_update_material(material_info);
							need_to_update_material_tree = 1;
						}
					}
				}
				

				if (ImGui::CollapsingHeader("Preview")) {

				}

			}
			ImGui::End();
		}

		// Medium
		{
			static int set_focus = 0;

			const auto make_medium_tree_table = [&](const auto& myself, const json& parent) -> void {

				for (auto& [node_id, node] : parent.items()) {
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					std::string node_name = node["name"];
					std::string node_type = node["type"];
					
					if (edit_medium_name_status && selected_medium_id == node["id"]) {
						//ImGui::ActivateItem();
						if (set_focus) {
							ImGui::SetKeyboardFocusHere();
							set_focus = 0;
						}
						if (ImGui::InputText("##medium_name", medium_name_buffer, 64,
							0//ImGuiInputTextFlags_EnterReturnsTrue
						)) {
							//std::cout << "name 1\n";
						}
						else {
							//std::cout << "name 2\n";
						}

						if (ImGui::IsItemDeactivated()) {
							//std::cout << "name 11\n";
							edit_medium_name_status = 0;

							if (ImGui::IsItemDeactivatedAfterEdit()) { //IsItemDeactivated   IsItemDeactivatedAfterEdit
								//std::cout << "name 22\n";
								auto new_name = PBR_API_rename_medium(node["id"], medium_name_buffer);

								sprintf_s(medium_name_buffer, "%s", new_name.c_str());

								need_to_update_medium_tree = 1;
							}
						}
					}
					else {
						if (ImGui::Selectable(node_name.c_str(), selected_medium_id == node["id"], ImGuiSelectableFlags_AllowDoubleClick)) {
							selected_medium_id = node["id"];
							selected_medium = node;
							dump_medium_node_to_buffer(node);

							if (ImGui::IsMouseDoubleClicked(0)) {
								//std::cout << "db click\n";
								edit_medium_name_status = 1;
								set_focus = 1;
							}
						}
					}

					if (!edit_medium_name_status) {
						if (ImGui::BeginPopupContextItem())
						{
							ImGui::Text("This a popup for \"%s\"!", node_name.c_str());
							if (ImGui::Button("Delete")) {
								ImGui::CloseCurrentPopup();
								open_delete_pop = 1;
								op_node = node;
								delete_node_type = 2;
							}

							if (ImGui::Button("Add")) {
								ImGui::CloseCurrentPopup();
								open_new_object_dialog = 1;
								op_node = node;
							}
							ImGui::EndPopup();
						}
					}


					ImGui::TableNextColumn();
					ImGui::Text(node_type.c_str());
					//ImGui::TableNextColumn();
					//ImGui::TextUnformatted(node->Type);
				}
			};

			if (ImGui::Begin("Medium", nullptr,
				//ImGuiWindowFlags_AlwaysAutoResize |
				//ImGuiWindowFlags_NoMove |
				//ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_NoCollapse)) {

				//ImGui::SetNextItemWidth();
				if (ImGui::Button("new medium", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
					selected_medium = PBR_API_new_medium();
					selected_medium_id = selected_medium["id"];
					need_to_update_medium_tree = 1;
					dump_medium_node_to_buffer(selected_medium);
				}

				static ImGuiTableFlags flags =
					//ImGuiTableFlags_BordersV | 
					ImGuiTableFlags_BordersOuterH |
					ImGuiTableFlags_Resizable |
					ImGuiTableFlags_RowBg |
					ImGuiTableFlags_NoBordersInBody;

				if (ImGui::BeginTable("MediumTable", 2, flags)) {
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
					//ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
					ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
					ImGui::TableHeadersRow();

					make_medium_tree_table(make_medium_tree_table, medium_tree);
				}
				ImGui::EndTable();

				if (ImGui::CollapsingHeader("Property")) {
					if (selected_medium_id != -1) {

						int update_medium = 0;

						//ImGui::InputText("name", material_name_buffer, 64, ImGuiInputTextFlags_ReadOnly);
						ImGui::Text(medium_name_buffer);

						if (ImGui::Combo("medium", &medium_type, "homo\0\0"))
						{
							/*PBR_API_update_scene_object(json({
								{"id",selected_obj_id},
								{"world_pos", pos}
								}));*/
							update_medium = 1;
						}


						switch (medium_type) {
						case 0:
							ImGui::ColorEdit3("sigma_a", sigma_a,
								ImGuiColorEditFlags_Float |
								ImGuiColorEditFlags_NoDragDrop
							);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_medium = 1;
							}

							ImGui::ColorEdit3("sigma_s", sigma_s,
								ImGuiColorEditFlags_Float |
								ImGuiColorEditFlags_NoDragDrop
							);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_medium = 1;
							}

							ImGui::SliderFloat("g", &medium_g, 0, 1);
							if (ImGui::IsItemDeactivatedAfterEdit()) {
								update_medium = 1;
							}
							break;
						}

						if (update_medium) {

							json medium_info = json({
								{"id", selected_medium_id},
								{"name", selected_medium["name"]}
								});

							switch (medium_type) {
							case 0:
								medium_info["type"] = "homo";
								medium_info["sigma_s"] = sigma_s;
								medium_info["sigma_a"] = sigma_a;
								medium_info["g"] = medium_g;

								break;
							}

							PBR_API_update_medium(medium_info);
							need_to_update_medium_tree = 1;
						}
					}
				}


				if (ImGui::CollapsingHeader("Preview")) {

				}

			}
			ImGui::End();
		}

		{
			//ImGui::SetNextWindowSize(ImVec2(1024 + 20, 1024 + 50));
			//ImGui::SetNextWindowPos(ImVec2(512, 20));

			/*ImGui::Begin("mipmap");

			ImGui::Image((ImTextureID)app->testImages[0]->descriptorSet, ImVec2(app->testImages[0]->width, app->testImages[0]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::Image((ImTextureID)app->testImages[1]->descriptorSet, ImVec2(app->testImages[1]->width, app->testImages[1]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::Image((ImTextureID)app->testImages[2]->descriptorSet, ImVec2(app->testImages[2]->width, app->testImages[2]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::Image((ImTextureID)app->testImages[3]->descriptorSet, ImVec2(app->testImages[3]->width, app->testImages[3]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::Image((ImTextureID)app->testImages[4]->descriptorSet, ImVec2(app->testImages[4]->width, app->testImages[4]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			ImGui::Image((ImTextureID)app->testImages[5]->descriptorSet, ImVec2(app->testImages[5]->width, app->testImages[5]->height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

			ImGui::End();*/
		}

		{
			ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 20, main_viewport->WorkPos.y + 40), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(480, 600), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowBgAlpha(1);

			if (ImGui::Begin("Control", nullptr,
				//ImGuiWindowFlags_AlwaysAutoResize |
				//ImGuiWindowFlags_NoDocking |
				//ImGuiWindowFlags_NoMove |
				//ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_NoCollapse)) {

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

				bool cameraChanged = false;
				bool system_config_changed = false;
				static bool integrator_changed = false;

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

				static std::string medium_string = "";
				if (medium_name_map.contains(medium_combo_selected_id)) {
					medium_string = medium_name_map[medium_combo_selected_id];
				}

				if (ImGui::BeginCombo("camera medium", medium_string.c_str())) {
					for (const auto& [medium_id, medium_name] : medium_name_map) {
						if (ImGui::Selectable(medium_name.c_str(), medium_id == medium_combo_selected_id)) {
							medium_combo_selected_id = medium_id;
							cameraChanged = true;

							cs.medium_id = medium_combo_selected_id;
						}
					}
					ImGui::EndCombo();
				}

				//ImGui::PopItemWidth();

				//ImGui::PushItemWidth(160);

				ImGui::SliderFloat("fov", &cs.fov, 0, 180);
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					cameraChanged = true;
				}

				//ImGui::SameLine();

				ImGui::SliderFloat("aspect_ratio", &cs.asp, 0.5f, 2, "%.1f");
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					cameraChanged = true;
				}

				//ImGui::PopItemWidth();

				//ImGui::PushItemWidth(400);

				ImGui::SliderFloat2("near_far", cs.near_far, 0.001f, 1000);
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					cameraChanged = true;
				}

				ImGui::SliderInt2("resolution", cs.resolution, 0, 1000);
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					cameraChanged = true;
				}

				if (ImGui::SliderInt("scale", &cs.image_scale, 1, 10)) {
					cs.resolution[0] = cs.image_scale * 128;
					cs.resolution[1] = cs.image_scale * 128;
					cameraChanged = true;
				}

				ImGui::SliderInt("render_threads", &system_config.render_threads_no, 1, 16);
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					system_config_changed = true;
				}

				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor(0.2f, 0.8f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, (ImVec4)ImColor(0.2f, 0.8f, 0.3f));
				
				std::string progressString = std::format("render progress\n\nrender duration: \n{}s", float(render_status_info["render_duration"]) / 1000);
				
				ImGui::PlotHistogram(progressString.c_str(), progress_per.data(), int(progress_per.size()), 0, NULL, 0.0f, 1.0f, ImVec2(0, 100.0f));

				ImGui::PopStyleColor(2);

				if (scene_options.render_method == 3) {
					ImGui::Text("current iteration: %d", int(render_status_info["currentIteration"]));
				}

				if (cameraChanged) {
					Log("cameraChanged");
					PBR_API_set_perspective_camera(cs);
				}

				if (system_config_changed) {
					PbrApiSetSystemConfig(system_config);
				}

				ImGui::Separator();

				ImGui::Text("Scene Setting");
				ImGui::Text("Nodes structure:");
				ImGui::SameLine();

				if (ImGui::RadioButton("brute force", &scene_options.nodes_structure, 0)) {
					PBR_API_SET_SCENE_OPTIONS(scene_options);
				}

				ImGui::SameLine();

				if (ImGui::RadioButton("BVH", &scene_options.nodes_structure, 1)) {
					PBR_API_SET_SCENE_OPTIONS(scene_options);
				}

				ImGui::Text("Method:");
				ImGui::SameLine();

				if (ImGui::RadioButton("whitted", &scene_options.render_method, 0)) {
					PbrApiSelectIntegrator(scene_options.render_method);
					integrator_changed = true;
				}

				ImGui::SameLine();

				if (ImGui::RadioButton("path", &scene_options.render_method, 1)) {
					PbrApiSelectIntegrator(scene_options.render_method);
					integrator_changed = true;
				}

				ImGui::SameLine();

				if (ImGui::RadioButton("vpath", &scene_options.render_method, 4)) {
					PbrApiSelectIntegrator(scene_options.render_method);
					integrator_changed = true;
				}

				ImGui::SameLine();

				if (ImGui::RadioButton("pm", &scene_options.render_method, 2)) {
					PbrApiSelectIntegrator(scene_options.render_method);
					integrator_changed = true;
				}

				ImGui::SameLine();

				if (ImGui::RadioButton("ppm", &scene_options.render_method, 3)) {
					PbrApiSelectIntegrator(scene_options.render_method);
					integrator_changed = true;
				}

				if (scene_options.render_method == 0) { // whitted
					ImGui::SliderInt("ray_sample_no", &whitted_int_config.ray_sample_no, 1, 500);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::SliderInt("ray_bounce_no", &whitted_int_config.ray_bounce_no, 0, 16);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					if (integrator_changed) {
						json integrator_info;
						integrator_info["ray_sample_no"] = whitted_int_config.ray_sample_no;
						integrator_info["ray_bounce_no"] = whitted_int_config.ray_bounce_no;
						//integrator_info["render_threads_no"] = whitted_int_config.render_threads_no;

						json data;
						data["whitted"] = integrator_info;
						PbrApiSetIntegrator(data);
						integrator_changed = false;
					}
				}
				else if (scene_options.render_method == 1) { // path
					ImGui::SliderInt("ray_sample_no", &path_int_config.ray_sample_no, 1, 500);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::SliderInt("ray_bounce_no", &path_int_config.ray_bounce_no, 0, 16);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					if (integrator_changed) {
						json integrator_info;
						integrator_info["ray_sample_no"] = path_int_config.ray_sample_no;
						integrator_info["ray_bounce_no"] = path_int_config.ray_bounce_no;
						//integrator_info["render_threads_no"] = path_int_config.render_threads_no;

						json data;
						data["path"] = integrator_info;
						PbrApiSetIntegrator(data);
						integrator_changed = false;
					}
				}
				else if (scene_options.render_method == 4) { // vpath
					ImGui::SliderInt("ray_sample_no", &vpath_int_config.ray_sample_no, 1, 500);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::SliderInt("ray_bounce_no", &vpath_int_config.ray_bounce_no, 0, 16);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					if (integrator_changed) {
						json integrator_info;
						integrator_info["ray_sample_no"] = vpath_int_config.ray_sample_no;
						integrator_info["ray_bounce_no"] = vpath_int_config.ray_bounce_no;
						//integrator_info["render_threads_no"] = vpath_int_config.render_threads_no;

						json data;
						data["vpath"] = integrator_info;
						PbrApiSetIntegrator(data);
						integrator_changed = false;
					}
				}
				else if (scene_options.render_method == 2) {  // pm
					ImGui::SliderInt("ray_sample_no", &pm_int_config.ray_sample_no, 1, 500);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::SliderInt("ray_bounce_no", &pm_int_config.ray_bounce_no, 0, 16);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::SliderInt("emit photons", &pm_int_config.emit_photons, 0, 100000);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::Text("gather method:");
					ImGui::SameLine();

					if (ImGui::RadioButton("count", &pm_int_config.gather_method, 0)) {
						integrator_changed = true;
					}

					ImGui::SameLine();

					if (ImGui::RadioButton("x/1000", &pm_int_config.gather_method, 1)) {
						integrator_changed = true;
					}

					ImGui::SliderInt("gather photons", &pm_int_config.gather_photons, 0, 500);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::SliderFloat("gather photons radius(0=no limit)", &pm_int_config.gather_photons_r, 0, 2);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::InputFloat("energy scale", &pm_int_config.energy_scale);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::Text("filter:");
					ImGui::SameLine();

					if (ImGui::RadioButton("None", &pm_int_config.filter, 0)) {
						integrator_changed = true;
					}

					ImGui::SameLine();

					if (ImGui::RadioButton("Cone", &pm_int_config.filter, 1)) {
						integrator_changed = true;
					}

					ImGui::SameLine();

					if (ImGui::RadioButton("Gaussian", &pm_int_config.filter, 2)) {
						integrator_changed = true;
					}

					if (ImGui::Checkbox("reemit photons", &pm_int_config.reemit_photons)) {
						integrator_changed = true;
					}

					if (ImGui::Checkbox("render direct", &pm_int_config.render_direct)) {
						integrator_changed = true;
					}

					if (ImGui::Checkbox("render specular", &pm_int_config.render_specular)) {
						integrator_changed = true;
					}

					ImGui::SameLine();

					if (ImGui::RadioButton("Monte Carlo##1", &pm_int_config.specular_method, 0)) {
						integrator_changed = true;
					}

					ImGui::SameLine();

					if (ImGui::RadioButton("whitted##", &pm_int_config.specular_method, 1)) {
						integrator_changed = true;
					}

					ImGui::SliderInt("samples", &pm_int_config.specular_rt_samples, 1, 50);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					if (ImGui::Checkbox("render caustic", &pm_int_config.render_caustic)) {
						integrator_changed = true;
					}

					if (ImGui::Checkbox("render diffuse", &pm_int_config.render_diffuse)) {
						integrator_changed = true;
					}

					if (ImGui::Checkbox("render global", &pm_int_config.render_global)) {
						integrator_changed = true;
					}

					if (integrator_changed) {
						json integrator_info;
						integrator_info["ray_sample_no"] = pm_int_config.ray_sample_no;
						integrator_info["ray_bounce_no"] = pm_int_config.ray_bounce_no;
						//integrator_info["render_threads_no"] = pm_int_config.render_threads_no;

						integrator_info["emit_photons"] = pm_int_config.emit_photons;
						integrator_info["gather_photons"] = pm_int_config.gather_photons;
						integrator_info["gather_photons_r"] = pm_int_config.gather_photons_r;
						integrator_info["gather_method"] = pm_int_config.gather_method;
						integrator_info["filter"] = pm_int_config.filter;
						integrator_info["energy_scale"] = pm_int_config.energy_scale;
						integrator_info["reemit_photons"] = pm_int_config.reemit_photons;

						integrator_info["render_direct"] = pm_int_config.render_direct;
						integrator_info["render_specular"] = pm_int_config.render_specular;
						integrator_info["render_caustic"] = pm_int_config.render_caustic;
						integrator_info["render_diffuse"] = pm_int_config.render_diffuse;
						integrator_info["render_global"] = pm_int_config.render_global;

						integrator_info["specular_method"] = pm_int_config.specular_method;
						integrator_info["specular_rt_samples"] = pm_int_config.specular_rt_samples;

						json data;
						data["pm"] = integrator_info;
						PbrApiSetIntegrator(data);
						integrator_changed = false;
					}
				} else if (scene_options.render_method == 3) { // ppm
					ImGui::SliderInt("ray_sample_no", &ppm_int_config.ray_sample_no, 1, 500);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::SliderInt("ray_bounce_no", &ppm_int_config.ray_bounce_no, 0, 16);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::SliderFloat("alpha", &ppm_int_config.alpha, 0, 1);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::SliderFloat("initial radius", &ppm_int_config.inital_radius, 0, 10);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::InputFloat("energy scale", &ppm_int_config.energy_scale);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					ImGui::Text("filter:");
					ImGui::SameLine();

					if (ImGui::RadioButton("None", &ppm_int_config.filter, 0)) {
						integrator_changed = true;
					}

					ImGui::SameLine();

					if (ImGui::RadioButton("Cone", &ppm_int_config.filter, 1)) {
						integrator_changed = true;
					}

					ImGui::SameLine();

					if (ImGui::RadioButton("Gaussian", &ppm_int_config.filter, 2)) {
						integrator_changed = true;
					}

					if (ImGui::Checkbox("render direct", &ppm_int_config.render_direct)) {
						integrator_changed = true;
					}

					if (ImGui::Checkbox("render caustic", &ppm_int_config.render_caustic)) {
						integrator_changed = true;
					}

					if (ImGui::Checkbox("render diffuse", &ppm_int_config.render_diffuse)) {
						integrator_changed = true;
					}

					if (ImGui::Checkbox("render global", &ppm_int_config.render_global)) {
						integrator_changed = true;
					}

					ImGui::SliderInt("max iterations", &ppm_int_config.max_iterations, 1, 200);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						integrator_changed = true;
					}

					if (integrator_changed) {
						json integrator_info;
						integrator_info["ray_sample_no"] = ppm_int_config.ray_sample_no;
						integrator_info["ray_bounce_no"] = ppm_int_config.ray_bounce_no;
						//integrator_info["render_threads_no"] = ppm_int_config.render_threads_no;

						integrator_info["alpha"] = ppm_int_config.alpha;
						integrator_info["initalRadius"] = ppm_int_config.inital_radius;
						integrator_info["filter"] = ppm_int_config.filter;
						integrator_info["energy_scale"] = ppm_int_config.energy_scale;
						integrator_info["render_direct"] = ppm_int_config.render_direct;
						integrator_info["render_caustic"] = ppm_int_config.render_caustic;
						integrator_info["render_diffuse"] = ppm_int_config.render_diffuse;
						integrator_info["render_global"] = ppm_int_config.render_global;
						integrator_info["max_iterations"] = ppm_int_config.max_iterations;

						json data;
						data["ppm"] = integrator_info;
						PbrApiSetIntegrator(data);
						integrator_changed = false;
					}
				}

				if (ImGui::Button("Render", ImVec2(200, 120))) {

					static int set_default_scene = 1;
					if (set_default_scene == 0) {
						set_default_scene = 1;
					}

					SendEvent(thread_ctx, RENDER_TASK_ID);
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
			}

			ImGui::End();
		}

		ImGui::End();
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