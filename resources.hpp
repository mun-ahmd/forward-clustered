#pragma once
#include "core.hpp"
#include "MeshLoader.h"
#include <vector>
#include <array>
#include <string.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "VulkanStartingOut.hpp"

struct VertexInputDescription {
	//helper class to describe vertex input
	//limits usable vertex type to a single type for the application
	//eases creating pipelines
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex3);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex3, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex3, norm);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex3, uv);
		return attributeDescriptions;
	}
};

inline const std::vector<float> vertices = {
	// positions          // normals           // texture coords
	//front
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

	//back
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

	//left
	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	//right
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	 //top
	 -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
	  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
	  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	 -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
	 -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,

	 //bottom
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f
};

inline std::vector<Vertex3> getCubeVertices() {
	std::vector<Vertex3> verticesData;
	for (int i = 0; i < vertices.size(); i += 8) {
		Vertex3 vert{};
		vert.pos = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
		vert.norm = glm::vec3(vertices[i + 3], vertices[i + 4], vertices[i + 5]);
		vert.uv = glm::vec2(vertices[i + 6], vertices[i + 7]);
		verticesData.push_back(vert);
	}
	return verticesData;
}

struct BufferCopyInfo {
	VkBuffer src;
	VkBuffer dst;
	VkBufferCopy inf;

	BufferCopyInfo() = default;
	BufferCopyInfo(VkBuffer src, VkBuffer dst, VkBufferCopy inf) : src(src), dst(dst), inf(inf) {}
};

struct MeshPushConstants {
	glm::mat4 transform;
};

class Buffer {
public:
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocattedInfo;
	inline static int activeAllocatedCount = 0;

	static std::shared_ptr<Buffer> create(VulkanCore core,
		VkDeviceSize size,
		VkBufferUsageFlags usage, VmaAllocationCreateFlagBits vmaFlags,
		VkMemoryPropertyFlags requiredMemoryTypeFlags, VkMemoryPropertyFlags preferredMemoryTypeFlags = 0)
	{
		Buffer::activeAllocatedCount++;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = vmaFlags;
		allocInfo.requiredFlags = requiredMemoryTypeFlags;
		allocInfo.preferredFlags = preferredMemoryTypeFlags;

		Buffer buf;
		if (vmaCreateBuffer(core->allocator, &bufferInfo, &allocInfo, &buf.buffer, &buf.allocation, &buf.allocattedInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}
		return std::shared_ptr<Buffer>(new Buffer(buf),
			[core](Buffer* buf) {
				vmaDestroyBuffer(core->allocator, buf->buffer, buf->allocation);
				delete buf;
				Buffer::activeAllocatedCount--;
			}
		);
	}
};

inline void copyBuffer(VulkanCore core, VkCommandPool commandPool, std::vector<BufferCopyInfo> buffers) {
	VkCommandBuffer commandBuffer = core->beginSingleTimeCommands(commandPool);

	for (auto& copyInfo : buffers) {
		vkCmdCopyBuffer(commandBuffer, copyInfo.src, copyInfo.dst, 1, &copyInfo.inf);
	}

	core->endSingleTimeCommands(commandPool, commandBuffer);
}

inline auto prepareStagingBuffer(VulkanCore core, const void* data, size_t dataSize) {
	auto stagingBuffer = Buffer::create(
		core, dataSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);
	void* stagingBufferData;
	vmaMapMemory(core->allocator, stagingBuffer->allocation, &stagingBufferData);
	memcpy(stagingBufferData, data, dataSize);
	vmaUnmapMemory(core->allocator, stagingBuffer->allocation);
	return stagingBuffer;
}

class Image {
public:
	VkImage image;
	VkImageLayout layout;
	VkFormat format;
	VkExtent3D extent;
	VmaAllocation allocation;

	static VkImageCreateInfo makeCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
	{
		VkImageCreateInfo info = { };
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext = nullptr;

		info.imageType = VK_IMAGE_TYPE_2D;

		info.format = format;
		info.extent = extent;

		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = usageFlags;

		return info;
	}

