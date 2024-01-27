#pragma once
#include "buffer.hpp"
#include "vulkan_utils.hpp"

template<class ResourceInfo>
class DUResource {
	//DYNAMIC UNIFORM RESOURCE
public:
	std::shared_ptr<Buffer> resourceBuf;

	std::shared_ptr<Buffer> stagingBuf;
	void* stagingBufMap;

	size_t storedResources;
	size_t maximumResources;

	size_t paddedElementSize;

	void clearBuffer() {
		storedResources = 0;
	}

	void init(uint32_t maximumResourcesValue = 100) {
		auto core = VulkanUtils::utils().getCore();
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

	size_t getResourceOffset() {
		return (this->storedResources - 1) * this->paddedElementSize;
	}

	size_t getResourceOffset(size_t index) {
		assert(index < this->storedResources);
		return index * this->paddedElementSize;
	}

	void addResource(VulkanCore core, VkCommandPool pool, ResourceInfo info) {
		memcpy(this->stagingBufMap, &info, sizeof(info));

		VkBufferCopy copier{};
		copier.srcOffset = 0;
		copier.size = sizeof(ResourceInfo);
		copier.dstOffset = this->getResourceOffset();

		copyBuffer(core, pool, { BufferCopyInfo(stagingBuf->buffer, resourceBuf->buffer, copier) });

		this->storedResources++;
	}

	VkDescriptorSet createDescriptor(VulkanCore core, VkShaderStageFlags shaderStageFlags) {
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
