#include "RenderingServer.hpp"
#include "RenderingResources.hpp"
#include "EnumsFromStrings.hpp"
#include "vulkan_utils.hpp"

using namespace Rendering;

//todo for now have decided to abandon this automatic format selection
//struct FormatChooseInfo {
//	//these booleans are used to find a suitable VkFormat for the image as well as image usage flags
//	//one and only one of these needs to be true
//	bool isColor = false;
//	bool isDepth = false;
//
//	//for now always assume as both transfer src or dst
//	//bool isTransferSrc = true;
//	//bool isTransferDst = true;
//	bool isAttachment = false;
//	bool isSampled = false;
//	bool isStorage = false;
//
//	//these are required if true, otherwise they may or may not be there
//	bool isPacked = false;
//	bool isSigned = false;
//	bool isUnsigned = false;
//
//	//only one of these should be true
//	bool isNorm = false;
//	bool isSRGB = false;
//	bool isInt = false;
//	bool isScaled = false;
//	bool isFloat = false;
//};
//VkFormat findSuitableImageFormat(FormatChooseInfo info) {
//	assert(info.isColor != info.isDepth);
//	assert( (info.isScaled ^ info.isInt ^ info.isSRGB ^ info.isNorm ^ info.isFloat) == 1);
//
//	//todo incorporate bit width and num components
//	//maybe use lookup table for less messy code
//	if (info.isColor) {
//		if (info.isScaled) {
//			if (info.isPacked) {
//				if (info.isSigned) {
//					//scaled, packed and signed
//
//				}
//				else if (info.isUnsigned) {
//					//scaled, packed and unsigned
//
//				}
//				else {
//					//scaled, packed and don't care
//
//				}
//			}
//			else if (info.isSigned) {
//				//scaled, signed
//
//			}
//			else if (info.isUnsigned) {
//				//scaled, unsigned
//
//			}
//			else {
//				//scaled, do not care about sign or pack
//
//			}
//		}
//		else if (info.isInt) {
//			if (info.isPacked) {
//				if (info.isSigned) {
//					//int, packed and signed
//
//				}
//				else if (info.isUnsigned) {
//					//int, packed and unsigned
//
//				}
//				else {
//					//int, packed and don't care
//
//				}
//			}
//			else if (info.isSigned) {
//				//int, signed
//
//			}
//			else if (info.isUnsigned) {
//				//int, unsigned
//
//			}
//			else {
//				//int, do not care about sign or pack
//
//			}
//		}
//		else if (info.isSRGB) {
//			if (info.isPacked) {
//				if (info.isSigned) {
//					//srgb, packed and signed
//
//				}
//				else if (info.isUnsigned) {
//					//srgb, packed and unsigned
//
//				}
//				else {
//					//srgb, packed and don't care
//
//				}
//			}
//			else if (info.isSigned) {
//				//srgb, signed
//
//			}
//			else if (info.isUnsigned) {
//				//srgb, unsigned
//
//			}
//			else {
//				//srgb, do not care about sign or pack
//
//			}
//		}
//		else if (info.isNorm) {
//			if (info.isPacked) {
//				if (info.isSigned) {
//					//norm, packed and signed
//
//				}
//				else if (info.isUnsigned) {
//					//norm, packed and unsigned
//
//				}
//				else {
//					//norm, packed and don't care
//
//				}
//			}
//			else if (info.isSigned) {
//				//norm, signed
//
//			}
//			else if (info.isUnsigned) {
//				//norm, unsigned
//
//			}
//			else {
//				//norm, do not care about sign or pack
//
//			}
//		}
//		else if (info.isFloat) {
//			if (info.isPacked) {
//				if (info.isSigned) {
//					//float, packed and signed
//
//				}
//				else if (info.isUnsigned) {
//					//float, packed and unsigned
//
//				}
//				else {
//					//float, packed and don't care
//
//				}
//			}
//			else if (info.isSigned) {
//				//float, signed
//
//			}
//			else if (info.isUnsigned) {
//				//float, unsigned
//
//			}
//			else {
//				//float, do not care about sign or pack
//
//			}
//		}
//	}
//
//}

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
	info.usage = usageFlags;
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

	return this->resources.add(img);
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
	Image img = this->resources.get<Image>(image);
	VkImageAspectFlags aspect = imageAspectFlagsFromString(aspectIN);

	__cmdTransitionImageLayout__(
		this->activeCBuf,
		img.image,
		aspect,
		imageLayoutFromString(oldLayout),
		imageLayoutFromString(newLayout)
	);
}

