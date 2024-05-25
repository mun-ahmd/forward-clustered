#include "RenderingServer.hpp"
#include "RenderingResources.hpp"
#include "EnumsFromStrings.hpp"
#include "vulkan_utils.hpp"

using namespace Rendering;
//std::unordered_map <std::string, std::vector<VkFormat>> imageFormats = {
//	{
//		"r8g8b8a8",
//		{
//			VK_FORMAT_R8G8B8A8_SINT,
//			VK_FORMAT_R8G8B8A8_SNORM,
//			VK_FORMAT_R8G8B8A8_SRGB,
//			VK_FORMAT_R8G8B8A8_SSCALED,
//			VK_FORMAT_R8G8B8A8_UINT,
//			VK_FORMAT_R8G8B8A8_UNORM,
//			VK_FORMAT_R8G8B8A8_USCALED
//			
//		}
//	},
//	{
//		"r"
//	},
//	{"r8g8b8uscale", {VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR}}
//};

//todo for now have decided to abandon this automatic format selection
struct FormatChooseInfo {
	//these booleans are used to find a suitable VkFormat for the image as well as image usage flags
	//one and only one of these needs to be true
	bool isColor = false;
	bool isDepth = false;

	//for now always assume as both transfer src or dst
	//bool isTransferSrc = true;
	//bool isTransferDst = true;
	bool isAttachment = false;
	bool isSampled = false;
	bool isStorage = false;

	//these are required if true, otherwise they may or may not be there
	bool isPacked = false;
	bool isSigned = false;
	bool isUnsigned = false;

	//only one of these should be true
	bool isNorm = false;
	bool isSRGB = false;
	bool isInt = false;
	bool isScaled = false;
	bool isFloat = false;
};
VkFormat findSuitableImageFormat(FormatChooseInfo info) {
	assert(info.isColor != info.isDepth);
	assert( (info.isScaled ^ info.isInt ^ info.isSRGB ^ info.isNorm ^ info.isFloat) == 1);

	//todo incorporate bit width and num components
	//maybe use lookup table for less messy code
	if (info.isColor) {
		if (info.isScaled) {
			if (info.isPacked) {
				if (info.isSigned) {
					//scaled, packed and signed

				}
				else if (info.isUnsigned) {
					//scaled, packed and unsigned

				}
				else {
					//scaled, packed and don't care

				}
			}
			else if (info.isSigned) {
				//scaled, signed

			}
			else if (info.isUnsigned) {
				//scaled, unsigned

			}
			else {
				//scaled, do not care about sign or pack

			}
		}
		else if (info.isInt) {
			if (info.isPacked) {
				if (info.isSigned) {
					//int, packed and signed

				}
				else if (info.isUnsigned) {
					//int, packed and unsigned

				}
				else {
					//int, packed and don't care

				}
			}
			else if (info.isSigned) {
				//int, signed

			}
			else if (info.isUnsigned) {
				//int, unsigned

			}
			else {
				//int, do not care about sign or pack

			}
		}
		else if (info.isSRGB) {
			if (info.isPacked) {
				if (info.isSigned) {
					//srgb, packed and signed

				}
				else if (info.isUnsigned) {
					//srgb, packed and unsigned

				}
				else {
					//srgb, packed and don't care

				}
			}
			else if (info.isSigned) {
				//srgb, signed

			}
			else if (info.isUnsigned) {
				//srgb, unsigned

			}
			else {
				//srgb, do not care about sign or pack

			}
		}
		else if (info.isNorm) {
			if (info.isPacked) {
				if (info.isSigned) {
					//norm, packed and signed

				}
				else if (info.isUnsigned) {
					//norm, packed and unsigned

				}
				else {
					//norm, packed and don't care

				}
			}
			else if (info.isSigned) {
				//norm, signed

			}
			else if (info.isUnsigned) {
				//norm, unsigned

			}
			else {
				//norm, do not care about sign or pack

			}
		}
		else if (info.isFloat) {
			if (info.isPacked) {
				if (info.isSigned) {
					//float, packed and signed

				}
				else if (info.isUnsigned) {
					//float, packed and unsigned

				}
				else {
					//float, packed and don't care

				}
			}
			else if (info.isSigned) {
				//float, signed

			}
			else if (info.isUnsigned) {
				//float, unsigned

			}
			else {
				//float, do not care about sign or pack

			}
		}
	}

}