	inline static int allocattedCount = 0;

	static std::shared_ptr<Image> create(
		VulkanCore core,
		VkImageCreateInfo imageCreateInfo, VmaAllocationCreateFlagBits vmaFlags,
		VkMemoryPropertyFlags requiredMemoryTypeFlags = 0, VkMemoryPropertyFlags preferredMemoryTypeFlags = 0)
	{
		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocationCreateInfo.flags = vmaFlags;
		allocationCreateInfo.requiredFlags = requiredMemoryTypeFlags;
		allocationCreateInfo.preferredFlags = preferredMemoryTypeFlags;

		Image img{};
		img.layout = imageCreateInfo.initialLayout;
		img.format = imageCreateInfo.format;
		img.extent = imageCreateInfo.extent;
		VmaAllocationInfo allocationInfo;
		if (
			vmaCreateImage(core->allocator, &imageCreateInfo, &allocationCreateInfo, &img.image, &img.allocation, &allocationInfo)
			!= VK_SUCCESS
		){
			throw std::runtime_error("Could not create image resource!");
		}

		allocattedCount++;
		return std::shared_ptr<Image>(new Image(img),
			[core](Image* img) {
				vmaDestroyImage(core->allocator, img->image, img->allocation);
				delete img;
				allocattedCount--;
			});
	}

	VkImageView getView(VulkanCore core, VkFormat viewFormat, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.pNext = nullptr;

		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.image = image;
		info.format = viewFormat;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;
		info.subresourceRange.aspectMask = aspectFlags;

		return getView(core, info);
	}

	VkImageView getView(VulkanCore core, VkImageViewCreateInfo info)
	{
		VkImageView view;
		if (vkCreateImageView(core->device, &info, nullptr, &view) != VK_SUCCESS) {
			throw std::runtime_error("Could not create image view!");
		}
		//have to destroy views yourself
		return view;
	}

	void transitionImageLayout(VkImageLayout newLayout) {
		VulkanCore core = VulkanUtils::utils().getCore();
		VkCommandBuffer commandBuffer = core->beginSingleTimeCommands(VulkanUtils::utils().getCommandPool());

		VkImageLayout oldLayout = layout;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
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

		core->endSingleTimeCommands(VulkanUtils::utils().getCommandPool(), commandBuffer);

		this->layout = newLayout;
	}

	void copyFromBuffer(RC<Buffer> buffer, VkBufferImageCopy region) {
		auto& vku = VulkanUtils::utils();
		assert((layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || layout == VK_IMAGE_LAYOUT_GENERAL), "");
		VkCommandBuffer commandBuffer = vku.getCore()->beginSingleTimeCommands(vku.getCommandPool());
		vkCmdCopyBufferToImage(commandBuffer, buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		vku.getCore()->endSingleTimeCommands(vku.getCommandPool(), commandBuffer);
	}

	
};

class Sampler {
public:
	VkSampler sampler;

