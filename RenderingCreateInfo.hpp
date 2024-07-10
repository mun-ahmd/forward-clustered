#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include "EnumsFromStrings.hpp"

namespace Rendering {
	typedef uint32_t ResourceID;

	struct ImageCreateInfo {
		std::string format;

		//for now always assume as both transfer src or dst
		//bool isTransferSrc = true;
		//bool isTransferDst = true;
		bool isColorAttachment = false;
		bool isDepthAttachment = false;
		bool isSampled = false;
		bool isStorage = false;

		uint32_t numDimensions = 2;
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;

		uint32_t mipLevels = 1;
		uint32_t arrayLayers = 1;
	};

	struct ImageViewCreateInfo {
		std::string aspectMask;
		int levelCount;
		int arrayLayerCount;
	};


	struct BufferCreateInfo {
		uint64_t size;
		bool createDedicatedMemory;	//gpu local or not
		bool createMapped;
	};

	struct BufferCopyInfo {
		Rendering::ResourceID srcBuffer;
		Rendering::ResourceID dstBuffer;
		uint64_t srcOffset;
		uint64_t dstOffset;
		uint64_t size;
	};

	struct BufferToImageCopy {
		uint64_t bufferOffset;
		uint32_t bufferRowLength;
		uint32_t bufferImageHeight;
		
		std::string imageAspect;
		
		uint32_t mipLevel;
		uint32_t baseArrayLayer;
		uint32_t layerCount;

		int32_t imageOffsetX;
		int32_t imageOffsetY;
		int32_t imageOffsetZ;

		uint32_t imageWidth;
		uint32_t imageHeight;
		uint32_t imageDepth;
	};

	struct SamplerCreateInfo {
		std::array<std::optional<VkFilter>, 2> minMagFilter = {};
		std::array<std::optional<VkSamplerAddressMode>, 3> uvwAddressMode = {};
		std::array<std::optional<float>, 3> mipLodBiasMinMax = {};
		std::optional<VkSamplerMipmapMode> mipMapMode = {};
		std::optional<float> maxAnisotropy = {};
		std::optional<VkCompareOp> compareOp = {};
		std::optional<VkBorderColor> borderColor = {};
		std::optional<VkSamplerCreateFlags> flags = {};
		VkBool32 unnormCoords = VK_FALSE;

		VkSamplerCreateInfo getCreateInfo() {
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

		void setMinFilter(std::string filter) {
			this->minMagFilter[0] = filterFromString(filter);
		}
		void setMagFilter(std::string filter) {
			this->minMagFilter[1] = filterFromString(filter);
		}
		void setAnisotropy(float value) {
			this->maxAnisotropy = value;
		}
		//todo complete

	};

	struct DescriptorPoolCreateInfo {
		uint32_t maxSets;
		uint32_t uniformBufferCount;
		uint32_t uniformBufferDynamicCount;
		uint32_t storageBufferCount;
		uint32_t combinedImageSamplerCount;
	};

	struct DescriptorSetCreateInfo {
		ResourceID descriptorPool;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		std::vector<VkDescriptorBindingFlags> bindingFlags;

		void addBinding(
			int bindingIndex,
			uint32_t descriptorCount,
			std::string descriptorType,
			std::string shaderStages,
			bool isPartiallyBound
		) {
			VkDescriptorSetLayoutBinding binding{};
			{
				binding.binding = 0;
				binding.descriptorCount = 1;
				binding.descriptorType = descriptorTypeFromString(descriptorType);
				binding.stageFlags = shaderStageFlagsFromString(shaderStages);
			}
			this->bindings.push_back(binding);

			while (this->bindingFlags.size() <= bindingIndex) {
				this->bindingFlags.push_back(0);
			}
			if (isPartiallyBound) {
				this->bindingFlags[bindingIndex] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
			}
		}

	};

	struct PipelineLayoutCreateInfo {
		std::vector<VkPushConstantRange> pushConstants; //modified through methods, not directly
		std::vector<ResourceID> setsForSetLayouts;
		//todo add shaderStageFlags
		void addPushConstant(uint32_t offset, uint32_t size, std::string shaderStageFlags) {
			VkPushConstantRange range{};
			range.offset = offset;
			range.size = size;
			range.stageFlags = shaderStageFlagsFromString(shaderStageFlags);
			this->pushConstants.push_back(range);
		}
		void addSetLayout(ResourceID descriptorSet) {
			this->setsForSetLayouts.push_back(descriptorSet);
		}
	};

	struct PipelineCreateInfo {
		ResourceID vertexShaderModule;
		ResourceID fragmentShaderModule;
		ResourceID pipelineLayout;

		bool sampleShadingEnable;
		float minSampleShading;
		uint32_t sampleCount;

		bool depthTestEnable;
		bool depthWriteEnable;
		bool depthBoundsTestEnable;
		bool stencilTestEnable;
		std::string depthCompareOp;

		std::string polygonMode;

		std::vector<VkFormat> colorAttachmentFormats;
		std::string depthAttachmentFormat;

		void addColorAttachment(std::string colorAttachmentFormat) {
			this->colorAttachmentFormats.push_back(formatFromString(colorAttachmentFormat));
		}