void RenderingServer::transitionImageLayout(
	Rendering::ResourceID image,
	std::string aspectIN,
	std::string oldLayout,
	std::string newLayout
) {
	Image img = this->resources.get<Image>(image);
	VkImageAspectFlags aspect = imageAspectFlagsFromString(aspectIN);

	VkCommandBuffer commandBuffer = core->beginSingleTimeCommands(commandPool);

	__cmdTransitionImageLayout__(
		commandBuffer,
		img.image,
		aspect,
		imageLayoutFromString(oldLayout),
		imageLayoutFromString(newLayout)
	);

	core->endSingleTimeCommands(commandPool, commandBuffer);
}

void RenderingServer::destroyImage(Rendering::ResourceID image) {
	Image img = this->resources.get<Image>(image);
	vmaDestroyImage(core->allocator, img.image, img.allocation);
	this->resources.remove<Image>(image);
}

Rendering::ResourceID RenderingServer::createImageView(
	Rendering::ResourceID imageID,
	Rendering::ImageViewCreateInfo createInfo
) {
	Rendering::Image image = this->resources.get<Rendering::Image>(imageID);
	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext = nullptr;

	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.image = image.image;
	//todo see if you wanna add mutable formats
	info.format = image.format;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = createInfo.levelCount;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = createInfo.arrayLayerCount;
	info.subresourceRange.aspectMask = imageAspectFlagsFromString(createInfo.aspectMask);

	Rendering::ImageView view{};
	view.info = info;

	if (
		vkCreateImageView(
			core->device,
			&view.info,
			nullptr,
			&view.view
		) != VK_SUCCESS
	) {
		throw std::runtime_error("Could not create image view!");
	}

	return this->resources.add(view);
}

void RenderingServer::destroyImageView(Rendering::ResourceID imageView) {
	vkDestroyImageView(
		VulkanUtils::utils().getCore()->device,
		this->resources.get<Rendering::ImageView>(imageView).view,
		nullptr
	);
	this->resources.remove<Rendering::ImageView>(imageView);
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

	return this->resources.add(buffer);
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
	VkBuffer src = this->resources.get<Buffer>(copyInf.srcBuffer).buffer;
	VkBuffer dst = this->resources.get<Buffer>(copyInf.dstBuffer).buffer;
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

	VkBuffer src = this->resources.get<Buffer>(copyInf.srcBuffer).buffer;
	VkBuffer dst = this->resources.get<Buffer>(copyInf.dstBuffer).buffer;
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
	imageSubresource.aspectMask = imageAspectFlagsFromString(copyInf.imageAspect);
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
	Buffer buffer = this->resources.get<Buffer>(bufferID);
	Image image = this->resources.get<Image>(imageID);
	__cmdCopyBufferIntoImage__(this->activeCBuf, buffer, image, copyInfo);
}

void RenderingServer::copyBufferIntoImage(
	Rendering::ResourceID bufferID,
	Rendering::ResourceID imageID,
	Rendering::BufferToImageCopy copyInfo
)
{
	VkCommandBuffer commandBuffer = core->beginSingleTimeCommands(commandPool);

	Buffer buffer = this->resources.get<Buffer>(bufferID);
	Image image = this->resources.get<Image>(imageID);
	__cmdCopyBufferIntoImage__(commandBuffer, buffer, image, copyInfo);

	core->endSingleTimeCommands(commandPool, commandBuffer);
}

void RenderingServer::destroyBuffer(Rendering::ResourceID buffer) {
	Buffer buf = this->resources.get<Buffer>(buffer);
	vmaDestroyBuffer(core->allocator, buf.buffer, buf.allocation);
	this->resources.remove<Buffer>(buffer);
}

