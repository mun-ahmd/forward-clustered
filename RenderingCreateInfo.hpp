#pragma once
#include <stdint.h>
#include <vector>
#include <string>

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

	struct DescriptorSetCreateInfo {

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
		std::vector<VkSemaphore> waitSemaphores;
		std::vector<VkPipelineStageFlags> waitStages;
		ResourceID signalSemaphore;

		void addWaitSemaphore(ResourceID semaphore, std::string waitStage) {
			//todo
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

	struct RenderingInfo {

	};

};