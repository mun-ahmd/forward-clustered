#pragma once
#include "core.hpp"

class RendererBase {
protected:
	VulkanCore core;

	virtual void initPipeline(VkPipelineLayout layout) = 0;
	
	virtual void initResources() = 0;

	virtual void initRenderAreas(VkRect2D renderRegion) = 0;

	virtual void initClearColorValues() = 0;

public:
	virtual void init(VulkanCore core, VkRect2D renderRegion, VkPipelineLayout layout) {
		this->core = core;
		this->initRenderAreas(renderRegion);
		this->initClearColorValues();
		this->initResources();
		this->initPipeline(layout);
	}

	virtual VkPipelineLayout getPipelineLayout() = 0;
	virtual void beginRender(VkCommandBuffer commandBuffer) = 0;
	virtual void endRender(VkCommandBuffer commandBuffer) = 0;
};