Rendering::ResourceID RenderingServer::createSampler(Rendering::SamplerCreateInfo info) {
	Rendering::Sampler sampler{};

	VkSamplerCreateInfo samplerInfo = info.getCreateInfo();
	if (vkCreateSampler(core->device, &samplerInfo, nullptr, &sampler.sampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Sampler!");
	}
	
	return this->resources.add(sampler);
}

void RenderingServer::destroySampler(Rendering::ResourceID sampler) {
	this->resources.remove<Rendering::Sampler>(sampler);
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
	return this->resources.add(cbuf);
}

void RenderingServer::beginCommandBuffer(ResourceID cbuf, bool isOneTimeSubmit) {
	assert(this->activeCBufResourceID == 0);
	this->activeCBufResourceID = cbuf;
	this->activeCBuf = this->resources.get<CommandBuffer>(cbuf).buffer;

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
	this->resources.remove<CommandBuffer>(commandBuffer);
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
	return this->resources.add(shaderModule);
}

void RenderingServer::destroyShaderModule(ResourceID moduleID) {
	VkShaderModule module = this->resources.get<ShaderModule>(moduleID).shaderModule;
	vkDestroyShaderModule(core->device, module, nullptr);
	this->resources.remove<ShaderModule>(moduleID);
}

Rendering::ResourceID RenderingServer::createDescriptorPool(Rendering::DescriptorPoolCreateInfo poolInfo) {
	VkDescriptorPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	info.maxSets = poolInfo.maxSets;
	info.poolSizeCount = 4;
	VkDescriptorPoolSize sizes[] =
	{
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, poolInfo.uniformBufferCount},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, poolInfo.storageBufferCount},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolInfo.combinedImageSamplerCount},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, poolInfo.uniformBufferDynamicCount}
	};
	info.pPoolSizes = sizes;

	Rendering::DescriptorPool descriptorPool{};
	if (vkCreateDescriptorPool(core->device, &info, nullptr, &descriptorPool.pool) != VK_SUCCESS) {
		throw std::runtime_error("Error creating descriptor pool");
	};

	return this->resources.add(descriptorPool);
}

