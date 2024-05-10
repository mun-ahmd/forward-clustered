#pragma once
#include "image.hpp"
#include "mesh.hpp"
#include "GLTFScene.hpp"

class DepthOnlyRenderer{
public:
	struct {
		VkPipeline pipe;
		VkPipelineLayout layout;
	} pipeline;

private:
	VkFormat outputDepthImageFormat;
	
	VulkanCore core;

	VkRect2D renderArea{};
	VkViewport viewport{};
	VkRect2D scissor{};
	void initRenderAreas(VkRect2D renderRegion)
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
	void initClearColorValues() {
		//initializing clear colors
		depthClearValue.depthStencil = { 1.0f, 0 };
		colorClearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	}
	void initPipeline(VkPipelineLayout layout) {
		pipeline.layout = layout;

		//pipeline creation
		auto vertShaderCode = VulkanUtils::utils().compileGlslToSpv("Shaders/prePass.vert", shaderc_shader_kind::shaderc_vertex_shader);
		auto fragShaderCode = VulkanUtils::utils().compileGlslToSpv("Shaders/prePass.frag", shaderc_shader_kind::shaderc_fragment_shader);

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
		//only taking position attribute
		auto attributeDescriptions = VertexInputDescription::getAttributeDescriptions()[0];
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = &attributeDescriptions;

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

		renderingCreateInfo.colorAttachmentCount = 0;
		renderingCreateInfo.pColorAttachmentFormats = nullptr;
		renderingCreateInfo.depthAttachmentFormat = this->outputDepthImageFormat;

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
		pipelineInfo.pColorBlendState = nullptr;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = pipeline.layout;

		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		pipeline.pipe = core->createGraphicsPipeline(pipelineInfo);

		vkDestroyShaderModule(core->device, fragShaderModule, nullptr);
		vkDestroyShaderModule(core->device, vertShaderModule, nullptr);
	}
	void initResources() {}
	
	void init(VulkanCore core, VkRect2D renderRegion, VkPipelineLayout layout) {
		this->core = core;
		this->initRenderAreas(renderRegion);
		this->initClearColorValues();
		this->initResources();
		this->initPipeline(layout);
	}

public:
	DepthOnlyRenderer(
		VulkanCore core,
		VkRect2D renderRegion,
		VkPipelineLayout layout,
		VkFormat outputDepthImageFormat
	) : outputDepthImageFormat(outputDepthImageFormat) {
		init(core, renderRegion, layout);
	}

	static std::shared_ptr<DepthOnlyRenderer> create(
		VulkanCore core,
		VkRect2D renderRegion,
		VkPipelineLayout layout,
		VkFormat outputDepthImageFormat
	) {
		return std::make_shared<DepthOnlyRenderer>(
			core, renderRegion, layout, outputDepthImageFormat
		);
	}

	VkPipelineLayout getPipelineLayout() {
		return this->pipeline.layout;
	}

	void beginRender(
		VkCommandBuffer commandBuffer,
		VkImageView outputDepthImageView
	)
	{
		VkRenderingAttachmentInfo depthAttachmentInfo{};
		depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depthAttachmentInfo.imageView = outputDepthImageView;
		depthAttachmentInfo.clearValue = depthClearValue;

		VkRenderingInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderInfo.pDepthAttachment = &depthAttachmentInfo;
		renderInfo.pStencilAttachment = nullptr;
		renderInfo.colorAttachmentCount = 0;
		renderInfo.pColorAttachments = nullptr;

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