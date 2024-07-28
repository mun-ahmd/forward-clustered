#pragma once
#include <unordered_map>
#include <vector>
#include <string>

#include <lua.hpp>
#include <sol/sol.hpp>

#include "core.hpp"

#include "RenderingCreateInfo.hpp"
#include "RenderingResources.hpp"

// to define anything as : export this to lua
// name should be a string literal 

//TODO should replace this with an ECS like entt later
//A lot of the lookups are gonna be looking up for the same resource continously
//so it may be smart to introduce a cached version
template<typename T>
class IdMappedResource {
public:
	std::unordered_map<Rendering::ResourceID, T> store;

	constexpr static unsigned int ID_TAG_BITS = 4;
	// 4 bit customizable tag 28 bit id
	// 0 means null
	uint32_t currentID = 1;

	inline static uint8_t getTag(Rendering::ResourceID id) {
		return (id >> (32 - ID_TAG_BITS));
	}

	inline static uint32_t getBaseID(Rendering::ResourceID id) {
		return (id & (0xFFFFFFFF >> ID_TAG_BITS));
	}

	inline  Rendering::ResourceID genNextID(uint8_t tag) {
		assert( (currentID >> (32 - ID_TAG_BITS)) == 0 && "Ran out of ids to map to resources");
		uint32_t result = (tag << (32 - ID_TAG_BITS)) | (currentID & (0xFFFFFFFF >> ID_TAG_BITS));
		currentID += 1;
		return result;
	}

	Rendering::ResourceID add(T value, uint8_t tag) {
		Rendering::ResourceID valueID = genNextID(tag);
		store[valueID] = value;
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

	std::vector<T> getMany(std::vector<Rendering::ResourceID> ids) {
		std::vector<T> data;
		data.reserve(ids.size());
		for (Rendering::ResourceID id : ids) {
			data.push_back(this->get(id));
		}
		return data;
	}
};

class MultiIdMappedResources {
private:
	typedef decltype(((type_info*)nullptr)->hash_code()) __TYPEID__;
	
	template<typename T>
	inline __TYPEID__ getTypeIdentifier() {
		return typeid(T).hash_code();
	}

	std::unordered_map<__TYPEID__, std::shared_ptr<void>> resources;

	uint8_t activeResourceTag = 0;
public:
	
	void setActiveResourceTag(uint8_t tag) {
		this->activeResourceTag = tag;
	}

	template<typename T>
	void addResourceType() {
		__TYPEID__ typeIdentifier = getTypeIdentifier<T>();
		assert(this->resources.find(typeIdentifier) == this->resources.end());
		this->resources[typeIdentifier] = std::make_shared<IdMappedResource<T>>();
	}

	template<typename T>
	std::shared_ptr<IdMappedResource<T>> getResourceStore() {
		assert(this->resources.find(getTypeIdentifier<T>()) != this->resources.end());
		std::shared_ptr<IdMappedResource<T>> resourceStore =
			std::static_pointer_cast<IdMappedResource<T>>(
				this->resources[getTypeIdentifier<T>()]
			);
		return resourceStore;
	}

	template<typename T>
	Rendering::ResourceID add(T value) {
		return this->getResourceStore<T>()->add(value, activeResourceTag);
	}

	template<typename T>
	bool has(Rendering::ResourceID id) {
		return this->getResourceStore<T>()->has(id);
	}

	template<typename T>
	T get(Rendering::ResourceID id) {
		return this->getResourceStore<T>()->get(id);
	}

	template<typename T>
	std::optional<T> getIfExists(Rendering::ResourceID id) {
		if (this->has<T>(id)) {
			return this->get<T>(id);
		}
		else {
			return {};
		}
	}

