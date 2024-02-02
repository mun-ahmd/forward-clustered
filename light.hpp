#pragma once
#include "buffer_helpers.hpp"
#include "MeshLoader.hpp"


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


class LightsBuffer {
	//this object should not be stored for each frame in flight

	//todo create a persistently mapped staging buffer for each frame, to transfer stuff here and there to pointlights
private:
	size_t pointLightCount = 0;

	void initalizeSunLightBuffers() {
		for (auto& sunBuf : sunLightBuffers) {
			sunBuf = Buffer::create(
				VulkanUtils::utils().getCore(),
				sizeof(SunLightType),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				(VmaAllocationCreateFlagBits)(
					VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT |
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
					VMA_ALLOCATION_CREATE_MAPPED_BIT
					),
				0);
		}
	}

public:
	typedef DirectionalLightInfo SunLightType;

	IAResource<PointLightLength, PointLightInfo> pointLightsBuffer;
	std::vector<RC<Buffer>> sunLightBuffers;	//mapped so writing is easy
	std::vector<RC<Buffer>> pointLightStagingBuffers;	//todo implement 

	LightsBuffer() = default;
	LightsBuffer(VulkanCore core, uint32_t maxPointLightCount, uint32_t numFrames) {
		this->sunLightBuffers.resize(numFrames);
		this->pointLightStagingBuffers.resize(numFrames);

		this->initalizeSunLightBuffers();
		this->pointLightsBuffer.init(core, maxPointLightCount);
	}

	void addPointLights(VulkanCore core, VkCommandPool pool, PointLightInfo* lights, size_t lightsCount) {
		PointLightLength length;
		length.length = this->pointLightCount + lightsCount;
		this->pointLightsBuffer.updateWhole(core, pool, length, lights, lightsCount, this->pointLightCount);
		this->pointLightCount += lightsCount;
	}

	void setSunLight(SunLightType sunInfo, uint32_t frameIndex) {
		//sunlight buffer is mapped as it may change frequently
		memcpy(
			this->sunLightBuffers[frameIndex]->allocation->GetMappedData(),
			&sunInfo,
			sizeof(sunInfo)
		);
	}

	VkDescriptorSet createDescriptorSet(uint32_t frameIndex, VulkanCore core, VkShaderStageFlags shaderStageFlags) {
		std::vector<VkDescriptorSetLayoutBinding> bindings = 
			this->pointLightsBuffer.getDescriptorBindings(shaderStageFlags);
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = 2;
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			binding.stageFlags = shaderStageFlags;
			bindings.push_back(binding);
		}
		
		VkDescriptorSet descriptor = core->createDescriptorSet(bindings);

		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = this->sunLightBuffers[frameIndex]->buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(SunLightType);

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.dstSet = descriptor;
			write.dstBinding = 2;
			write.pBufferInfo = &bufferInfo;

			//write bindings 0 and 1
			this->pointLightsBuffer.writeToDescriptorSet(core, descriptor);
			//write binding 2
			vkUpdateDescriptorSets(
				core->device,
				1,
				&write,
				0,
				nullptr
			);
		}

		return descriptor;
	}

};

//todo make lights buffer