	static VkSamplerCreateInfo makeCreateInfo(
		std::array<std::optional<VkFilter>, 2> minMagFilter = {},
		std::array<std::optional<VkSamplerAddressMode>, 3> uvwAddressMode = {},
		std::array<std::optional<float>, 3> mipLodBiasMinMax = {},
		std::optional<VkSamplerMipmapMode> mipMapMode = {},
		std::optional<float> maxAnisotropy = {},
		std::optional<VkCompareOp> compareOp = {},
		std::optional<VkBorderColor> borderColor = {},
		std::optional<VkSamplerCreateFlags> flags = {},
		VkBool32 unnormCoords = VK_FALSE
	) {
		VkSamplerCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		ci.minFilter = minMagFilter[0].value_or(VK_FILTER_NEAREST);
		ci.magFilter = minMagFilter[1].value_or(VK_FILTER_NEAREST);

		ci.mipmapMode = mipMapMode.value_or(VK_SAMPLER_MIPMAP_MODE_NEAREST);

		ci.mipLodBias = mipLodBiasMinMax[0].value_or(0.0f);
		ci.minLod = mipLodBiasMinMax[1].value_or(0.0f);
		ci.maxLod = mipLodBiasMinMax[2].value_or(0.0f);

		ci.addressModeU = uvwAddressMode[0].value_or(VK_SAMPLER_ADDRESS_MODE_REPEAT);
		ci.addressModeV = uvwAddressMode[1].value_or(VK_SAMPLER_ADDRESS_MODE_REPEAT);
		ci.addressModeW = uvwAddressMode[2].value_or(VK_SAMPLER_ADDRESS_MODE_REPEAT);

		ci.anisotropyEnable = maxAnisotropy.has_value();
		ci.maxAnisotropy = maxAnisotropy.value_or(0.0f);
		
		ci.compareEnable = compareOp.has_value();
		ci.compareOp = compareOp.value_or(VK_COMPARE_OP_ALWAYS);

		ci.unnormalizedCoordinates = unnormCoords;

		ci.borderColor = borderColor.value_or(VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK);

		ci.flags = flags.value_or((VkSamplerCreateFlags)0);

		ci.pNext = nullptr;

		return ci;
	}

	inline static int allocattedCount = 0;
	static std::shared_ptr<Sampler> create(
		VulkanCore core,
		VkSamplerCreateInfo info
	){
		Sampler sampler{};
		if (vkCreateSampler(core->device, &info, nullptr, &sampler.sampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Sampler!");
		}

		allocattedCount++;
		return std::shared_ptr<Sampler>(new Sampler(sampler),
			[core](Sampler* sampler) {
				vkDestroySampler(core->device, sampler->sampler, nullptr);
				delete sampler;
				allocattedCount--;
			});
	}
};

template<class ResourceInfo>
class DUResource {
	//DYNAMIC UNIFORM RESOURCE
private:
	uint32_t index;
public:
	ResourceInfo resourceInfo;

	inline static std::shared_ptr<Buffer> resourceBuf;
	inline static std::shared_ptr<Buffer> stagingBuf;
	inline static void* stagingBufMap;
	inline static uint32_t storedResources;
	inline static uint32_t paddedElementSize;
	inline static uint32_t maximumResources;

	static void clear() {
		storedResources = 0;
	}

	static void init(VulkanCore core, uint32_t maximumResourcesValue = 100) {
		maximumResources = maximumResourcesValue;
		paddedElementSize =
			core->gpuProperties.limits.minUniformBufferOffsetAlignment > sizeof(ResourceInfo) ?
			core->gpuProperties.limits.minUniformBufferOffsetAlignment :
			sizeof(ResourceInfo) / core->gpuProperties.limits.minUniformBufferOffsetAlignment + 1;

		resourceBuf = Buffer::create(
			core,
			maximumResources * paddedElementSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 0
		);
		storedResources = 0;
		stagingBuf = Buffer::create(
			core,
			sizeof(ResourceInfo),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			(VmaAllocationCreateFlagBits)(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT),
			0
		);
		stagingBufMap = stagingBuf->allocattedInfo.pMappedData;
	}

	uint32_t getResourceOffset() {
		return DUResource<ResourceInfo>::paddedElementSize * this->index;
	}

	void makeResource(VulkanCore core, VkCommandPool pool, ResourceInfo info) {
		this->index = storedResources;
		storedResources++;
		memcpy(stagingBufMap, &info, sizeof(info));
		VkBufferCopy copier{};
		copier.srcOffset = 0;
		copier.size = sizeof(ResourceInfo);
		copier.dstOffset = this->index * (paddedElementSize);
		copyBuffer(core, pool, { BufferCopyInfo(stagingBuf->buffer, resourceBuf->buffer, copier) });
		this->resourceInfo = info;
	}

