#pragma once
#include <array>
#include <optional>
#include "vulkan_utils.hpp"
#include "buffer.hpp"

//forward declaration
class ImageView;
class Image;

class ImageView {
private:
	ImageView() = default;

public:
	VkImageView view;
	VkImageViewCreateInfo info;
	std::shared_ptr<Image> image;

	inline static int viewAllocatedCount = 0;
	struct ImageViewDeleter {
		void operator()(ImageView* imageView) {
			vkDestroyImageView(
				VulkanUtils::utils().getCore()->device,
				imageView->view,
				nullptr
			);
			viewAllocatedCount--;
			delete imageView;
		}
	};

	static std::unique_ptr<ImageView, ImageViewDeleter> create(
		std::shared_ptr<Image> baseImage,
		VkImageViewCreateInfo info
	) {
		ImageView imageView;
		imageView.image = baseImage;
		imageView.info = info;
		if (vkCreateImageView(
			VulkanUtils::utils().getCore()->device,
			&info,
			nullptr,
			&imageView.view
		) != VK_SUCCESS) {
			throw std::runtime_error("Could not create image view!");
		}

		viewAllocatedCount++;
		return std::unique_ptr<ImageView, ImageViewDeleter>(new ImageView(imageView));
	}
};

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

	inline static int imageAllocattedCount = 0;
	struct ImageDeleter {
		void operator()(Image* img) {
			vmaDestroyImage(VulkanUtils::utils().getCore()->allocator, img->image, img->allocation);
			delete img;
			imageAllocattedCount--;
		}
	};

	static std::unique_ptr<Image, ImageDeleter> create(
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
			) {
			throw std::runtime_error("Could not create image resource!");
		}

		imageAllocattedCount++;
		return std::unique_ptr<Image, ImageDeleter>(new Image(img));
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

inline std::unique_ptr<ImageView, ImageView::ImageViewDeleter> getImageView(
	std::shared_ptr<Image> image,
	VkImageViewCreateInfo info
) {
	//Image must be converted to a shared pointer due to views sharing ownership of the image
	//Image wont be deleted until the view is deleted
	return ImageView::create(image, info);
}

inline std::unique_ptr<ImageView, ImageView::ImageViewDeleter> getImageView(
	std::shared_ptr<Image> image,
	VkFormat viewFormat,
	VkImageAspectFlags aspectFlags
)
{
	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext = nullptr;

	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.image = image->image;
	info.format = viewFormat;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.aspectMask = aspectFlags;

	return getImageView(image, info);
}

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

	inline static int samplerAllocattedCount = 0;
	struct SamplerDeleter {
		void operator()(Sampler* sampler) {
			vkDestroySampler(VulkanUtils::utils().getCore()->device, sampler->sampler, nullptr);
			delete sampler;
			samplerAllocattedCount--;
		}
	};

	static std::unique_ptr<Sampler, SamplerDeleter> create(
		VulkanCore core,
		VkSamplerCreateInfo info
	) {
		Sampler sampler{};
		if (vkCreateSampler(core->device, &info, nullptr, &sampler.sampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Sampler!");
		}

		samplerAllocattedCount++;
		return std::unique_ptr<Sampler, SamplerDeleter>(new Sampler(sampler));
	}
};

using UniqueImage = std::unique_ptr<Image, Image::ImageDeleter>;
using UniqueImageView = std::unique_ptr<ImageView, ImageView::ImageViewDeleter>;
using UniqueSampler = std::unique_ptr<Sampler, Sampler::SamplerDeleter>;