#pragma once
#include "core.hpp"

struct BufferCopyInfo {
	VkBuffer src;
	VkBuffer dst;
	VkBufferCopy inf;

	BufferCopyInfo() = default;
	BufferCopyInfo(VkBuffer src, VkBuffer dst, VkBufferCopy inf) : src(src), dst(dst), inf(inf) {}
};

class Buffer {

	//todo big problem, the mapped pointer in allocatedInfo can change due to defragmentation
	//need to remove all instances where this is used
	VmaAllocationInfo allocattedInfo;

public:
	VkBuffer buffer;
	VmaAllocation allocation;

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

inline auto prepareStagingBufferPersistant(VulkanCore core, size_t dataSize) {
	auto stagingBuffer = Buffer::create(
		core, dataSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		(VmaAllocationCreateFlagBits)(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT),
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);
	return stagingBuffer;
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
