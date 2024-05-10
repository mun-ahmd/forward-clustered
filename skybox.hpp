#pragma once

#include "vulkan_utils.hpp"
#include "image.hpp"
#include "asyncImageLoader.hpp"

//for simplicity drawn with fullscreen triangle instead of cube
class SkyboxRenderer {
public:
	struct CubemapPushConstants {
		glm::mat4 inverseProjView;
	};
private:

	RC<Image> image;
	UniqueImageView cubemap;
	UniqueSampler sampler;
	
	struct {
		VkPipeline pipe;
		VkPipelineLayout layout;
	} draw;

	struct {
		//set 3
		VkDescriptorSet descriptor;
	} perFrame[MAX_FRAMES_IN_FLIGHT];

	void createPipeline(VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat) {
		//pipeline creation
		auto core = VulkanUtils::utils().getCore();

		auto vertShaderCode = VulkanUtils::utils().compileGlslToSpv("Shaders/cubemap.vert", shaderc_shader_kind::shaderc_vertex_shader);
		auto fragShaderCode = VulkanUtils::utils().compileGlslToSpv("Shaders/cubemap.frag", shaderc_shader_kind::shaderc_fragment_shader);

		VkShaderModule vertShaderModule = VulkanUtils::utils().createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = VulkanUtils::utils().createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo emptyInputState{};
		emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		emptyInputState.vertexAttributeDescriptionCount = 0;
		emptyInputState.pVertexAttributeDescriptions = nullptr;
		emptyInputState.vertexBindingDescriptionCount = 0;
		emptyInputState.pVertexBindingDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencilCI{};
		depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCI.depthWriteEnable = VK_FALSE;
		depthStencilCI.depthTestEnable = VK_TRUE;
		depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilCI.depthBoundsTestEnable = VK_FALSE;
		depthStencilCI.stencilTestEnable = VK_FALSE;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPushConstantRange push_constant{};
		push_constant.offset = 0;
		push_constant.size = sizeof(CubemapPushConstants);
		push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		auto setLayout = core->getLayout(this->perFrame[0].descriptor);
		pipelineLayoutInfo.pSetLayouts = &setLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &push_constant;

		this->draw.layout = core->createPipelineLayout(pipelineLayoutInfo);

		VkPipelineRenderingCreateInfo renderingCreateInfo{};
		renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingCreateInfo.colorAttachmentCount = 1;
		renderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;
		renderingCreateInfo.depthAttachmentFormat = depthAttachmentFormat;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.pNext = &renderingCreateInfo;

		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &emptyInputState;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &depthStencilCI;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = this->draw.layout;

		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		this->draw.pipe = core->createGraphicsPipeline(pipelineInfo);

		vkDestroyShaderModule(core->device, fragShaderModule, nullptr);
		vkDestroyShaderModule(core->device, vertShaderModule, nullptr);
	}
	void createCubemap(RC<AsyncImageLoader> iLoader) {

		ImageLoadRequest req{};
		req.desiredChannels = 4;
		req.isFloat = true;
		req.path = 
			R"(C:\Users\munee\source\repos\VulkanRenderer\3DModels\meadow_2_8k.hdr)";
		auto sizeInfo = getImageInfo(req.path.c_str());
		req.resolution = glm::ivec3(sizeInfo.x, sizeInfo.y, 1);

		//equirectangular image
		VkExtent3D extent{};
		extent.width = sizeInfo.x;
		extent.height = sizeInfo.y;
		extent.depth = 1;

		auto CI = Image::makeCreateInfo(
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			extent
		);

		auto core = VulkanUtils::utils().getCore();
		
		image = Image::create(
			core,
			CI,
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
		);

		image->transitionImageToLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		this->cubemap = getImageView(
			image,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		req.callback = [weak_image=std::weak_ptr<Image>(image)](ImageLoadRequest& request, RC<Buffer> staging) {
			auto image = weak_image.lock();
			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = image->extent;

			image->transitionImageToLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
			image->copyFromBuffer(staging, region);
			image->transitionImageToLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		};

		iLoader->request(req);
	}
	void createSampler() {
		this->sampler = Sampler::create(
			VulkanUtils::utils().getCore(),
			Sampler::makeCreateInfo(

			)
		);
	}
	void createDescriptorSet() {
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorSetLayoutBinding binding{};
			{
				binding.binding = 0;
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			this->perFrame[i].descriptor =
				VulkanUtils::utils().getCore()->createDescriptorSet({ binding });

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write.dstSet = this->perFrame[i].descriptor;
			write.dstBinding = 0;
			write.dstArrayElement = 0;

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = this->cubemap->image->layout;
			imageInfo.imageView = this->cubemap->view;
			imageInfo.sampler = this->sampler->sampler;
			write.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(
				VulkanUtils::utils().getCore()->device,
				1,
				&write,
				0,
				nullptr
			);
		}
	}

public:
	void initialize(RC<AsyncImageLoader> iLoader, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat) {
		this->createCubemap(iLoader);
		this->createSampler();
		this->createDescriptorSet();
		this->createPipeline(colorAttachmentFormat, depthAttachmentFormat);
	}

	void frameUpdate() {

	}

	void beginRender(
		VkCommandBuffer commandBuffer,
		VkViewport viewport,
		VkRect2D scissor
	) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.pipe);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		//descriptor set 0 should be bound after this
	}

	void endRender(
		VkCommandBuffer commandBuffer,
		uint32_t frameIndex,
		CubemapPushConstants inverseProjView
	) {
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS, draw.layout,
			0, 1, &perFrame[frameIndex].descriptor,
			0, nullptr
		);

		vkCmdPushConstants(
			commandBuffer,
			draw.layout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(inverseProjView),
			&inverseProjView
		);
		//TODO check
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	}

};