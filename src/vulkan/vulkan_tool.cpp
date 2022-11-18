#include"vulkan_tool.h"
#include"vulkan_device.h"
#include<fstream>
#include<cassert>
#include<vector>
#include<random>

namespace vktool {

    VkShaderModule LoadSPIRVShader(VkDevice device, const std::string& filename)
    {
        std::cout << "loadSPIRVShader " << filename << std::endl;
        size_t shaderSize;
        char* shaderCode = nullptr;

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
            if (vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule)) {
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


	void setImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask)
	{
		// Create an image barrier object
		VkImageMemoryBarrier imageMemoryBarrier = vktool::imageMemoryBarrier();
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (oldImageLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageMemoryBarrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newImageLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (imageMemoryBarrier.srcAccessMask == 0)
			{
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(
			cmdbuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}

	// Fixed sub resource on first mip level and layer
	void setImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask)
	{
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;
		setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
	}

}



VulkanImage::VulkanImage() {

}

VulkanImage::~VulkanImage() {
	//Clean();
}

int VulkanImage::Clean() {
	std::cout << "VulkanImage.Clean()" << std::endl;
	if (device == nullptr)
		return 0;
	
	vkFreeMemory(device->device, memory, nullptr);
	vkDestroyImage(device->device, image, nullptr);
	vkDestroyImageView(device->device, view, nullptr);
	vkDestroySampler(device->device, sampler, nullptr);
	vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);

	return 0;
}

int VulkanImage::BuildImage(std::shared_ptr<VulkanDevice> device, uint32_t width, uint32_t height, unsigned char * pixels) {
	std::cout << "VulkanImage.BuildImage()" << std::endl;

	this->status = 1;

	this->device = device;
	this->width = width;
	this->height = height;

	size_t uploadSize = width * height * 4 * sizeof(char);

	/*std::vector<char> pixels(width * height * 4);
	for (int i = 0; i < width * height * 4; ++i) {
		pixels[i] = 
	}*/

	//if(0) // random image
	//{
	//	std::random_device rd;
	//	std::mt19937 gen(rd());
	//	std::uniform_int_distribution<> distrib(0, 255);

	//	for (auto i = 0; i < pixels.size(); ++i) {
	//		pixels[i] = distrib(gen);
	//	}
	//}

	// VkResult err;

	// Create the Image:
	{
		std::cout << "vkCreateImage" << std::endl;
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.extent.width = width;
		info.extent.height = height;
		info.extent.depth = 1;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VK_CHECK_RESULT(vkCreateImage(device->device, &info, nullptr, &image));

		VkMemoryRequirements req;
		vkGetImageMemoryRequirements(device->device, image, &req);
		std::cout << "VkMemoryRequirements.memoryTypeBits = " << req.memoryTypeBits<< std::endl;

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = req.size;
		alloc_info.memoryTypeIndex = device->getMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->device, &alloc_info, nullptr, &memory));

