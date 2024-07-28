#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include "EnumsFromStrings.hpp"

#define EXPORTPROP(name)
#define EXPORTCLASS()
#define EXPORTTABLE()
#define EXPORTALL()

namespace Rendering {
	typedef EXPORTPROP("string") std::string anythingpleaseworksadopsadspa;

	typedef uint32_t ResourceID;

	struct EXPORTPROP("") EXPORTCLASS() ImageCreateInfo{
		std::string EXPORTPROP("") format;

		//for now always assume as both transfer src or dst
		//bool isTransferSrc = true;
		//bool isTransferDst = true;
		bool EXPORTPROP("") isColorAttachment = false;
		bool EXPORTPROP("") isDepthAttachment = false;
		bool EXPORTPROP("") isSampled = false;
		bool EXPORTPROP("") isStorage = false;

		uint32_t EXPORTPROP("") numDimensions = 2;
		uint32_t EXPORTPROP("") width = 1;
		uint32_t EXPORTPROP("") height = 1;
		uint32_t EXPORTPROP("") depth = 1;

		uint32_t EXPORTPROP("") mipLevels = 1;
		uint32_t EXPORTPROP("") arrayLayers = 1;
	};

	struct EXPORTPROP("") EXPORTCLASS() ImageViewCreateInfo {
		std::string EXPORTPROP("") aspectMask;
		int EXPORTPROP("") levelCount;
		int EXPORTPROP("") arrayLayerCount;
	};


	struct EXPORTPROP("") EXPORTCLASS() BufferCreateInfo {
		uint64_t EXPORTPROP("") size;
		bool EXPORTPROP("") createDedicatedMemory;	//gpu local or not
		bool EXPORTPROP("") createMapped;
	};

	struct EXPORTPROP("") EXPORTCLASS() BufferCopyInfo {
		Rendering::ResourceID EXPORTPROP("") srcBuffer;
		Rendering::ResourceID EXPORTPROP("") dstBuffer;
		uint64_t EXPORTPROP("") srcOffset;
		uint64_t EXPORTPROP("") dstOffset;
		uint64_t EXPORTPROP("") size;
	};

	struct EXPORTPROP("") EXPORTCLASS() BufferToImageCopy {
		uint64_t EXPORTPROP("") bufferOffset;
		uint32_t EXPORTPROP("") bufferRowLength;
		uint32_t EXPORTPROP("") bufferImageHeight;
		
		std::string EXPORTPROP("") imageAspect;
		
		uint32_t EXPORTPROP("") mipLevel;
		uint32_t EXPORTPROP("") baseArrayLayer;
		uint32_t EXPORTPROP("") layerCount;

		int32_t EXPORTPROP("") imageOffsetX;
		int32_t EXPORTPROP("") imageOffsetY;
		int32_t EXPORTPROP("") imageOffsetZ;

		uint32_t EXPORTPROP("") imageWidth;
		uint32_t EXPORTPROP("") imageHeight;
		uint32_t EXPORTPROP("") imageDepth;
	};

	struct EXPORTPROP("") EXPORTCLASS() SamplerCreateInfo {
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

		void EXPORTPROP("") setMinFilter(std::string filter) {
			this->minMagFilter[0] = filterFromString(filter);
		}
		void EXPORTPROP("") setMagFilter(std::string filter) {
			this->minMagFilter[1] = filterFromString(filter);
		}
		void EXPORTPROP("") setAnisotropy(float value) {
			this->maxAnisotropy = value;
		}
		//todo complete

	};

	struct EXPORTPROP("") EXPORTCLASS() DescriptorPoolCreateInfo {
		uint32_t EXPORTPROP("") maxSets;
		uint32_t EXPORTPROP("") uniformBufferCount;
		uint32_t EXPORTPROP("") uniformBufferDynamicCount;
		uint32_t EXPORTPROP("") storageBufferCount;
		uint32_t EXPORTPROP("") combinedImageSamplerCount;
	};

	struct EXPORTPROP("") EXPORTCLASS() DescriptorSetCreateInfo {
		ResourceID EXPORTPROP("") descriptorPool;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		std::vector<VkDescriptorBindingFlags> bindingFlags;

