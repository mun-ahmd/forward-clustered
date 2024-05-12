#pragma once
#include "image.hpp"
#include "mesh.hpp"
#include "GLTFScene.hpp"

class ForwardRenderer{
public:
	RC<Image> gNormal;
	UniqueImageView gNormalView;

	RC<Image> depthImage;
	UniqueImageView depthImageView;

	RC<Image> colorOutputImage;
	UniqueImageView colorOutputView;

	struct {
		VkPipeline pipe;
		VkPipelineLayout layout;
	} pipeline;

private:
	VulkanCore core;

	VkRect2D renderArea{};
	VkViewport viewport{};
	VkRect2D scissor{};
	virtual void initRenderAreas(VkRect2D renderRegion)
	{
		//initializing render areas
		renderArea = renderRegion;

		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(renderRegion.extent.width);
		viewport.height = static_cast<float>(renderRegion.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		scissor.offset = { 0, 0 };
		scissor.extent = renderRegion.extent;
	}

	VkClearValue depthClearValue{};
	VkClearValue colorClearValue{};
	virtual void initClearColorValues() {
		//initializing clear colors
		depthClearValue.depthStencil = { 1.0f, 0 };
		colorClearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	}
	void initPipeline(VkPipelineLayout layout) {
		pipeline.layout = layout;

		auto vertShaderCode = VulkanUtils::utils().compileGlslToSpv("Shaders/triangle.vert", shaderc_shader_kind::shaderc_vertex_shader);
		auto fragShaderCode = VulkanUtils::utils().compileGlslToSpv("Shaders/triangle.frag", shaderc_shader_kind::shaderc_fragment_shader);

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

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		auto bindingDescription = VertexInputDescription::getBindingDescription();
		auto attributeDescriptions = VertexInputDescription::getAttributeDescriptions();
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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

		VkPipelineColorBlendAttachmentState colorBlendAttachments[2];
		{
			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;
			colorBlendAttachments[0] = colorBlendAttachment;
			colorBlendAttachments[1] = colorBlendAttachment;
		}

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 2;
		colorBlending.pAttachments = colorBlendAttachments;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencilCI{};
		depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCI.depthTestEnable = VK_TRUE;
		depthStencilCI.depthWriteEnable = VK_TRUE;
		depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
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

		VkPipelineRenderingCreateInfo renderingCreateInfo{};
		renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

		renderingCreateInfo.colorAttachmentCount = 2;
		VkFormat colorAttachmentFormats[2] = { this->colorOutputImage->format, this->gNormal->format };
		renderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats;
		renderingCreateInfo.depthAttachmentFormat = this->depthImage->format;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.pNext = &renderingCreateInfo;

		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencilCI;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = pipeline.layout;

		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.subpass = 1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		pipeline.pipe = core->createGraphicsPipeline(pipelineInfo);

		vkDestroyShaderModule(core->device, fragShaderModule, nullptr);
		vkDestroyShaderModule(core->device, vertShaderModule, nullptr);
	}
	void initResources() {
		VkExtent3D fullResolution{ renderArea.extent.width, renderArea.extent.height, 1 };

		auto gNormalCI = Image::makeCreateInfo(
			VK_FORMAT_A2R10G10B10_UNORM_PACK32,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			fullResolution
		);
		gNormalCI.tiling = VK_IMAGE_TILING_OPTIMAL;

		gNormal = Image::create(
			core,
			gNormalCI,
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
		);
		gNormalView = getImageView(gNormal, gNormal->format, VK_IMAGE_ASPECT_COLOR_BIT);

		colorOutputImage = Image::create(
			core,
			Image::makeCreateInfo(
				VK_FORMAT_R16G16B16A16_SFLOAT,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				fullResolution
			),
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
		);
		colorOutputView = getImageView(colorOutputImage, colorOutputImage->format, VK_IMAGE_ASPECT_COLOR_BIT);

		depthImage = Image::create(
			core,
			Image::makeCreateInfo(
				core->findDepthFormat(),
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				fullResolution
			),
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
		);
		depthImageView = getImageView(depthImage, depthImage->format, VK_IMAGE_ASPECT_DEPTH_BIT);
	}
	void init(VulkanCore core, VkRect2D renderRegion, VkPipelineLayout layout) {
		this->core = core;
		this->initRenderAreas(renderRegion);
		this->initClearColorValues();
		this->initResources();
		this->initPipeline(layout);
	}

public:
	ForwardRenderer(
		VulkanCore core,
		VkRect2D renderRegion,
		VkPipelineLayout layout
	) {
		init(core, renderRegion, layout);
	}

	static std::shared_ptr<ForwardRenderer> create(
		VulkanCore core,
		VkRect2D renderRegion,
		VkPipelineLayout layout
	) {
		return std::make_shared<ForwardRenderer>(core, renderRegion, layout);
	}

	VkPipelineLayout getPipelineLayout() {
		return this->pipeline.layout;
	}

	void beginRender(VkCommandBuffer commandBuffer)
	{
		//dynamic rendering forward rendering pass
			//color pass 
			//todo render normals and ambient

		VkRenderingAttachmentInfo depthAttachmentInfo{};
		depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depthAttachmentInfo.imageView = depthImageView->view;
		depthAttachmentInfo.clearValue = depthClearValue;

		std::array<VkRenderingAttachmentInfo, 2> colorAttachmentInfos{};
		{
			//color output
			VkRenderingAttachmentInfo colorAttachmentInfo{};
			colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachmentInfo.imageView = this->colorOutputView->view;
			colorAttachmentInfo.clearValue = colorClearValue;
			colorAttachmentInfos[0] = colorAttachmentInfo;
		}
		{
			//normal output
			VkRenderingAttachmentInfo colorAttachmentInfo{};
			colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachmentInfo.imageView = this->gNormalView->view;
			colorAttachmentInfo.clearValue = colorClearValue;
			colorAttachmentInfos[1] = colorAttachmentInfo;
		}

		VkRenderingInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderInfo.pDepthAttachment = &depthAttachmentInfo;
		renderInfo.pStencilAttachment = nullptr;
		renderInfo.colorAttachmentCount = colorAttachmentInfos.size();
		renderInfo.pColorAttachments = colorAttachmentInfos.data();

		renderInfo.layerCount = 1;
		renderInfo.viewMask = 0;
		renderInfo.renderArea = renderArea;

		renderInfo.pNext = nullptr;

		vkCmdBeginRendering(commandBuffer, &renderInfo);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipe);

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}
	
	void endRender(VkCommandBuffer commandBuffer) {
		vkCmdEndRendering(commandBuffer);
	}
};