	static VkDescriptorSet createDescriptor(VulkanCore core, VkShaderStageFlags shaderStageFlags) {
		VkDescriptorSetLayoutBinding binding0{};
		binding0.binding = 0;
		binding0.descriptorCount = 1;
		binding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		binding0.stageFlags = shaderStageFlags;
		auto descriptor = core->createDescriptorSet({ binding0 });

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		write.dstSet = descriptor;
		write.dstBinding = 0;
		VkDescriptorBufferInfo inf{};
		inf.buffer = resourceBuf->buffer;
		inf.offset = 0;
		inf.range = sizeof(ResourceInfo);
		write.pBufferInfo = &inf;
		vkUpdateDescriptorSets(core->device, 1, &write, 0, nullptr);

		return descriptor;
	}
};

template<class BaseInfo, class ArrayInfo>
class IAResource {
	//INDEFINITE ARRAY RESOURCE
	//STORED AS SHADER STORAGE BUFFER
	//change to store base info in UBO and array in storage buffer
	//above suggestion is bad since ubo are readonly from shaders
public:
	std::shared_ptr<Buffer> resourceBuf;
	uint32_t maxLength;

	void init(VulkanCore core, uint32_t maximumResourcesValue = 100, VkBufferUsageFlagBits additionalBufferUsageFlags = (VkBufferUsageFlagBits)0) {
		maxLength = maximumResourcesValue;
		resourceBuf = Buffer::create(
			core,
			sizeof(BaseInfo) + maxLength * sizeof(ArrayInfo),
			additionalBufferUsageFlags | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 0
		);
	}

	VkDeviceSize getBaseInfoOffset() {
		return 0;
	}

	void updateWhole(VulkanCore core, VkCommandPool pool, BaseInfo baseInfo, ArrayInfo* arrInfo, size_t arrLen, size_t arrOffset) {
		assert(arrOffset + arrLen <= maxLength, "Buffer array updation out of bounds");
		auto stagingBuf = Buffer::create(
			core,
			sizeof(BaseInfo) + arrLen * sizeof(ArrayInfo),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			(VmaAllocationCreateFlagBits)(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT),
			0
		);
		auto stagingBufMap = stagingBuf->allocattedInfo.pMappedData;
		memcpy(stagingBufMap, &baseInfo, sizeof(BaseInfo));
		memcpy(reinterpret_cast<char*>(stagingBufMap) + sizeof(BaseInfo), arrInfo, sizeof(ArrayInfo) * arrLen);
		
		VkBufferCopy copierBase{};
		copierBase.srcOffset = 0;
		copierBase.dstOffset = 0;
		copierBase.size = sizeof(BaseInfo);

		VkBufferCopy copierArr{};
		copierArr.srcOffset = sizeof(BaseInfo);
		copierArr.dstOffset = sizeof(BaseInfo) + arrOffset * sizeof(ArrayInfo);
		copierArr.size = arrLen * sizeof(ArrayInfo);

		std::vector<BufferCopyInfo> copyRegions = { 
			BufferCopyInfo(stagingBuf->buffer, resourceBuf->buffer, copierBase),
			BufferCopyInfo(stagingBuf->buffer, resourceBuf->buffer, copierArr) 
		};

		if (arrLen == 0) 
			copyRegions.pop_back();

		copyBuffer(core, pool, copyRegions);
	}

	inline void updateBase(VulkanCore core, VkCommandPool pool, BaseInfo baseInfo) {
		this->updateWhole(core, pool, baseInfo, nullptr, 0, 0);
	}

	void updateArray(VulkanCore core, VkCommandPool pool, ArrayInfo* arrInfo, size_t arrLen, size_t arrOffset) {
		assert(arrOffset + arrLen <= maxLength, "Buffer array updation out of bounds");
		auto stagingBuf = Buffer::create(
			core,
			arrLen * sizeof(ArrayInfo),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			(VmaAllocationCreateFlagBits)(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT),
			0
		);
		auto stagingBufMap = stagingBuf->allocattedInfo.pMappedData;
		memcpy(stagingBufMap, arrInfo, sizeof(ArrayInfo) * arrLen);
		VkBufferCopy copier{};
		copier.srcOffset = 0;
		copier.dstOffset = sizeof(BaseInfo) + arrOffset * sizeof(ArrayInfo);
		copier.size = arrLen * sizeof(ArrayInfo);
		copyBuffer(core, pool, { BufferCopyInfo(stagingBuf->buffer, resourceBuf->buffer, copier) });
	}

