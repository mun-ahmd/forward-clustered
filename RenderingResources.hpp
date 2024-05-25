#pragma once
#include "core.hpp"

namespace Rendering {
	struct Buffer {
		VkBuffer buffer;
		VmaAllocation allocation;
		uint64_t allocatedSize;
	};

	struct Image {
		VkImage image;
		VkImageLayout layout;
		VkFormat format;
		VkImageType imageType;
		VkExtent3D extent;
		uint32_t mipLevels;
		uint32_t arrayLayers;

		VmaAllocation allocation;
	};

	struct ImageView {
		VkImageView view;
		VkImageViewCreateInfo info;
	};

	struct CommandBuffer {
		VkCommandBuffer buffer;
	};

	struct DescriptorSet {
		VkDescriptorSet set;
		VkDescriptorSetLayout layout;
	};

	struct Pipeline {
		VkPipeline pipeline;
		VkPipelineLayout layout;
		ResourceID layoutID;
	};
};