Rendering::ResourceID RenderingServer::createDescriptorSet(Rendering::DescriptorSetCreateInfo info) {
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.flags = 0;	//todo add layout create flags (like update after bind (imp))
	layoutInfo.bindingCount = info.bindings.size();
	layoutInfo.pBindings = info.bindings.data();
	
	assert(info.bindings.size() == info.bindingFlags.size() && "improper binding indices!");
	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
	bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlagsInfo.bindingCount = info.bindings.size();
	bindingFlagsInfo.pBindingFlags = info.bindingFlags.data();
	bindingFlagsInfo.pNext = nullptr;

	layoutInfo.pNext = &bindingFlagsInfo;

	VkDescriptorSetLayout layout;
	if (vkCreateDescriptorSetLayout(core->device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
		throw std::runtime_error("Error creating descriptor set layout");
	};

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = this->resources.get<DescriptorPool>(info.descriptorPool).pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSet dSet{};
	if (vkAllocateDescriptorSets(core->device, &allocInfo, &dSet) != VK_SUCCESS) {
		throw std::runtime_error("Error creating descriptor set");
	};

	Rendering::DescriptorSet descriptorSet{};
	descriptorSet.layout = layout;
	descriptorSet.set = dSet;
	return this->resources.add(descriptorSet);
}

void __writeResourceToDescriptorSet__(
	VkDevice device,
	VkDescriptorSet dstSet,
	uint32_t dstBinding,
	uint32_t dstArrayElement,
	VkDescriptorType descriptorType,
	VkDescriptorBufferInfo* bufferInfo,
	VkDescriptorImageInfo* imageInfo
) {
	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorCount = 1;

	write.descriptorType = descriptorType;

	write.dstSet = dstSet;
	write.dstBinding = dstBinding;
	write.dstArrayElement = dstArrayElement;

	write.pBufferInfo = bufferInfo;
	write.pImageInfo = imageInfo;

	write.pNext = nullptr;


	vkUpdateDescriptorSets(
		device,
		1,
		&write,
		0,
		nullptr
	);
}

void RenderingServer::writeBufferToDescriptorSet(
	Rendering::ResourceID descriptorSet,
	uint32_t dstBinding,
	uint32_t dstArrayElement,
	Rendering::ResourceID buffer,
	uint32_t offset,
	uint32_t range,
	bool isUniform,
	bool isDynamic
) {
	VkDescriptorSet dstSet = this->resources.get<Rendering::DescriptorSet>(descriptorSet).set;

	VkDescriptorType descriptorType{};

	if (isUniform) {
		descriptorType = isDynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}
	else {
		descriptorType = isDynamic ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	}

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = this->resources.get<Rendering::Buffer>(buffer).buffer;
	bufferInfo.offset = offset;
	bufferInfo.range = range;

	__writeResourceToDescriptorSet__(
		core->device,
		dstSet,
		dstBinding,
		dstArrayElement,
		descriptorType,
		&bufferInfo,
		nullptr
	);
}

void RenderingServer::writeImageToDescriptorSet(
	Rendering::ResourceID descriptorSet,
	uint32_t dstBinding,
	uint32_t dstArrayElement,
	Rendering::ResourceID imageView,
	std::string imageLayout,
	bool isSampledNotStorage
) {
	VkDescriptorSet dstSet = this->resources.get<Rendering::DescriptorSet>(descriptorSet).set;
	VkDescriptorType descriptorType = isSampledNotStorage ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageView = this->resources.get<Rendering::ImageView>(imageView).view;
	imgInfo.imageLayout = imageLayoutFromString(imageLayout);
	imgInfo.sampler = VK_NULL_HANDLE;

	__writeResourceToDescriptorSet__(
		core->device,
		dstSet,
		dstBinding,
		dstArrayElement,
		descriptorType,
		nullptr,
		&imgInfo
	);
}

void RenderingServer::writeSamplerToDescriptorSet(
	Rendering::ResourceID descriptorSet,
	uint32_t dstBinding,
	uint32_t dstArrayElement,
	Rendering::ResourceID sampler
) {
	VkDescriptorSet dstSet = this->resources.get<Rendering::DescriptorSet>(descriptorSet).set;
	VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

	VkDescriptorImageInfo imgInfo{};
	imgInfo.sampler = this->resources.get<Rendering::Sampler>(sampler).sampler;

	__writeResourceToDescriptorSet__(
		core->device,
		dstSet,
		dstBinding,
		dstArrayElement,
		descriptorType,
		nullptr,
		&imgInfo
	);
}

void RenderingServer::writeCombinedImageSamplerToDescriptorSet(
	Rendering::ResourceID descriptorSet,
	uint32_t dstBinding,
	uint32_t dstArrayElement,
	Rendering::ResourceID imageView,
	Rendering::ResourceID sampler,
	std::string imageLayout
) {
	VkDescriptorSet dstSet = this->resources.get<Rendering::DescriptorSet>(descriptorSet).set;
	VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageView = this->resources.get<Rendering::ImageView>(imageView).view;
	imgInfo.imageLayout = imageLayoutFromString(imageLayout);
	imgInfo.sampler = this->resources.get<Rendering::Sampler>(sampler).sampler;

	__writeResourceToDescriptorSet__(
		core->device,
		dstSet,
		dstBinding,
		dstArrayElement,
		descriptorType,
		nullptr,
		&imgInfo
	);
}


ResourceID RenderingServer::createPipelineLayout(PipelineLayoutCreateInfo info){
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(info.setsForSetLayouts.size());
	for (ResourceID id : info.setsForSetLayouts) {
		layouts.push_back(this->resources.get<DescriptorSet>(id).layout);
	}

	pipelineLayoutInfo.setLayoutCount = layouts.size();
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	
	pipelineLayoutInfo.pushConstantRangeCount = info.pushConstants.size();
	pipelineLayoutInfo.pPushConstantRanges = info.pushConstants.data();

	Rendering::PipelineLayout pipelineLayout{};
	pipelineLayout.layout = core->createPipelineLayout(pipelineLayoutInfo);

	return this->resources.add(pipelineLayout);
}

ResourceID RenderingServer::createPipeline(PipelineCreateInfo info) {
	//pipeline creation
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = this->resources.get<ShaderModule>(info.vertexShaderModule).shaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = this->resources.get<ShaderModule>(info.fragmentShaderModule).shaderModule;
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

	pipelineInfo.layout = this->resources.get<PipelineLayout>(info.pipelineLayout).layout;

	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	Rendering::Pipeline pipeline{};
	pipeline.layout = pipelineInfo.layout;
	pipeline.layoutID = info.pipelineLayout;
	pipeline.pipeline = core->createGraphicsPipeline(pipelineInfo);

	return this->resources.add(pipeline);
}

void RenderingServer::cmdUsePipeline(ResourceID pipeline) {
	vkCmdBindPipeline(activeCBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, this->resources.get<Pipeline>(pipeline).pipeline);
}

void RenderingServer::destroyPipeline(ResourceID pipeline) {
	Pipeline pipe = this->resources.get<Pipeline>(pipeline);
	vkDestroyPipeline(core->device, pipe.pipeline, nullptr);
}

void RenderingServer::beginRendering(Rendering::RenderingInfo info) {
	VkRenderingInfo renderInfo{};
	renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderInfo.pDepthAttachment = nullptr;
	renderInfo.pStencilAttachment = nullptr;
	renderInfo.colorAttachmentCount = 0;
	renderInfo.pColorAttachments = nullptr;

	//todo see if you want to add multiview functionality later
	renderInfo.layerCount = 1;
	renderInfo.viewMask = 0;
	renderInfo.renderArea = info.renderArea;

	renderInfo.pNext = nullptr;

	auto depthImageView = this->resources.getIfExists<Rendering::ImageView>(info.depthImageView);
	if (depthImageView.has_value()) {
		info.depthAttachment.imageView = depthImageView->view;
		renderInfo.pDepthAttachment = &info.depthAttachment;
	}

	auto stencilImageView = this->resources.getIfExists<Rendering::ImageView>(info.stencilImageView);
	if (stencilImageView.has_value()) {
		info.stencilAttachment.imageView = stencilImageView->view;
		renderInfo.pStencilAttachment = &info.stencilAttachment;
	}

	assert(info.colorAttachmentImageViews.size() == info.colorAttachments.size());
	for (int i = 0; i < info.colorAttachments.size(); i++) {
		auto colorImageView = this->resources.getIfExists<Rendering::ImageView>(
			info.colorAttachmentImageViews[i]
		);
		if (colorImageView.has_value()) {
			info.colorAttachments[i].imageView = colorImageView->view;
		}
		else {
			info.colorAttachments[i].imageView = VK_NULL_HANDLE;
		}
	}
	renderInfo.colorAttachmentCount = info.colorAttachments.size();
	renderInfo.pColorAttachments = info.colorAttachments.data();

	vkCmdBeginRendering(activeCBuf, &renderInfo);
}

void RenderingServer::endRendering() {
	vkCmdEndRendering(this->activeCBuf);
}

void RenderingServer::setActiveViewport(uint32_t index, Rendering::ViewportInfo info) {
	VkViewport viewport = info.getAsVk();
	vkCmdSetViewport(activeCBuf, index, 1, &viewport);
}

void RenderingServer::setActiveScissor(uint32_t index, Rendering::Rect2D info) {
	VkRect2D scissor = info.getAsVk();
	vkCmdSetScissor(activeCBuf, index, 1, &scissor);
}

Rendering::ResourceID RenderingServer::createFence(bool createSignalled) {
	VkFenceCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.flags = createSignalled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
	info.pNext = nullptr;
	Fence fence{};
	fence.fence = core->createFence(info);
	return this->resources.add(fence);
}

void RenderingServer::destroyFence(Rendering::ResourceID fence) {
	vkDestroyFence(core->device, this->resources.get<Fence>(fence).fence, nullptr);
	this->resources.remove<Fence>(fence);
}

Rendering::ResourceID RenderingServer::createSemaphore() {
	VkSemaphoreCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	Semaphore semaphore{};
	semaphore.semaphore = core->createSemaphore(info);
	return this->resources.add(semaphore);
}

void RenderingServer::destroySemaphore(Rendering::ResourceID semaphore) {
	vkDestroySemaphore(core->device, this->resources.get<Semaphore>(semaphore).semaphore, nullptr);
	this->resources.remove<Semaphore>(semaphore);
}

void RenderingServer::submitCommandBuffer(
	ResourceID commandBuffer,
	CommandBufferSubmitInfo submitInfo
) {
	VkCommandBuffer cbuf = this->resources.get<CommandBuffer>(commandBuffer).buffer;
	VkFence fence = this->resources.get<Fence>(submitInfo.fence).fence;
	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;
	
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cbuf;
	
	std::vector<VkSemaphore> signalSemaphores;
	for (Semaphore s : this->resources.getResourceStore<Semaphore>()->getMany(
		submitInfo.signalSemaphores
	)) {
		signalSemaphores.push_back(s.semaphore);
	}

	submit.signalSemaphoreCount = signalSemaphores.size();
	submit.pSignalSemaphores = signalSemaphores.data();

	std::vector<VkSemaphore> waitSemaphores;
	for (Semaphore s : this->resources.getResourceStore<Semaphore>()->getMany(
		submitInfo.waitSemaphores
	)) {
		waitSemaphores.push_back(s.semaphore);
	}

	submit.waitSemaphoreCount = waitSemaphores.size();
	submit.pWaitSemaphores = waitSemaphores.data();
	submit.pWaitDstStageMask = submitInfo.waitStages.data();

	if (vkQueueSubmit(core->graphicsQueue, 1, &submit, fence) != VK_SUCCESS) {
		//error
		//todo	add util function to print resource username and id seperately
		throw std::runtime_error(
			"error while submitting command buffer: " +
			std::to_string(commandBuffer)
		);
	}
}


void bindRenderingCreateInfoToLua(sol::state& luaState);
void bindRenderingServerToLua(sol::table& rendering, RenderingServer* server);

void RenderingServer::registerRenderingBindings()
{
	lua.state.open_libraries(sol::lib::base);
	lua.rendering = lua.state.create_named_table("rs");
	bindRenderingCreateInfoToLua(lua.state);
	bindRenderingServerToLua(lua.rendering, this);
}

void bindRenderingCreateInfoToLua(sol::state& luaState) {
	luaState.new_usertype<ImageCreateInfo>(
		"ImageCreateInfo",
		sol::constructors<ImageCreateInfo()>(),
		"format", &ImageCreateInfo::format,
		"isColorAttachment", &ImageCreateInfo::isColorAttachment,
		"isDepthAttachment", &ImageCreateInfo::isDepthAttachment,
		"isSampled", &ImageCreateInfo::isSampled,
		"isStorage", &ImageCreateInfo::isStorage,
		"numDimensions", &ImageCreateInfo::numDimensions,
		"width", &ImageCreateInfo::width,
		"height", &ImageCreateInfo::height,
		"depth", &ImageCreateInfo::depth,
		"mipLevels", &ImageCreateInfo::mipLevels,
		"arrayLayers", &ImageCreateInfo::arrayLayers
	);
	luaState.new_usertype<ImageViewCreateInfo>(
		"ImageViewCreateInfo",
		sol::constructors<ImageViewCreateInfo()>(),
		"aspectMask", &ImageViewCreateInfo::aspectMask,
		"levelCount", &ImageViewCreateInfo::levelCount,
		"arrayLayerCount", &ImageViewCreateInfo::arrayLayerCount
	);
	luaState.new_usertype<BufferCreateInfo>(
		"BufferCreateInfo",
		sol::constructors<BufferCreateInfo()>(),
		"size", &BufferCreateInfo::size,
		"createDedicatedMemory", &BufferCreateInfo::createDedicatedMemory,
		"createMapped", &BufferCreateInfo::createMapped
	);
	luaState.new_usertype<BufferCopyInfo>(
		"BufferCopyInfo",
		sol::constructors<BufferCopyInfo()>(),
		"srcBuffer", &BufferCopyInfo::srcBuffer,
		"dstBuffer", &BufferCopyInfo::dstBuffer,
		"srcOffset", &BufferCopyInfo::srcOffset,
		"dstOffset", &BufferCopyInfo::dstOffset,
		"size", &BufferCopyInfo::size
	);
	luaState.new_usertype<BufferToImageCopy>(
		"BufferToImageCopy",
		sol::constructors<BufferToImageCopy()>(),
		"bufferOffset", &BufferToImageCopy::bufferOffset,
		"bufferRowLength", &BufferToImageCopy::bufferRowLength,
		"bufferImageHeight", &BufferToImageCopy::bufferImageHeight,
		"imageAspect", &BufferToImageCopy::imageAspect,
		"mipLevel", &BufferToImageCopy::mipLevel,
		"baseArrayLayer", &BufferToImageCopy::baseArrayLayer,
		"layerCount", &BufferToImageCopy::layerCount,
		"imageOffsetX", &BufferToImageCopy::imageOffsetX,
		"imageOffsetY", &BufferToImageCopy::imageOffsetY,
		"imageOffsetZ", &BufferToImageCopy::imageOffsetZ,
		"imageWidth", &BufferToImageCopy::imageWidth,
		"imageHeight", &BufferToImageCopy::imageHeight,
		"imageDepth", &BufferToImageCopy::imageDepth
	);
	luaState.new_usertype<SamplerCreateInfo>(
		"SamplerCreateInfo",
		sol::constructors<SamplerCreateInfo()>(),
		"setMinFilter", &SamplerCreateInfo::setMinFilter,
		"setMagFilter", &SamplerCreateInfo::setMagFilter,
		"setAnisotropy", &SamplerCreateInfo::setAnisotropy
	);
	luaState.new_usertype<DescriptorPoolCreateInfo>(
		"DescriptorPoolCreateInfo",
		sol::constructors<DescriptorPoolCreateInfo()>(),
		"maxSets", &DescriptorPoolCreateInfo::maxSets,
		"uniformBufferCount", &DescriptorPoolCreateInfo::uniformBufferCount,
		"uniformBufferDynamicCount", &DescriptorPoolCreateInfo::uniformBufferDynamicCount,
		"storageBufferCount", &DescriptorPoolCreateInfo::storageBufferCount,
		"combinedImageSamplerCount", &DescriptorPoolCreateInfo::combinedImageSamplerCount
	);
	luaState.new_usertype<DescriptorSetCreateInfo>(
		"DescriptorSetCreateInfo",
		sol::constructors<DescriptorSetCreateInfo()>(),
		"descriptorPool", &DescriptorSetCreateInfo::descriptorPool,
		"addBinding", &DescriptorSetCreateInfo::addBinding
	);
	luaState.new_usertype<PipelineLayoutCreateInfo>(
		"PipelineLayoutCreateInfo",
		sol::constructors<PipelineLayoutCreateInfo()>(),
		"addPushConstant", &PipelineLayoutCreateInfo::addPushConstant,
		"addSetLayout", &PipelineLayoutCreateInfo::addSetLayout
	);
	luaState.new_usertype<PipelineCreateInfo>(
		"PipelineCreateInfo",
		sol::constructors<PipelineCreateInfo()>(),
		"vertexShaderModule", &PipelineCreateInfo::vertexShaderModule,
		"fragmentShaderModule", &PipelineCreateInfo::fragmentShaderModule,
		"pipelineLayout", &PipelineCreateInfo::pipelineLayout,
		"sampleShadingEnable", &PipelineCreateInfo::sampleShadingEnable,
		"minSampleShading", &PipelineCreateInfo::minSampleShading,
		"sampleCount", &PipelineCreateInfo::sampleCount,
		"depthTestEnable", &PipelineCreateInfo::depthTestEnable,
		"depthWriteEnable", &PipelineCreateInfo::depthWriteEnable,
		"depthBoundsTestEnable", &PipelineCreateInfo::depthBoundsTestEnable,
		"stencilTestEnable", &PipelineCreateInfo::stencilTestEnable,
		"depthCompareOp", &PipelineCreateInfo::depthCompareOp,
		"polygonMode", &PipelineCreateInfo::polygonMode,
		"depthAttachmentFormat", &PipelineCreateInfo::depthAttachmentFormat,
		"addColorAttachment", &PipelineCreateInfo::addColorAttachment,
		"addVertexBinding", &PipelineCreateInfo::addVertexBinding,
		"addVertexAttribute", &PipelineCreateInfo::addVertexAttribute
	);
	luaState.new_usertype<CommandBufferSubmitInfo>(
		"CommandBufferSubmitInfo",
		sol::constructors<CommandBufferSubmitInfo()>(),
		"fence", &CommandBufferSubmitInfo::fence,
		"addWaitSemaphore", &CommandBufferSubmitInfo::addWaitSemaphore,
		"addSignalSemaphore", &CommandBufferSubmitInfo::addSignalSemaphore
	);
	luaState.new_usertype<InstanceInfo>(
		"InstanceInfo",
		sol::constructors<InstanceInfo()>()

	);
	luaState.new_usertype<RenderedInstanceCreateInfo>(
		"RenderedInstanceCreateInfo",
		sol::constructors<RenderedInstanceCreateInfo()>(),
		"mesh", &RenderedInstanceCreateInfo::mesh,
		"material", &RenderedInstanceCreateInfo::material,
		"info", &RenderedInstanceCreateInfo::info
	);
	luaState.new_usertype<ViewportInfo>(
		"ViewportInfo",
		sol::constructors<ViewportInfo()>(),
		"width", &ViewportInfo::width,
		"height", &ViewportInfo::height,
		"offsetX", &ViewportInfo::offsetX,
		"offsetY", &ViewportInfo::offsetY,
		"maxDepth", &ViewportInfo::maxDepth,
		"minDepth", &ViewportInfo::minDepth
	);
	luaState.new_usertype<Rect2D>(
		"Rect2D",
		sol::constructors<Rect2D()>(),
		"width", &Rect2D::width,
		"height", &Rect2D::height,
		"offsetX", &Rect2D::offsetX,
		"offsetY", &Rect2D::offsetY
	);
	luaState.new_usertype<RenderingInfo>(
		"RenderingInfo",
		sol::constructors<RenderingInfo()>(),
		"setRenderArea", &RenderingInfo::setRenderArea,
		"setStencilAttachment", &RenderingInfo::setStencilAttachment,
		"setDepthAttachment", &RenderingInfo::setDepthAttachment,
		"addColorAttachment", &RenderingInfo::addColorAttachment
	);

}

void bindRenderingServerToLua(sol::table& rendering, RenderingServer* server) {
	rendering.set_function("createImage", &RenderingServer::createImage, server);
	rendering.set_function("cmdTransitionImageLayout", &RenderingServer::cmdTransitionImageLayout, server);
	rendering.set_function("transitionImageLayout", &RenderingServer::transitionImageLayout, server);
	rendering.set_function("destroyImage", &RenderingServer::destroyImage, server);
	rendering.set_function("createImageView", &RenderingServer::createImageView, server);
	rendering.set_function("destroyImageView", &RenderingServer::destroyImageView, server);
	rendering.set_function("createBuffer", &RenderingServer::createBuffer, server);
	rendering.set_function("cmdCopyBuffer", &RenderingServer::cmdCopyBuffer, server);
	rendering.set_function("copyBuffer", &RenderingServer::copyBuffer, server);
	rendering.set_function("cmdCopyBufferIntoImage", &RenderingServer::cmdCopyBufferIntoImage, server);
	rendering.set_function("copyBufferIntoImage", &RenderingServer::copyBufferIntoImage, server);
	rendering.set_function("destroyBuffer", &RenderingServer::destroyBuffer, server);
	rendering.set_function("createSampler", &RenderingServer::createSampler, server);
	rendering.set_function("destroySampler", &RenderingServer::destroySampler, server);
	rendering.set_function("createShaderModule", &RenderingServer::createShaderModule, server);
	rendering.set_function("destroyShaderModule", &RenderingServer::destroyShaderModule, server);
	rendering.set_function("createDescriptorPool", &RenderingServer::createDescriptorPool, server);
	rendering.set_function("createDescriptorSet", &RenderingServer::createDescriptorSet, server);
	rendering.set_function("writeBufferToDescriptorSet", &RenderingServer::writeBufferToDescriptorSet, server);
	rendering.set_function("writeImageToDescriptorSet", &RenderingServer::writeImageToDescriptorSet, server);
	rendering.set_function("writeSamplerToDescriptorSet", &RenderingServer::writeSamplerToDescriptorSet, server);
	rendering.set_function("writeCombinedImageSamplerToDescriptorSet", &RenderingServer::writeCombinedImageSamplerToDescriptorSet, server);
	rendering.set_function("createPipelineLayout", &RenderingServer::createPipelineLayout, server);
	rendering.set_function("createPipeline", &RenderingServer::createPipeline, server);
	rendering.set_function("cmdUsePipeline", &RenderingServer::cmdUsePipeline, server);
	rendering.set_function("destroyPipeline", &RenderingServer::destroyPipeline, server);
	rendering.set_function("beginRendering", &RenderingServer::beginRendering, server);
	rendering.set_function("endRendering", &RenderingServer::endRendering, server);
	rendering.set_function("setActiveViewport", &RenderingServer::setActiveViewport, server);
	rendering.set_function("setActiveScissor", &RenderingServer::setActiveScissor, server);
	rendering.set_function("createCommandBuffer", &RenderingServer::createCommandBuffer, server);
	rendering.set_function("beginCommandBuffer", &RenderingServer::beginCommandBuffer, server);
	rendering.set_function("endCommandBuffer", &RenderingServer::endCommandBuffer, server);
	rendering.set_function("destroyCommandBuffer", &RenderingServer::destroyCommandBuffer, server);
	rendering.set_function("createFence", &RenderingServer::createFence, server);
	rendering.set_function("destroyFence", &RenderingServer::destroyFence, server);
	rendering.set_function("createSemaphore", &RenderingServer::createSemaphore, server);
	rendering.set_function("destroySemaphore", &RenderingServer::destroySemaphore, server);
	rendering.set_function("submitCommandBuffer", &RenderingServer::submitCommandBuffer, server);
}