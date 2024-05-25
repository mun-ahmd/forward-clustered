#pragma once
#include <unordered_map>
#include <vector>
#include <string>

#include "core.hpp"

#include "RenderingCreateInfo.hpp"
#include "RenderingResources.hpp"

//TODO should replace this with an ECS like entt later
//A lot of the lookups are gonna be looking up for the same resource continously
//so it may be smart to introduce a cached version
template<typename T>
class IdMappedResource {
public:
	std::unordered_map<Rendering::ResourceID, T> store;

	//start from 1, 0 means null
	uint32_t currentID = 1;

	Rendering::ResourceID add(T value) {
		Rendering::ResourceID valueID = currentID;
		store[valueID] = value;
		currentID += 1;
		return valueID;
	}

	void remove(Rendering::ResourceID id) {
		store.erase(id);
	}

	bool has(Rendering::ResourceID id) {
		return (store.find(id) != store.end());
	}

	T get(Rendering::ResourceID id) {
		return store[id];
	}
};

//for now let swapchain use vkobject on its own without rendering server
//	only add a command "blitToSwapchain(image, areaInfo)" to the rendering server
//	good because it does not have to internally use blit cmd
//  (useful if the swapchain image does not support transfer dst)

class Swapchain {

};


/*
Good news:
	my current multiple frames in flight understanding was completely wrong
	Here is how it actually works (assuming 2 frames in flight):
		there are 3 stages:
			Command Recording	<- as many frames as you want can do this (ofcourse not simultaneously since that would be useless)
			GPU Execution		<- only one frame can do this at a time
			Presentation		<- only uses that frame's swapchain image

		SO, you want your frames to share the data: let us call these things common data
			(model matrices, material info, textures, lights, meshes, etc)

		What you do not want to happen is that one frame is executing
			and at the same time something else executes a command buffer
			writing to common data, most likely through a transfer operation
			How can this happen:
				1) the common data was memory mapped and written to from cpu
				2) a staging buffer was written to and then transferred to the common data
				3) an in use descriptor set was updated through vkUpdateDescriptorSet
					3.1) this is often done when you are binding different textures to a descriptor

		Possible solutions:
			1) Do all transfers within the frame's command buffer
			use appropriate pipeline barriers

				a) So give each frame a somewhat large shared buffer
				use this for all transfers required
				(ofc different areas of the buffer for each transfer
				otherwise you will overwrite on one location haha)

				b) Whenever a cpu <-> gpu transfer is needed create a new staging buffer

				c) When small memory transfer use the frame's shared buffer,
				for larger, more specific transfers maintain a dedicated staging buffer per frame
*/
/*
Idea:
	move all commands to be a member function of command buffer objects
	because of cool interfacing like: cmdBuf.drawObjects()
	Counter-argument:
		command functions take arguments like buffer and image resource ids
		these require the rendering server's data store to get the data from resource id

*/
class RenderingServer {
private:
	VulkanCore core;
	VkCommandPool commandPool;

	VkCommandBuffer activeCBuf;
	Rendering::ResourceID activeCBufResourceID;
	//todo add swapchain into this

	IdMappedResource<Rendering::Image> images;
	IdMappedResource<Rendering::ImageView> imageViews;
	IdMappedResource<Rendering::Buffer> buffers;
	IdMappedResource<Rendering::CommandBuffer> commandBuffers;
	IdMappedResource<VkShaderModule> shaderModules;
	IdMappedResource<Rendering::DescriptorSet> descriptorSets;
	IdMappedResource<VkPipelineLayout> pipelineLayouts;
	IdMappedResource<Rendering::Pipeline> pipelines;

public:
	Rendering::ResourceID createImage(Rendering::ImageCreateInfo createInfo);
	void cmdTransitionImageLayout(
		Rendering::ResourceID image,
		std::string aspect,
		std::string oldLayout,
		std::string newLayout
	);

	void transitionImageLayout(
		Rendering::ResourceID image,
		std::string aspect,
		std::string oldLayout,
		std::string newLayout
	);
	void destroyImage(Rendering::ResourceID image);

	//todo
	Rendering::ResourceID createImageView(
		Rendering::ResourceID image,
		Rendering::ImageViewCreateInfo createInfo
	);
	void destroyImageView(Rendering::ResourceID imageView);

	Rendering::ResourceID createBuffer(Rendering::BufferCreateInfo createInfo);
	void cmdCopyBuffer(Rendering::BufferCopyInfo copyInfo);
	void copyBuffer(Rendering::BufferCopyInfo copyInfo);
	void cmdCopyBufferIntoImage(
		Rendering::ResourceID buffer,
		Rendering::ResourceID image,
		Rendering::BufferToImageCopy copyInfo
	);
	void copyBufferIntoImage(
		Rendering::ResourceID buffer,
		Rendering::ResourceID image,
		Rendering::BufferToImageCopy copyInfo
	);
	//todo also find mechanism to write to buffers from lua
	//not urgent
	void destroyBuffer(Rendering::ResourceID buffer);

	Rendering::ResourceID createShaderModule(std::string shaderFile, std::string shaderType);
	void destroyShaderModule(Rendering::ResourceID module);

	//todo maybe also add descriptor pools here
	Rendering::ResourceID createDescriptorSet(Rendering::DescriptorSetCreateInfo info);

	Rendering::ResourceID createPipelineLayout(Rendering::PipelineLayoutCreateInfo info);
	Rendering::ResourceID createPipeline(Rendering::PipelineCreateInfo createInfo);
	void cmdUsePipeline(Rendering::ResourceID pipeline);
	void destroyPipeline(Rendering::ResourceID pipeline);

	//todo
	void beginRendering(Rendering::RenderingInfo info);
	void endRendering();

	Rendering::ResourceID createCommandBuffer();
	//makes this one the active commandbuffer, error if one is already going on
	void beginCommandBuffer(Rendering::ResourceID commandBuffer, bool isOneTimeSubmit);
	void endCommandBuffer();
	void destroyCommandBuffer(Rendering::ResourceID commandBuffer);

	//todo
	Rendering::ResourceID createFence(bool createSignalled);
	void destroyFence(Rendering::ResourceID fence);
	Rendering::ResourceID createSemaphore(bool createSignalled);
	void destroySemaphore(Rendering::ResourceID semaphore);
	void submitCommandBuffer(ResourceID commandBuffer, CommandBufferSubmitInfo submitInfo);

	Rendering::ResourceID createLight(Rendering::LightCreateInfo info);
	void updateLight(Rendering::ResourceID light, Rendering::LightProperties props);
	void destroyLight(Rendering::ResourceID light);

	Rendering::ResourceID createMaterial(Rendering::MaterialCreateInfo info);
	void destroyMaterial(Rendering::ResourceID material);

	Rendering::ResourceID createMesh(Rendering::MeshCreateInfo info);
	void destroyMesh(Rendering::ResourceID mesh);

	Rendering::ResourceID createRenderedInstance(Rendering::RenderedInstanceCreateInfo info);
	void updateRenderedInstance(Rendering::ResourceID instance, Rendering::InstanceInfo info);
	void destroyRenderedInstance(Rendering::ResourceID instance);
};