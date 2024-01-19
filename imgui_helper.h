#pragma once
#include "core.hpp"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"


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
	bool my_tool_active;
	float my_color[4] = { 0.2, 0.3, 0.85, 1.0 };
	std::vector<std::string> textData;

	void test() {
		// Create a window called "My First Tool", with a menu bar.
		ImGui::Begin("My First Tool", &my_tool_active, ImGuiWindowFlags_MenuBar);
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
				if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
				if (ImGui::MenuItem("Close", "Ctrl+W")) { my_tool_active = false; }
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		// Edit a color (stored as ~4 floats)
		ImGui::ColorEdit4("Color", my_color);

		// Plot some values
		const float my_values[] = { 0.2f, 0.1f, 1.0f, 0.5f, 0.9f, 2.2f };
		ImGui::PlotLines("Frame Times", my_values, IM_ARRAYSIZE(my_values));

		// Display contents in a scrolling region
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Important Stuff");
		ImGui::BeginChild("Scrolling");
		for (auto& text : textData) {
			ImGui::Text(text.c_str());
		}
		ImGui::Text("Some text");
		ImGui::EndChild();
		ImGui::End();
	}

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