		std::vector<VkVertexInputBindingDescription> vertexBindings;

		void addVertexBinding(uint32_t binding, uint32_t stride, bool isPerInstance) {
			VkVertexInputBindingDescription vBinding{};
			vBinding.binding = binding;
			vBinding.inputRate = isPerInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
			vBinding.stride = stride;
			this->vertexBindings.push_back(vBinding);
		}

		std::vector<VkVertexInputAttributeDescription> vertexAttributes;

		void addVertexAttribute(uint32_t binding, uint32_t location, uint32_t offset, std::string format) {
			VkVertexInputAttributeDescription vAttribute{};
			vAttribute.binding = binding;
			vAttribute.format = formatFromString(format);
			vAttribute.location = location;
			vAttribute.offset = offset;
			this->vertexAttributes.push_back(vAttribute);
		}

		
	};

	struct CommandBufferSubmitInfo {
		std::vector<Rendering::ResourceID> waitSemaphores;
		std::vector<VkPipelineStageFlags> waitStages;
		std::vector<ResourceID> signalSemaphores;
		ResourceID fence;

		void addWaitSemaphore(ResourceID semaphore, std::string waitStage) {
			this->waitSemaphores.push_back(semaphore);
			this->waitStages.push_back(pipelineStageFlagsFromString(waitStage));
		}

		void addSignalSemaphore(ResourceID semaphore) {
			this->signalSemaphores.push_back(semaphore);
		}
	};

	struct LightProperties {

	};

	struct LightCreateInfo {

	};

	struct MaterialCreateInfo {

	};

	struct MeshCreateInfo {

	};

	struct InstanceInfo {

	};

	struct RenderedInstanceCreateInfo {
		ResourceID mesh;
		ResourceID material;
		InstanceInfo info;
	};

	struct ViewportInfo {
		float width;
		float height;
		float offsetX;
		float offsetY;
		float maxDepth;
		float minDepth;

		VkViewport getAsVk() {
			VkViewport v{};
			v.width = width;
			v.height = height;
			v.x = offsetX;
			v.y = offsetY;
			v.maxDepth = maxDepth;
			v.minDepth = minDepth;
			return v;
		}
	};

	struct Rect2D {
		uint32_t width;
		uint32_t height;
		int32_t offsetX;
		int32_t offsetY;

		VkRect2D getAsVk() {
			VkRect2D rect{};
			rect.extent.width = width;
			rect.extent.height = height;
			rect.offset.x = offsetX;
			rect.offset.y = offsetY;
			return rect;
		}
	};

	struct RenderingInfo {
	
	private:
		VkRenderingAttachmentInfo __formInfo__(
			VkClearValue clearValue,
			std::string layout,
			std::string loadOp,
			std::string storeOp
		) {
			VkRenderingAttachmentInfo info{};
			info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			
			info.clearValue = clearValue;
			
			info.imageLayout = imageLayoutFromString(layout);
			info.imageView = VK_NULL_HANDLE; //to be set later within the rendering server

			info.loadOp = attachmentLoadOpFromString(loadOp);
			info.storeOp = attachmentStoreOpFromString(storeOp);

			//todo see if there is a need for resolve modes
			info.resolveMode = VK_RESOLVE_MODE_NONE;
			//info.resolveImageView
			//info.resolveImageLayout;

			return info;
		}

	public:
		VkRect2D renderArea;
		void setRenderArea(Rect2D rect) {
			this->renderArea = rect.getAsVk();
		}

		std::vector<VkRenderingAttachmentInfo> colorAttachments;
		std::vector<Rendering::ResourceID> colorAttachmentImageViews;
		
		VkRenderingAttachmentInfo depthAttachment;
		ResourceID depthImageView;
		
		VkRenderingAttachmentInfo stencilAttachment;
		ResourceID stencilImageView;

		void setStencilAttachment(
			Rendering::ResourceID imageView,
			std::string imageLayout,
			std::string loadOp,
			std::string storeOp,
			uint32_t clearVal
		) {
			VkClearValue clearValue{};
			clearValue.depthStencil.stencil = clearVal;

			stencilAttachment = __formInfo__(clearValue, imageLayout, loadOp, storeOp);

			stencilImageView = imageView;
		}

		void setDepthAttachment(
			Rendering::ResourceID imageView,
			std::string imageLayout,
			std::string loadOp,
			std::string storeOp,
			float clearVal
		) {
			VkClearValue clearValue{};
			clearValue.depthStencil.depth = clearVal;

			depthAttachment = __formInfo__(clearValue, imageLayout, loadOp, storeOp);
			
			depthImageView = imageView;
		}

		void addColorAttachment(
			Rendering::ResourceID imageView,
			std::string imageLayout,
			std::string loadOp,
			std::string storeOp,
			float clearR, float clearG, float clearB, float clearA
		) {

			VkClearValue clearValue{};
			clearValue.color.float32[0] = clearR;
			clearValue.color.float32[1] = clearG;
			clearValue.color.float32[2] = clearB;
			clearValue.color.float32[3] = clearA;

			colorAttachments.push_back(__formInfo__(clearValue, imageLayout, loadOp, storeOp));

			colorAttachmentImageViews.push_back(imageView);
		}

	};

};