	template<typename T>
	void remove(Rendering::ResourceID id) {
		this->getResourceStore<T>()->remove(id);
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

	//todo problematic since managing tags is not automatic
	//inline std::shared_ptr<IdMappedResource<Rendering::Image>> images(){
	//	return this->resources.getResourceStore<Rendering::Image>();
	//}
	//inline std::shared_ptr<IdMappedResource<Rendering::ImageView>> imageViews() {
	//	return this->resources.getResourceStore<Rendering::ImageView>();
	//}
	//inline std::shared_ptr<IdMappedResource<Rendering::Buffer>> buffers() {
	//	return this->resources.getResourceStore<Rendering::Buffer>();
	//}
	//inline std::shared_ptr<IdMappedResource<Rendering::CommandBuffer>> commandBuffers() {
	//	return this->resources.getResourceStore<Rendering::CommandBuffer>();
	//}
	//inline std::shared_ptr<IdMappedResource<Rendering::ShaderModule>> shaderModules() {
	//	return this->resources.getResourceStore<Rendering::ShaderModule>();
	//}
	//inline std::shared_ptr<IdMappedResource<Rendering::DescriptorPool>> descriptorPools() {
	//	return this->resources.getResourceStore<Rendering::DescriptorPool>();
	//}
	//inline std::shared_ptr<IdMappedResource<Rendering::DescriptorSet>> descriptorSets() {
	//	return this->resources.getResourceStore<Rendering::DescriptorSet>();
	//}
	//inline std::shared_ptr<IdMappedResource<Rendering::PipelineLayout>> pipelineLayouts() {
	//	return this->resources.getResourceStore<Rendering::PipelineLayout>();
	//}
	//inline std::shared_ptr<IdMappedResource<Rendering::Pipeline>> pipelines() {
	//	return this->resources.getResourceStore<Rendering::Pipeline>();
	//}
	//inline std::shared_ptr<IdMappedResource<Rendering::Semaphore>> semaphores() {
	//	return this->resources.getResourceStore<Rendering::Semaphore>();
	//}
	//inline std::shared_ptr<IdMappedResource<Rendering::Fence>> fences() {
	//	return this->resources.getResourceStore<Rendering::Fence>();
	//}
	
	MultiIdMappedResources resources;
	void registerResourceTypes() {
		resources.addResourceType<Rendering::Image>();
		resources.addResourceType<Rendering::ImageView>();
		resources.addResourceType<Rendering::Buffer>();
		resources.addResourceType<Rendering::Sampler>();
		resources.addResourceType<Rendering::CommandBuffer>();
		resources.addResourceType<Rendering::ShaderModule>();
		resources.addResourceType<Rendering::DescriptorPool>();
		resources.addResourceType<Rendering::DescriptorSet>();
		resources.addResourceType<Rendering::PipelineLayout>();
		resources.addResourceType<Rendering::Pipeline>();
		resources.addResourceType<Rendering::Semaphore>();
		resources.addResourceType<Rendering::Fence>();
	}

public:
	enum class ResourceUser : uint8_t {
		NONE = 0,
		LUA = 1,
		SCENE = 2,
		OTHER = 3
	};

private:
	RenderingServer::ResourceUser activeResourceUser;
	void setActiveTagForAll(uint8_t tag) {
		activeResourceUser = static_cast<RenderingServer::ResourceUser>(tag);
		this->resources.setActiveResourceTag(tag);
	}

	struct {
		std::string scriptPath;
		sol::state state;
		sol::table rendering;
		sol::load_result load;
		bool initialized = false;
	} lua;
	
	void registerRenderingBindings();
	

public:
	ResourceUser activeUser = ResourceUser::NONE;

	void setOperatingUser(RenderingServer::ResourceUser activeUser) {
		this->setActiveTagForAll(static_cast<uint8_t>(activeUser));
	}

	void registerRenderingScript(std::string scriptPath) {
		//lua script
		if (lua.initialized) {
			//warn about changing the script
			std::cout <<
				"Changing script path from \"" <<
				lua.scriptPath <<
				"\" to \"" <<
				scriptPath <<
				"\"" <<
				std::endl;
		}
		lua.initialized = true;
		lua.scriptPath = scriptPath;

		lua.state = sol::state();
		this->registerRenderingBindings();
		lua.load = lua.state.load_file(scriptPath);
	}

	void executeRenderingScript() {
		sol::protected_function_result renderingResult = lua.load.call();
		if (!renderingResult.valid()) {
			//handle error
			std::cerr <<
				"Error while executing rendering script: \"" <<
				lua.scriptPath <<
				"\"" <<
				std::endl;

			std::cerr <<
				(static_cast<sol::error>(renderingResult).what()) <<
				std::endl;
		}
	}

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
	//eg.) void writeToBuffer(xyz);

	void destroyBuffer(Rendering::ResourceID buffer);

	Rendering::ResourceID createSampler(Rendering::SamplerCreateInfo info);
	void destroySampler(Rendering::ResourceID sampler);

	Rendering::ResourceID createShaderModule(std::string shaderFile, std::string shaderType);
	void destroyShaderModule(Rendering::ResourceID module);

	Rendering::ResourceID createDescriptorPool(Rendering::DescriptorPoolCreateInfo info);
	Rendering::ResourceID createDescriptorSet(Rendering::DescriptorSetCreateInfo info);
	
	void writeBufferToDescriptorSet(
		Rendering::ResourceID descriptorSet,
		uint32_t dstBinding,
		uint32_t dstArrayElement,
		Rendering::ResourceID buffer,
		uint32_t offset,
		uint32_t range,
		bool isUniform,
		bool isDynamic
	);
	void writeImageToDescriptorSet(
		Rendering::ResourceID descriptorSet,
		uint32_t dstBinding,
		uint32_t dstArrayElement,
		Rendering::ResourceID imageView,
		std::string imageLayout,
		bool isSampledNotStorage
	);
	void writeSamplerToDescriptorSet(
		Rendering::ResourceID descriptorSet,
		uint32_t dstBinding,
		uint32_t dstArrayElement,
		Rendering::ResourceID sampler
	);
	void writeCombinedImageSamplerToDescriptorSet(
		Rendering::ResourceID descriptorSet,
		uint32_t dstBinding,
		uint32_t dstArrayElement,
		Rendering::ResourceID imageView,
		Rendering::ResourceID sampler,
		std::string imageLayout
	);

	Rendering::ResourceID createPipelineLayout(Rendering::PipelineLayoutCreateInfo info);
	Rendering::ResourceID createPipeline(Rendering::PipelineCreateInfo createInfo);
	void cmdUsePipeline(Rendering::ResourceID pipeline);
	void destroyPipeline(Rendering::ResourceID pipeline);

	void beginRendering(Rendering::RenderingInfo info);
	void endRendering();

	void setActiveViewport(uint32_t index, Rendering::ViewportInfo info);
	void setActiveScissor(uint32_t index, Rendering::Rect2D info);


	Rendering::ResourceID createCommandBuffer();
	//makes this one the active commandbuffer, error if one is already going on
	void beginCommandBuffer(Rendering::ResourceID commandBuffer, bool isOneTimeSubmit);
	void endCommandBuffer();
	void destroyCommandBuffer(Rendering::ResourceID commandBuffer);

	Rendering::ResourceID createFence(bool createSignalled);
	void destroyFence(Rendering::ResourceID fence);
	Rendering::ResourceID createSemaphore();
	void destroySemaphore(Rendering::ResourceID semaphore);
	void submitCommandBuffer(Rendering::ResourceID commandBuffer, Rendering::CommandBufferSubmitInfo submitInfo);

	//todo
	// move all of these to a different class which uses the RenderingServer api
	// to manage all of the scene resources
	// scene resources should be initialized and managed by a different script
	// these resources will use a different tag than the rendering script resources
	// this will allow for faster reloading when only the rendering script is changed
	// as the scene will not be reloaded which takes by far the most time to do along with vulkan core initialization
	//
	//		to be done after testing of everything else is done

	//Rendering::ResourceID createLight(Rendering::LightCreateInfo info);
	//void updateLight(Rendering::ResourceID light, Rendering::LightProperties props);
	//void destroyLight(Rendering::ResourceID light);

	//Rendering::ResourceID createMaterial(Rendering::MaterialCreateInfo info);
	//void destroyMaterial(Rendering::ResourceID material);

	//Rendering::ResourceID createMesh(Rendering::MeshCreateInfo info);
	//void destroyMesh(Rendering::ResourceID mesh);

	//Rendering::ResourceID createRenderedInstance(Rendering::RenderedInstanceCreateInfo info);
	//void updateRenderedInstance(Rendering::ResourceID instance, Rendering::InstanceInfo info);
	//void destroyRenderedInstance(Rendering::ResourceID instance);
};