		void EXPORTPROP("") addBinding(
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

	struct EXPORTPROP("") EXPORTCLASS() PipelineLayoutCreateInfo {
		std::vector<VkPushConstantRange> pushConstants; //modified through methods, not directly
		std::vector<ResourceID> setsForSetLayouts;
		//todo add shaderStageFlags
		void EXPORTPROP("") addPushConstant(uint32_t offset, uint32_t size, std::string shaderStageFlags) {
			VkPushConstantRange range{};
			range.offset = offset;
			range.size = size;
			range.stageFlags = shaderStageFlagsFromString(shaderStageFlags);
			this->pushConstants.push_back(range);
		}
		void EXPORTPROP("") addSetLayout(ResourceID descriptorSet) {
			this->setsForSetLayouts.push_back(descriptorSet);
		}
	};

	struct EXPORTPROP("") EXPORTCLASS() PipelineCreateInfo {
		ResourceID EXPORTPROP("") vertexShaderModule;
		ResourceID EXPORTPROP("") fragmentShaderModule;
		ResourceID EXPORTPROP("") pipelineLayout;

		bool EXPORTPROP("") sampleShadingEnable;
		float EXPORTPROP("") minSampleShading;
		uint32_t EXPORTPROP("") sampleCount;

		bool EXPORTPROP("") depthTestEnable;
		bool EXPORTPROP("") depthWriteEnable;
		bool EXPORTPROP("") depthBoundsTestEnable;
		bool EXPORTPROP("") stencilTestEnable;
		std::string EXPORTPROP("") depthCompareOp;

		std::string EXPORTPROP("") polygonMode;

		std::string EXPORTPROP("") depthAttachmentFormat;

		std::vector<VkFormat> colorAttachmentFormats;
		void EXPORTPROP("") addColorAttachment(std::string colorAttachmentFormat) {
			this->colorAttachmentFormats.push_back(formatFromString(colorAttachmentFormat));
		}


		std::vector<VkVertexInputBindingDescription> vertexBindings;
		void EXPORTPROP("") addVertexBinding(uint32_t binding, uint32_t stride, bool isPerInstance) {
			VkVertexInputBindingDescription vBinding{};
			vBinding.binding = binding;
			vBinding.inputRate = isPerInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
			vBinding.stride = stride;
			this->vertexBindings.push_back(vBinding);
		}


		std::vector<VkVertexInputAttributeDescription> vertexAttributes;
		void EXPORTPROP("") addVertexAttribute(uint32_t binding, uint32_t location, uint32_t offset, std::string format) {
			VkVertexInputAttributeDescription vAttribute{};
			vAttribute.binding = binding;
			vAttribute.format = formatFromString(format);
			vAttribute.location = location;
			vAttribute.offset = offset;
			this->vertexAttributes.push_back(vAttribute);
		}
	};

	struct EXPORTPROP("") EXPORTCLASS() CommandBufferSubmitInfo {
		ResourceID EXPORTPROP("") fence;

		std::vector<Rendering::ResourceID> waitSemaphores;
		std::vector<VkPipelineStageFlags> waitStages;
		std::vector<ResourceID> signalSemaphores;

		void EXPORTPROP("") addWaitSemaphore(ResourceID semaphore, std::string waitStage) {
			this->waitSemaphores.push_back(semaphore);
			this->waitStages.push_back(pipelineStageFlagsFromString(waitStage));
		}

		void EXPORTPROP("") addSignalSemaphore(ResourceID semaphore) {
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

	struct EXPORTPROP("") EXPORTCLASS() InstanceInfo {

	};

	struct EXPORTPROP("") EXPORTCLASS() RenderedInstanceCreateInfo {
		ResourceID EXPORTPROP("") mesh;
		ResourceID EXPORTPROP("") material;
		InstanceInfo EXPORTPROP("") info;
	};

	struct EXPORTPROP("") EXPORTCLASS() ViewportInfo {
		float EXPORTPROP("") width;
		float EXPORTPROP("") height;
		float EXPORTPROP("") offsetX;
		float EXPORTPROP("") offsetY;
		float EXPORTPROP("") maxDepth;
		float EXPORTPROP("") minDepth;

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

	struct EXPORTPROP("") EXPORTCLASS() Rect2D {
		uint32_t EXPORTPROP("") width;
		uint32_t EXPORTPROP("") height;
		int32_t EXPORTPROP("") offsetX;
		int32_t EXPORTPROP("") offsetY;

		VkRect2D getAsVk() {
			VkRect2D rect{};
			rect.extent.width = width;
			rect.extent.height = height;
			rect.offset.x = offsetX;
			rect.offset.y = offsetY;
			return rect;
		}
	};

	struct RenderingInfo EXPORTPROP("") EXPORTCLASS() {
	
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
		void EXPORTPROP("") setRenderArea(Rect2D rect) {
			this->renderArea = rect.getAsVk();
		}

		VkRenderingAttachmentInfo stencilAttachment;
		ResourceID stencilImageView;
		void EXPORTPROP("") setStencilAttachment(
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

		VkRenderingAttachmentInfo depthAttachment;
		ResourceID depthImageView;
		void EXPORTPROP("") setDepthAttachment(
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

		std::vector<VkRenderingAttachmentInfo> colorAttachments;
		std::vector<Rendering::ResourceID> colorAttachmentImageViews;
		void EXPORTPROP("") addColorAttachment(
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