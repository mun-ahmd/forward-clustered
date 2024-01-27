#pragma once
#include "buffer_helpers.hpp"
#include "image.hpp"

struct MaterialInfo {
	glm::vec4 baseColorFactor;
	glm::vec4 metallicRoughness_waste2;
	glm::uvec4 baseTexId_metallicRoughessTexId_waste2;
};

class Materials : public DUResource<MaterialInfo> {
private:
	void createMaterialsDescriptorSet() {
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
			inf.buffer = this->resourceBuf->buffer;
			inf.offset = 0;
			inf.range = sizeof(MaterialInfo);
			write0.pBufferInfo = &inf;
		}
		vkUpdateDescriptorSets(VulkanUtils::utils().getCore()->device, 1, &write0, 0, nullptr);
		descriptorSet = descriptor;
	}

public:
	std::vector<UniqueImageView> materialImages;
	std::vector<RC<Sampler>> materialSamplers;

	std::vector<MaterialInfo> materialInfos;

	VkDescriptorSet descriptorSet;

	void init(size_t maxMaterialsCount) {
		DUResource<MaterialInfo>::init(maxMaterialsCount);
		createMaterialsDescriptorSet();
	}
	
	void clear() {
		this->materialImages.clear();
		this->materialInfos.clear();
		this->materialInfos.clear();
		this->clearBuffer();
	}

	VkDescriptorSet getDescriptorSet() {
		return descriptorSet;
	}

	void addMaterial(VulkanCore core, VkCommandPool pool, MaterialInfo info) {
		this->addResource(core, pool, info);
		this->materialInfos.push_back(info);
	}

	void reserve(size_t size) {
		this->materialInfos.reserve(size);
	}

	MaterialInfo& operator[](size_t index) {
		return this->materialInfos[index];
	}

	const MaterialInfo& operator[](size_t index) const {
		return this->materialInfos[index];
	}

	unsigned int addMaterialImage(RC<Image> vImage, RC<Sampler> vSampler) {
		unsigned int index = materialImages.size();
		
		materialSamplers.push_back(
			vSampler
		);

		materialImages.push_back(
			getImageView(
				vImage,
				vImage->format,
				VK_IMAGE_ASPECT_COLOR_BIT
			)
		);

		VkWriteDescriptorSet write1{};
		write1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write1.descriptorCount = 1;
		write1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write1.dstSet = descriptorSet;
		write1.dstBinding = 1;
		write1.dstArrayElement = index;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = vImage->layout;
		imageInfo.imageView = materialImages[index]->view;
		imageInfo.sampler = materialSamplers[index]->sampler;
		write1.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(VulkanUtils::utils().getCore()->device, 1, &write1, 0, nullptr);
		return index;
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