ResourceID RenderingServer::createImage(ImageCreateInfo imageInfo) {
	VkFormat parsedFormat;
	bool isColorImage = (imageInfo.format != "depth");
	if (!isColorImage) {
		//depth image
		parsedFormat = core->findDepthFormat();
	}
	else {
		parsedFormat =  formatFromString(imageInfo.format);
	}

	VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (imageInfo.isColorAttachment)
		usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (imageInfo.isDepthAttachment)
		usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (imageInfo.isSampled)
		usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (imageInfo.isStorage)
		usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;

	VkImageCreateInfo info = { };
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = nullptr;

	if (imageInfo.numDimensions == 1)
		info.imageType = VK_IMAGE_TYPE_1D;
	if (imageInfo.numDimensions == 2)
		info.imageType = VK_IMAGE_TYPE_2D;
	if (imageInfo.numDimensions == 3)
		info.imageType = VK_IMAGE_TYPE_3D;

	info.format = parsedFormat;
	info.extent.width = imageInfo.width;
	info.extent.height = imageInfo.height;
	info.extent.depth = imageInfo.depth;

	info.mipLevels = imageInfo.mipLevels;
	info.arrayLayers = imageInfo.arrayLayers;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocationCreateInfo = {};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	//todo for now images are only dedicated memory
	allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocationCreateInfo.requiredFlags = 0;
	allocationCreateInfo.preferredFlags = 0;

	Image img{};
	img.layout = info.initialLayout;
	img.format = info.format;
	img.imageType = info.imageType;
	img.extent = info.extent;
	img.mipLevels = info.mipLevels;
	img.arrayLayers = info.arrayLayers;

	VmaAllocationInfo allocationInfo;
	if (
		vmaCreateImage(core->allocator, &info, &allocationCreateInfo, &img.image, &img.allocation, &allocationInfo)
		!= VK_SUCCESS
		) {
		throw std::runtime_error("Could not create image resource!");
	}

	return this->images.add(img);
}