	VkDescriptorSet createDescriptor(VulkanCore core, VkShaderStageFlags shaderStageFlags) {
		//todo update --> done
		//todo test
		VkDescriptorSetLayoutBinding binding0{};
		binding0.binding = 0;
		binding0.descriptorCount = 1;
		binding0.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding0.stageFlags = shaderStageFlags;

		VkDescriptorSetLayoutBinding binding1{};
		binding1.binding = 1;
		binding1.descriptorCount = 1;
		binding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding1.stageFlags = shaderStageFlags;
		auto descriptor = core->createDescriptorSet({ binding0, binding1 });

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.dstSet = descriptor;
		write.dstBinding = 0;
		VkDescriptorBufferInfo inf{};
		inf.buffer = resourceBuf->buffer;
		inf.offset = 0;
		inf.range = sizeof(BaseInfo);
		write.pBufferInfo = &inf;

		auto write2 = write;
		write.dstBinding = 1;
		auto inf2 = inf;
		inf2.offset = sizeof(BaseInfo);
		inf2.range = sizeof(ArrayInfo) * this->maxLength;
		write.pBufferInfo = &inf2;

		VkWriteDescriptorSet writes[2] = { write, write2 };
		vkUpdateDescriptorSets(core->device, 2, writes, 0, nullptr);
		return descriptor;
	}
};

//todo create specializations for IAResource where there is no base info or where there is no array info
struct NullResourceInfo {};
template<class ArrayInfo>
class IAResource<NullResourceInfo, ArrayInfo> {
	//INDEFINITE ARRAY RESOURCE
	//STORED AS SHADER STORAGE BUFFER
	//THIS SPECIALIZATION DOES NOT HAVE ANY BASE INFO
public:
	std::shared_ptr<Buffer> resourceBuf;
	uint32_t maxLength;

	void init(VulkanCore core, uint32_t maximumResourcesValue = 100) {
		maxLength = maximumResourcesValue;
		resourceBuf = Buffer::create(
			core,
			maxLength * sizeof(ArrayInfo),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 0
		);
	}

	void updateArray(VulkanCore core, VkCommandPool pool, ArrayInfo* arrInfo, size_t arrLen, size_t arrOffset) {
		assert(arrOffset + arrLen <= maxLength, "Buffer array updation out of bounds");
		auto stagingBuf = Buffer::create(
			core,
			arrLen * sizeof(ArrayInfo),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			(VmaAllocationCreateFlagBits)(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT),
			0
		);

		void* stagingBufMap = stagingBuf->allocattedInfo.pMappedData;
		int a = sizeof(ArrayInfo);
		memcpy(stagingBufMap, arrInfo, sizeof(ArrayInfo) * arrLen);
		VkBufferCopy copier{};
		copier.srcOffset = 0;
		copier.dstOffset = arrOffset * sizeof(ArrayInfo);
		copier.size = arrLen * sizeof(ArrayInfo);
		copyBuffer(core, pool, { BufferCopyInfo(stagingBuf->buffer, resourceBuf->buffer, copier) });
	}

