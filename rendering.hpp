#pragma once
#include "forwardRenderer.hpp"
#include "postProcessing.hpp"
#include "depthOnlyRenderer.hpp"


struct MeshPushConstants {
	glm::mat4 transform;
};

//the full renderer
class GraphicsRenderer {
	VulkanCore core;
	VkRect2D renderRegion;
	RC<ForwardRenderer> forward;
	RC<PostProcessRenderer> postProcess;

	VkPipelineLayout forwardPipelineLayout;
	void createForwardPipelineLayout() {
		VkPipelineLayoutCreateInfo plci{};
		plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		plci.pushConstantRangeCount = 1;
		VkPushConstantRange push_constant;
		push_constant.offset = 0;
		push_constant.size = sizeof(MeshPushConstants);
		push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	}

	VkDescriptorSet forwardDS;
	void createForwardDS() {
	
	}

	VkPipelineLayout postProcessPipelineLayout;
	void createPostProcessPipelineLayout() {
	
	}

	VkDescriptorSet postProcessDS;
	void createPostProcessDS() {
	
	}



public:
	void init(VulkanCore coreIN, VkRect2D renderRegionIN){
		core = coreIN;
		renderRegion = renderRegionIN;
		forward = ForwardRenderer::create(core, renderRegion, forwardPipelineLayout);
		postProcess = PostProcessRenderer::create(core, renderRegion, postProcessPipelineLayout);
	}
};