void __cmdTransitionImageLayout__(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags imageAspect, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = imageAspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;


	if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = 0;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void RenderingServer::cmdTransitionImageLayout(
	ResourceID image,
	std::string aspectIN,
	std::string oldLayout,
	std::string newLayout
) {
	Image img = this->images.get(image);
	VkImageAspectFlags aspect = imageAspectFromString(aspectIN);

	__cmdTransitionImageLayout__(
		this->activeCBuf,
		img.image,
		aspect,
		layoutFromString(oldLayout),
		layoutFromString(newLayout)
	);
}

void RenderingServer::transitionImageLayout(
	Rendering::ResourceID image,
	std::string aspectIN,
	std::string oldLayout,
	std::string newLayout
) {
	Image img = this->images.get(image);
	VkImageAspectFlags aspect = imageAspectFromString(aspectIN);

	VkCommandBuffer commandBuffer = core->beginSingleTimeCommands(commandPool);

	__cmdTransitionImageLayout__(
		commandBuffer,
		img.image,
		aspect,
		layoutFromString(oldLayout),
		layoutFromString(newLayout)
	);

	core->endSingleTimeCommands(commandPool, commandBuffer);
}

void RenderingServer::destroyImage(Rendering::ResourceID image) {
	Image img = this->images.get(image);
	vmaDestroyImage(core->allocator, img.image, img.allocation);
	this->images.remove(image);
}


ResourceID RenderingServer::createBuffer(BufferCreateInfo info) {
	Buffer buffer{};

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = info.size;

	//because on desktop gpus I have read usage flags do not matter much
	//setting all, if it causes issues, will add more settings in the create info
	//but overall will move away from enums in this api
	//leaving out VK_BUFFER_USAGE_INDEX_BUFFER_BIT and VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
	bufferInfo.usage =
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = 0;
	if (info.createDedicatedMemory)
		allocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	if (info.createMapped)
		allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	allocInfo.requiredFlags = 0;
	allocInfo.preferredFlags = 0;

	VmaAllocationInfo allocattedInfo{};
	if (vmaCreateBuffer(core->allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &allocattedInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	return this->buffers.add(buffer);
}

void __cmdCopyBuffer__(
	VkCommandBuffer commandBuffer,
	VkBuffer src,
	VkBuffer dst,
	VkDeviceSize srcOffset,
	VkDeviceSize dstOffset,
	VkDeviceSize copySize
) {
	VkBufferCopy bufferCopyInf{};
	bufferCopyInf.srcOffset = srcOffset;
	bufferCopyInf.dstOffset = dstOffset;
	bufferCopyInf.size = copySize;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &bufferCopyInf);
}

void RenderingServer::cmdCopyBuffer(
	BufferCopyInfo copyInf
) {
	VkBuffer src = this->buffers.get(copyInf.srcBuffer).buffer;
	VkBuffer dst = this->buffers.get(copyInf.dstBuffer).buffer;
	__cmdCopyBuffer__(
		this->activeCBuf,
		src,
		dst,
		copyInf.srcOffset,
		copyInf.dstOffset,
		copyInf.size
	);
}

void RenderingServer::copyBuffer(BufferCopyInfo copyInf) {
	VkCommandBuffer cBuf = core->beginSingleTimeCommands(commandPool);

	VkBuffer src = this->buffers.get(copyInf.srcBuffer).buffer;
	VkBuffer dst = this->buffers.get(copyInf.dstBuffer).buffer;
	__cmdCopyBuffer__(
		cBuf,
		src,
		dst,
		copyInf.srcOffset,
		copyInf.dstOffset,
		copyInf.size
	);

	core->endSingleTimeCommands(commandPool, cBuf);
}

void __cmdCopyBufferIntoImage__(
	VkCommandBuffer commandBuffer,
	Rendering::Buffer& buffer,
	Rendering::Image& image,
	Rendering::BufferToImageCopy& copyInf
) {
	VkImageSubresourceLayers imageSubresource{};
	imageSubresource.aspectMask = imageAspectFromString(copyInf.imageAspect);
	imageSubresource.mipLevel = copyInf.mipLevel;
	imageSubresource.baseArrayLayer = copyInf.baseArrayLayer;
	imageSubresource.layerCount = copyInf.layerCount;

	VkBufferImageCopy bufferToImageInfo{};
	bufferToImageInfo.bufferOffset = copyInf.bufferOffset;
	bufferToImageInfo.bufferRowLength = copyInf.bufferRowLength;
	bufferToImageInfo.bufferImageHeight = copyInf.bufferImageHeight;

	bufferToImageInfo.imageSubresource = imageSubresource;

	bufferToImageInfo.imageOffset = {
		copyInf.imageOffsetX,
		copyInf.imageOffsetY,
		copyInf.imageOffsetZ
	};

	bufferToImageInfo.imageExtent = {
		copyInf.imageWidth,
		copyInf.imageHeight,
		copyInf.imageDepth
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer.buffer,
		image.image,
		image.layout,
		1,
		&bufferToImageInfo
	);
}

void RenderingServer::cmdCopyBufferIntoImage(
	Rendering::ResourceID bufferID,
	Rendering::ResourceID imageID,
	Rendering::BufferToImageCopy copyInfo
)
{
	Buffer buffer = this->buffers.get(bufferID);
	Image image = this->images.get(imageID);
	__cmdCopyBufferIntoImage__(this->activeCBuf, buffer, image, copyInfo);
}

void RenderingServer::copyBufferIntoImage(
	Rendering::ResourceID bufferID,
	Rendering::ResourceID imageID,
	Rendering::BufferToImageCopy copyInfo
)
{
	VkCommandBuffer commandBuffer = core->beginSingleTimeCommands(commandPool);

	Buffer buffer = this->buffers.get(bufferID);
	Image image = this->images.get(imageID);
	__cmdCopyBufferIntoImage__(commandBuffer, buffer, image, copyInfo);

	core->endSingleTimeCommands(commandPool, commandBuffer);
}

void RenderingServer::destroyBuffer(Rendering::ResourceID buffer) {
	Buffer buf = this->buffers.get(buffer);
	vmaDestroyBuffer(core->allocator, buf.buffer, buf.allocation);
	this->buffers.remove(buffer);
}

ResourceID RenderingServer::createCommandBuffer() {
	VkCommandBufferAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext = nullptr;
	info.commandBufferCount = 1;
	info.commandPool = commandPool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	Rendering::CommandBuffer cbuf{};
	cbuf.buffer = core->createCommandBuffer(info);
	return this->commandBuffers.add(cbuf);
}

void RenderingServer::beginCommandBuffer(ResourceID cbuf, bool isOneTimeSubmit) {
	assert(this->activeCBufResourceID == 0);
	this->activeCBufResourceID = cbuf;
	this->activeCBuf = this->commandBuffers.get(cbuf).buffer;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = isOneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;

	vkBeginCommandBuffer(this->activeCBuf, &beginInfo);
}

void RenderingServer::endCommandBuffer() {
	assert(this->activeCBufResourceID != 0);
	this->activeCBufResourceID = 0;
	this->activeCBuf = VK_NULL_HANDLE;
}

void RenderingServer::destroyCommandBuffer(ResourceID commandBuffer) {
	//command buffers get destroyed along with command pool
	this->commandBuffers.remove(commandBuffer);
}

shaderc_shader_kind __shaderKindFromString__(std::string shaderType) {
	if (shaderType == "vertex") {
		return shaderc_shader_kind::shaderc_vertex_shader;
	}
	else if (shaderType == "fragment") {
		return shaderc_shader_kind::shaderc_fragment_shader;
	}
	else if (shaderType == "compute") {
		return shaderc_shader_kind::shaderc_compute_shader;
	}
	else if (shaderType == "geometry") {
		return shaderc_shader_kind::shaderc_geometry_shader;
	}
	else if (shaderType == "mesh") {
		return shaderc_shader_kind::shaderc_mesh_shader;
	}
	else {
		throw std::runtime_error("invalid shader kind string: " + shaderType);
	}
}

ResourceID RenderingServer::createShaderModule(std::string shaderFile, std::string shaderType) {
	//converts glsl to SPIR-V and then creates a shader module
	shaderc_shader_kind shaderc_shaderKind = __shaderKindFromString__(shaderType);

	auto shaderSPIRV = VulkanUtils::utils().compileGlslToSpv(
		shaderFile,
		shaderc_shaderKind
	);
	VkShaderModule shaderModule = VulkanUtils::utils().createShaderModule(
		shaderSPIRV
	);
	return this->shaderModules.add(shaderModule);
}

void RenderingServer::destroyShaderModule(ResourceID moduleID) {
	VkShaderModule module = this->shaderModules.get(moduleID);
	vkDestroyShaderModule(core->device, module, nullptr);
	this->shaderModules.remove(moduleID);
}

ResourceID RenderingServer::createPipelineLayout(PipelineLayoutCreateInfo info){
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(info.setsForSetLayouts.size());
	for (ResourceID id : info.setsForSetLayouts) {
		layouts.push_back(this->descriptorSets.get(id).layout);
	}

	pipelineLayoutInfo.setLayoutCount = layouts.size();
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	
	pipelineLayoutInfo.pushConstantRangeCount = info.pushConstants.size();
	pipelineLayoutInfo.pPushConstantRanges = info.pushConstants.data();

	VkPipelineLayout pipelineLayout = core->createPipelineLayout(pipelineLayoutInfo);

	return this->pipelineLayouts.add(pipelineLayout);
}

ResourceID RenderingServer::createPipeline(PipelineCreateInfo info) {
	//pipeline creation
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = this->shaderModules.get(info.vertexShaderModule);
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = this->shaderModules.get(info.fragmentShaderModule);
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = info.vertexBindings.size();
	vertexInputInfo.pVertexBindingDescriptions = info.vertexBindings.data();
	vertexInputInfo.vertexAttributeDescriptionCount = info.vertexAttributes.size();
	vertexInputInfo.pVertexAttributeDescriptions = info.vertexAttributes.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = polygonModeFromString(info.polygonMode);
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = info.sampleShadingEnable;
	multisampling.minSampleShading = info.sampleShadingEnable ? info.minSampleShading : 0.0;

	//sample count 1 => VK_SAMPLE_COUNT_1_BIT, 32 => VK_SAMPLE_COUNT_32_BIT
	multisampling.rasterizationSamples = static_cast<VkSampleCountFlagBits>(info.sampleCount);

	VkPipelineDepthStencilStateCreateInfo depthStencilCI{};
	depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCI.depthTestEnable = info.depthTestEnable;
	depthStencilCI.depthWriteEnable = info.depthWriteEnable;
	depthStencilCI.depthCompareOp = compareOpFromString(info.depthCompareOp);
	depthStencilCI.depthBoundsTestEnable = info.depthBoundsTestEnable;
	depthStencilCI.stencilTestEnable = info.stencilTestEnable;

	//todo maybe much later add dynamic states
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineRenderingCreateInfo renderingCreateInfo{};
	renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

	renderingCreateInfo.colorAttachmentCount = info.colorAttachmentFormats.size();
	renderingCreateInfo.pColorAttachmentFormats = info.colorAttachmentFormats.data();
	renderingCreateInfo.depthAttachmentFormat = formatFromString(info.depthAttachmentFormat);

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.pNext = &renderingCreateInfo;

	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	//todo for now only vertex + fragment graphics pipelines supported
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	//pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencilCI;
	pipelineInfo.pDynamicState = &dynamicState;

	pipelineInfo.layout = this->pipelineLayouts.get(info.pipelineLayout);

	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	Rendering::Pipeline pipeline{};
	pipeline.layout = pipelineInfo.layout;
	pipeline.layoutID = info.pipelineLayout;
	pipeline.pipeline = core->createGraphicsPipeline(pipelineInfo);

	return this->pipelines.add(pipeline);
}

void RenderingServer::cmdUsePipeline(ResourceID pipeline) {
	vkCmdBindPipeline(activeCBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelines.get(pipeline).pipeline);
}

void RenderingServer::destroyPipeline(ResourceID pipeline) {
	Pipeline pipe = this->pipelines.get(pipeline);
	vkDestroyPipeline(core->device, pipe.pipeline, nullptr);
}