	VkDescriptorSet createDescriptor(VulkanCore core, VkShaderStageFlags shaderStageFlags) {
		//todo update --> done
		//todo test
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = 0;
		binding.descriptorCount = 1;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding.stageFlags = shaderStageFlags;
		auto descriptor = core->createDescriptorSet({ binding });

		VkDescriptorBufferInfo inf{};
		inf.buffer = resourceBuf->buffer;
		inf.offset = 0;
		inf.range = sizeof(ArrayInfo) * this->maxLength;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.dstSet = descriptor;
		write.dstBinding = 0;
		write.pBufferInfo = &inf;

		vkUpdateDescriptorSets(core->device, 1, &write, 0, nullptr);
		return descriptor;
	}
};

struct MaterialInfo {
	glm::vec4 baseColorFactor;
	glm::vec4 metallicRoughness_waste2;
	glm::uvec4 baseTexId_metallicRoughessTexId_waste2;
};

class Material : public DUResource<MaterialInfo> {
private:
	static void createMaterialsDescriptorSet() {
		VkDescriptorSetLayoutBinding binding0{};
		{
			binding0.binding = 0;
			binding0.descriptorCount = 1;
			binding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			binding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		VkDescriptorSetLayoutBinding binding1{};
		{
			binding1.binding = 1;
			binding1.descriptorCount = 1024;
			binding1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		VkDescriptorBindingFlags bindFlags[2] = { 0, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT };
		VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
		extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		extendedInfo.pNext = nullptr;
		extendedInfo.bindingCount = 2;
		extendedInfo.pBindingFlags = bindFlags;

		auto descriptor = VulkanUtils::utils().getCore()->createDescriptorSet({ binding0, binding1 }, 0, &extendedInfo);

		VkWriteDescriptorSet write0{};
		{
			write0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write0.descriptorCount = 1;
			write0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			write0.dstSet = descriptor;
			write0.dstBinding = 0;
			VkDescriptorBufferInfo inf{};
			inf.buffer = Material::resourceBuf->buffer;
			inf.offset = 0;
			inf.range = sizeof(Material::resourceInfo);
			write0.pBufferInfo = &inf;
		}
		vkUpdateDescriptorSets(VulkanUtils::utils().getCore()->device, 1, &write0, 0, nullptr);
		descriptorSet = descriptor;
	}
public:
	inline static std::vector<RC<Image>> materialImages;
	inline static RC<Sampler> sampler;
	inline static VkDescriptorSet descriptorSet;

	static unsigned int addMaterialImage(RC<Image> vImage, RC<Sampler> vSampler) {
		sampler = vSampler;
		unsigned int index = materialImages.size();
		materialImages.push_back(vImage);
		VkWriteDescriptorSet write1{};
		write1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write1.descriptorCount = 1;
		write1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write1.dstSet = descriptorSet;
		write1.dstBinding = 1;
		write1.dstArrayElement = index;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = vImage->layout;
		imageInfo.imageView = vImage->getView(
			VulkanUtils::utils().getCore(),
			vImage->format,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		imageInfo.sampler = sampler->sampler;
		write1.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(VulkanUtils::utils().getCore()->device, 1, &write1, 0, nullptr);
		return index;
	}

	static void clear() {
		for (int i = 0; i < materialImages.size(); i++) {
			//if (materialImages[i].use_count() != 1) {
			//	std::cout << 
			//		"Warning: cleared materials but image " << i << 
			//		" is not unique"<< std::endl;
			//}
		}
		materialImages.clear();
	}

	static void init(size_t maxMaterialsCount) {
		DUResource<MaterialInfo>::init(VulkanUtils::utils().getCore(), maxMaterialsCount);
		createMaterialsDescriptorSet();
	}

	static VkDescriptorSet getDescriptorSet() {
		return descriptorSet;
	}

	static std::shared_ptr<Material> create(VulkanCore core, VkCommandPool pool, MaterialInfo info) {
		Material mat{};
		mat.makeResource(core, pool, info);
		return std::make_shared<Material>(mat);
	}
};

inline MaterialInfo randomMaterial() {
	auto r = []() {
		return (float)std::rand() / (float)RAND_MAX;
	};
	auto mat = MaterialInfo{ glm::vec4(r(), r(), r(), r()), glm::vec4(r(), r(), r(), r()) };
	return mat;
}

struct PointLightInfo {
	glm::vec4 color;
	glm::vec3 position;
	float radius;
};

struct alignas(sizeof(PointLightInfo)) PointLightLength {
	int length;
};

class PointLightsBuffer : public IAResource<PointLightLength, PointLightInfo> {
public:
	size_t count = 0;
	void addLights(VulkanCore core, VkCommandPool pool, PointLightInfo* lights, size_t lightsCount) {
		PointLightLength length;
		length.length = this->count + lightsCount;
		this->updateWhole(core, pool, length, lights, lightsCount, this->count);
		this->count += lightsCount;
	}
};

class Mesh {
public:
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indexBuffer;
	uint32_t numIndices;
	Mesh() = default;
	Mesh(VulkanCore core, const VkCommandPool commandPool, MeshData<Vertex3>& meshData) {
		const auto& vertexData = meshData.vertices;
		size_t vertexDataSize = vertexData.size() * sizeof(Vertex3);
		this->vertexBuffer = Buffer::create(
			core, vertexDataSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			(VmaAllocationCreateFlagBits)0,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		auto vertexStagingBuffer = prepareStagingBuffer(core, vertexData.data(), vertexDataSize);


		std::vector<uint16_t> indexData(meshData.indices.begin(), meshData.indices.end());
		size_t indexDataSize = indexData.size() * sizeof(uint16_t);
		this->numIndices = indexData.size();
		this->indexBuffer = Buffer::create(
			core, indexDataSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			(VmaAllocationCreateFlagBits)0,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		auto indexStagingBuffer = prepareStagingBuffer(core, indexData.data(), indexDataSize);

		VkBufferCopy copier{};
		copier.dstOffset = 0; copier.srcOffset = 0;
		copier.size = vertexDataSize;
		BufferCopyInfo vBufferCI(vertexStagingBuffer->buffer, vertexBuffer->buffer, copier);
		copier.size = indexDataSize;
		BufferCopyInfo iBufferCI(indexStagingBuffer->buffer, indexBuffer->buffer, copier);

		copyBuffer(core, commandPool, { vBufferCI, iBufferCI });
	}
};

class SwapChain {
public:
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::shared_ptr<Image> depthImage;
	VkImageView depthImageView;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	uint32_t framesInFlight = 0;
	VulkanCore core;

	SwapChain() = default;
	SwapChain(
		VulkanCore core, VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, VkExtent2D extent,
		const VkRenderPass& renderPass) : core(core) {
		createSwapChain(core->findQueueFamilies(), core->querySwapChainSupport(), surfaceFormat, presentMode, extent);
		createImageViews(core->device);
		createFramebuffers(core->device, renderPass);
	}

	void cleanupSwapChain() {
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(core->device, imageView, nullptr);
		}
		vkDestroyImageView(core->device, depthImageView, nullptr);
		vkDestroySwapchainKHR(core->device, swapChain, nullptr);
	}

	void cleanupFramebuffers() {
		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(core->device, framebuffer, nullptr);
		}
	}

	uint32_t acquireNextImage(const VkDevice& device, const VkSemaphore& imageAvailableSemaphore) {
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("OutOfDateSwapchain");
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		framesInFlight++;
		return imageIndex;
	}

	void present(const VkQueue& presentQueue, const VkSemaphore* waitSemaphores, uint32_t imageIndex) {
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = waitSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			throw OutOfDateSwapChainError();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
		framesInFlight--;
	}

private:
	void createSwapChain(
		QueueFamilyIndices indices,
		SwapChainSupportDetails swapChainSupport, VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, VkExtent2D extent) {
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = core->surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(core->device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(core->device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(core->device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		VmaAllocationCreateInfo allocationCI{};

		depthImage = Image::create(
			core,
			Image::makeCreateInfo(
				core->findDepthFormat(),
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VkExtent3D{ swapChainExtent.width, swapChainExtent.height, 1 }
			),
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
		);
	}

	void createImageViews(const VkDevice& device) {
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}
		}

		depthImageView = depthImage->getView(core, depthImage->format, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	void createFramebuffers(const VkDevice& device, const VkRenderPass& renderPass) {
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[2] = {
				swapChainImageViews[i],
				depthImageView
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 2;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

};
