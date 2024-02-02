#pragma once
#include "core.hpp"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

#include "storage_helper.hpp"

#include <filesystem>
namespace fs = std::filesystem;


class MyImgui {
private:
	void VK_CHECK(VkResult result) {
		if (result != VK_SUCCESS) {
			throw std::exception("Vulkan command unsuccesful - imgui helper");
		}
	}
	VkPipelineCache imguiPipelineCache;
	VkDescriptorPool imguiPool;
	VulkanCore core;
public:
	void init(VulkanCore core, VkRenderPass renderPass, VkCommandBuffer cmd, int subpass = 0){
		this->core = core;
		//1: create descriptor pool for IMGUI
		// the size of the pool is very oversize, but it's copied from imgui demo itself.
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		this->VK_CHECK(vkCreateDescriptorPool(core->device, &pool_info, nullptr, &this->imguiPool));


		VkPipelineCacheCreateInfo pipeCacheCreateInfo{};
		pipeCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		pipeCacheCreateInfo.initialDataSize = 0;
		this->VK_CHECK(vkCreatePipelineCache(core->device, &pipeCacheCreateInfo, nullptr, &this->imguiPipelineCache));


		// 2: initialize imgui library

		//this initializes the core structures of imgui
		ImGui::CreateContext();

		//this initializes imgui for SDL
		ImGui_ImplGlfw_InitForVulkan(core->window, true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = core->instance;
		init_info.PhysicalDevice = core->physicalDevice;
		init_info.Device = core->device;
		init_info.QueueFamily = core->findQueueFamilies().graphicsFamily.value();
		init_info.Queue = core->graphicsQueue;
		init_info.PipelineCache = this->imguiPipelineCache;
		init_info.DescriptorPool = this->imguiPool;
		init_info.Subpass = subpass;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		//this initializes imgui for Vulkan
		ImGui_ImplVulkan_Init(&init_info, renderPass);

		//execute a gpu command to upload imgui font textures

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		this->VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

		ImGui_ImplVulkan_CreateFontsTexture(cmd);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &cmd;

		this->VK_CHECK(vkEndCommandBuffer(cmd));
		vkQueueSubmit(core->graphicsQueue, 1, &end_info, VK_NULL_HANDLE);

		vkDeviceWaitIdle(core->device);

		//clear font textures from cpu data
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void newFrame() {
		//imgui new frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void drawWithinRenderPass(VkCommandBuffer commandBuf) {
		//Call at end of renderpass
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuf);
	}

	void destroy() {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		vkDestroyPipelineCache(core->device, this->imguiPipelineCache, nullptr);
		vkDestroyDescriptorPool(core->device, this->imguiPool, nullptr);
		this->core.reset();
	}

};

struct GLTFModelSelector {
	constexpr inline static const char* baseDirectory = "3DModels";

	std::vector<std::string> discoveredGLTFModelPaths;
	std::string loadedModelPath = "3DModels/main_sponza/Main.1_Sponza/NewSponza_Main_glTF_002.gltf";

	GLTFModelSelector() {
		//discover possible model paths
		std::vector<std::string> directoryQueue = { baseDirectory };
		int directoryQueueIndex = 0;
		try {
			while (directoryQueueIndex < directoryQueue.size()) {
				for (const auto& entry : fs::directory_iterator(directoryQueue[directoryQueueIndex])) {
					if (
						fs::is_regular_file(entry) &&
						(entry.path().extension() == ".gltf")
						) {
						std::cout << "Found .gltf file: " << entry.path() << std::endl;
						discoveredGLTFModelPaths.push_back(entry.path().generic_string());
					}
					else if (fs::is_directory(entry)) {
						directoryQueue.push_back(entry.path().generic_string());
					}
				}
				directoryQueueIndex++;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << std::endl;
		}

		if (Store::itemInStore("lastLoadedModel")) {
			auto bytes = Store::fetchBytes("lastLoadedModel");
			this->loadedModelPath = std::string(bytes.get());
		}
	}

	void render(std::function<void()> ifFileSelectedCallback) {
		// Display menu items
		for (int i = 0; i < discoveredGLTFModelPaths.size(); i++) {
			ImGui::PushID(i); // Ensure each menu item has a unique ID
			if (ImGui::Button(discoveredGLTFModelPaths[i].c_str())) {
				printf("Item %d clicked!\n", i);
				this->loadedModelPath = discoveredGLTFModelPaths[i];
				Store::storeBytes("lastLoadedModel", this->loadedModelPath.c_str(), this->loadedModelPath.size() + 1);
				ifFileSelectedCallback();
			}
			ImGui::PopID();
		}
	}

} gltfModelSelector;