		VK_CHECK_RESULT(vkBindImageMemory(device->device, image, memory, 0));

	}

	// Create the Image View:
	{
		std::cout << "vkCreateImageView" << std::endl;

		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;
		VK_CHECK_RESULT(vkCreateImageView(device->device, &info, nullptr, &view));
	}

	// Descriptor Pool
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			//vktool::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),	// Vertex shader UBO
			//vktool::descriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1),		// Sampled image
			//vktool::descriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 3),			// 3 samplers (array)

			vktool::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vktool::descriptorPoolCreateInfo(
				static_cast<uint32_t>(poolSizes.size()),
				poolSizes.data(),
				1);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device->device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0: Vertex shader uniform buffer
			//vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			// Binding 1: Sampled image
			//vktool::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 0),// 1),
			// Binding 2: Sampler array (3 descriptors)
			//vktool::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 3),
			//vktool::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 3),
			vktool::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1),
		};

		// 这里的descriptorPoolInfo和setLayoutBindings暂时凑成和font descriptorSet一样。否则报错格式不匹配。
		// 因为setLayoutBindings最终会关联到pipeline。
		// 可能正规的做法要么统一定一套layout配置。要么就得区分pipeline。

		VkDescriptorSetLayoutCreateInfo descriptorLayout = vktool::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->device, &descriptorLayout, nullptr, &descriptorSetLayout));
	}

	// sampler
	{
		VkSamplerCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.magFilter = VK_FILTER_LINEAR;
		info.minFilter = VK_FILTER_LINEAR;
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.minLod = -1000;
		info.maxLod = 1000;
		info.maxAnisotropy = 1.0f;
		VK_CHECK_RESULT(vkCreateSampler(device->device, &info, nullptr, &sampler));
	}

	// Create the Descriptor Set:
	//descriptorSet = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Create Descriptor Set:
	{
		std::cout << "vkAllocateDescriptorSets" << std::endl;

		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = descriptorPool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &descriptorSetLayout;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device->device, &alloc_info, &descriptorSet));
	}

	// Update the Descriptor Set:
	{
		std::cout << "vkUpdateDescriptorSets" << std::endl;
		VkDescriptorImageInfo desc_image[1] = {};
		desc_image[0].sampler = sampler;
		desc_image[0].imageView = view;
		desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkWriteDescriptorSet write_desc[1] = {};
		write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc[0].dstSet = descriptorSet;
		write_desc[0].descriptorCount = 1;
		write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_desc[0].pImageInfo = desc_image;
		vkUpdateDescriptorSets(device->device, 1, write_desc, 0, NULL);
	}

	// Create the Upload Buffer:

	VulkanBuffer stagingBuffer;
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		uploadSize));

	/*
	{
		VkBufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = uploadSize;
		buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(device->device, &buffer_info, nullptr, &stagingBuffer.buffer));
		VkMemoryRequirements req;
		vkGetBufferMemoryRequirements(device->device, stagingBuffer.buffer, &req);
		//bd->BufferMemoryAlignment = (bd->BufferMemoryAlignment > req.alignment) ? bd->BufferMemoryAlignment : req.alignment;
		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = req.size;
		alloc_info.memoryTypeIndex = device->getMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
		VK_CHECK_RESULT(vkAllocateMemory(device->device, &alloc_info, nullptr, &stagingBuffer.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device->device, stagingBuffer.buffer, stagingBuffer.memory, 0));
	}*/

	// Upload to Buffer:

	stagingBuffer.map(uploadSize, 0);
	stagingBuffer.copyTo(pixels, uploadSize);
	stagingBuffer.flush(uploadSize, 0);
	stagingBuffer.unmap();

	/*
	{
		char* map = NULL;
		VK_CHECK_RESULT(vkMapMemory(device->device, bd->UploadBufferMemory, 0, upload_size, 0, (void**)(&map)));
		memcpy(map, pixels, upload_size);
		VkMappedMemoryRange range[1] = {};
		range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = bd->UploadBufferMemory;
		range[0].size = upload_size;
		VK_CHECK_RESULT(vkFlushMappedMemoryRanges(device->device, 1, range));
		vkUnmapMemory(device->device, bd->UploadBufferMemory);
	}
	*/

	// 创建cmd
	auto cmdBuffer = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// Copy to Image:
	{
		VkImageMemoryBarrier copy_barrier[1] = {};
		copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copy_barrier[0].image = image;
		copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy_barrier[0].subresourceRange.levelCount = 1;
		copy_barrier[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);

		VkBufferImageCopy region = {};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = width;
		region.imageExtent.height = height;
		region.imageExtent.depth = 1;
		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		VkImageMemoryBarrier use_barrier[1] = {};
		use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		use_barrier[0].image = image;
		use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		use_barrier[0].subresourceRange.levelCount = 1;
		use_barrier[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
	}

	device->flushCommandBuffer(cmdBuffer, device->graphicsQueue, true);

	// Store our identifier
	//io.Fonts->SetTexID((ImTextureID)bd->FontDescriptorSet);

	stagingBuffer.destroy();

	std::cout << "BuildImage descriptorSet = " << descriptorSet << std::endl;

	return 0;
}