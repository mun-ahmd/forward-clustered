#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <shaderc/shaderc.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include <iostream>
#include <vector>
#include <array>
#include <set>
#include <optional>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <functional>

#include "MeshLoader.h"
#include "cameraObj.h"

#include "core.hpp"
#include "resources.hpp"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

//RC = Reference Counted
template<typename T>
using RC = std::shared_ptr<T>;

class Frame {
private:
	//only write to below using memcpy
	void* matricesMappedPointer;

	void initializeMatricesBuffer() {
		this->matricesBuffer = Buffer::create(core, sizeof(globalDescriptor), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VmaAllocationCreateFlagBits)(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), 0);
		matricesMappedPointer = this->matricesBuffer->allocation->GetMappedData();
	}

	void createGlobalDescriptorSet() {

		initializeMatricesBuffer();

		VkDescriptorSetLayoutBinding binding0{};
		binding0.descriptorCount = 1;
		binding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding0.stageFlags = VK_SHADER_STAGE_ALL;
		binding0.binding = 0;
		this->globalDS = core->createDescriptorSet({ binding0 });

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = binding0.descriptorType;
		write.dstSet = this->globalDS;
		write.dstBinding = binding0.binding;
		write.pNext = nullptr;
		VkDescriptorBufferInfo info{};
		info.buffer = this->matricesBuffer->buffer;
		info.offset = 0;
		info.range = sizeof(globalDescriptor);
		write.pBufferInfo = &info;

		vkUpdateDescriptorSets(core->device, 1, &write, 0, nullptr);
	}

	void cleanupGlobalDescriptorSet() {
	}

public:
	struct globalDescriptor {
		glm::mat4 proj;
		glm::mat4 view;
		glm::mat4 projView;

		void setView(glm::mat4& view) {
			this->view = view;
			this->projView = proj * view;
		}
	};


	VulkanCore core;

	VkCommandPool commandBufferPool;
	VkCommandBuffer commandBuffer;

	VkFence inFlightFence;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	//if it does not have a value then frame is not in flight
	std::optional<uint32_t> imageIndex;

	RC<Buffer> matricesBuffer;
	VkDescriptorSet globalDS;

	Frame() = default;

	Frame(VulkanCore core, VkCommandPool commandPool) : core(core), commandBufferPool(commandPool) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandBufferPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		commandBuffer = core->createCommandBuffer(allocInfo);
		createGlobalDescriptorSet();

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		imageAvailableSemaphore = core->createSemaphore(semaphoreInfo);
		renderFinishedSemaphore = core->createSemaphore(semaphoreInfo);
		inFlightFence = core->createFence(fenceInfo);
	}

	void beginFrame(SwapChain& swapChain, globalDescriptor value) {
		memcpy(this->matricesMappedPointer, &value, sizeof(globalDescriptor));
		vkWaitForFences(core->device, 1, &this->inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		imageIndex.reset();
		try {
			imageIndex = swapChain.acquireNextImage(core->device, imageAvailableSemaphore);
		}
		catch (const OutOfDateSwapChainError& err) {
			throw err;
		}

		vkResetFences(core->device, 1, &inFlightFence);

		vkResetCommandBuffer(commandBuffer, 0);
	}

	void endFrame(SwapChain& swapChain) {
		if (!imageIndex.has_value()) {
			//no frame in flight
			throw std::runtime_error("No Frame in Flight!");
		}
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(core->graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		swapChain.present(core->presentQueue, signalSemaphores, imageIndex.value());
	}

	void drawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

	}

	void cleanup(const VkDevice& device) {
		cleanupGlobalDescriptorSet();
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroyFence(device, inFlightFence, nullptr);
		vkFreeCommandBuffers(device, commandBufferPool, 1, &commandBuffer);
	}
};

class HelloTriangleApplication {
public:
	void run() {
		initVulkan();
		mainLoop();
	}

	~HelloTriangleApplication() {
		meshes.clear();
		meshes.shrink_to_fit();

		for (auto& frame : frames)
			frame.cleanup(core->device);

		vkDestroyCommandPool(core->device, commandPool, nullptr);

		swapChain.cleanupFramebuffers(core->device);
		vkDestroyPipeline(core->device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(core->device, pipelineLayout, nullptr);
		vkDestroyRenderPass(core->device, renderPass, nullptr);

		swapChain.cleanupSwapChain(core->device);



	}

private:
	VulkanCore core;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	SwapChain swapChain;

	VkCommandPool commandPool;

	std::array<Frame, MAX_FRAMES_IN_FLIGHT> frames;
	uint32_t current_frame = 0;

	shaderc::Compiler compiler = shaderc::Compiler();

	std::vector<Mesh> meshes;
	std::vector<RC<Material>> materials;
	std::vector<uint32_t> meshMatIndices;
	std::vector<glm::mat4> transforms;

	std::vector<PointLight> pointLights;

	VkDescriptorSet materialsDescriptorSet;

	//RC<Buffer> buffer;

	CamHandler camera;

	void initMeshesMaterials() {
		camera = CamHandler(core->window);

		auto loadedModel = loadGLTF("3DModels/cubeScene/cube.gltf").value();
		meshes.reserve(loadedModel.meshData.meshes.size());
		transforms = loadedModel.meshData.transforms;
		materials.reserve(loadedModel.meshData.meshes.size());

		for (auto& matIndex : loadedModel.meshData.matIndex) {
			matIndex = materials.size();
			materials.push_back(Material::create(core, commandPool, randomMaterial()));
		}
		meshMatIndices = std::vector<uint32_t>(
			loadedModel.meshData.matIndex.begin(),
			loadedModel.meshData.matIndex.end()
		);
		for (size_t i = 0; i < loadedModel.meshData.meshes.size(); ++i)
			meshes.push_back(Mesh(core, commandPool, loadedModel.meshData.meshes[i]));
	}

	void initVulkan() {
		core = createVulkanCore();
		createCommandPool();
		for (auto& frame : frames)
			frame = Frame(core, commandPool);
		auto swapCapabilities = core->querySwapChainSupport();
		auto swapChainFormat = chooseSwapSurfaceFormat(swapCapabilities.formats);
		auto swapPresentMode = chooseSwapPresentMode(swapCapabilities.presentModes);
		Material::init(core);
		materialsDescriptorSet = Material::createDescriptor(core);
		createRenderPass(swapChainFormat.format);
		createGraphicsPipeline();
		swapChain = core->createSwapChain(swapChainFormat, swapPresentMode, chooseSwapExtent(swapCapabilities.capabilities), renderPass);
		initMeshesMaterials();
	}

	void mainLoop() {
		//make a model view matrix for rendering the object
		glm::vec3 camPos = { 0.f,0.f,-1.f };
		//camera position
		glm::mat4 view = camera.getView();
		//camera projection
		glm::mat4 projection = glm::perspective(glm::radians(70.f), static_cast<float>(swapChain.swapChainExtent.width) / swapChain.swapChainExtent.height, 0.1f, 200.0f);

		Frame::globalDescriptor gDescValue{};
		gDescValue.proj = projection;

		double startTime = glfwGetTime();
		while (!glfwWindowShouldClose(core->window)) {
			if (glfwGetKey(core->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				glfwSetWindowShouldClose(core->window, GLFW_TRUE);
				continue;
			}
			try {
				double deltaTime = glfwGetTime() - startTime;
				startTime = glfwGetTime();
				camera.moveAround(deltaTime);
				camera.lookAround();

				Frame& activeFrame = frames[current_frame];

				view = camera.getView();
				gDescValue.setView(view);
				activeFrame.beginFrame(swapChain, gDescValue);
				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0; // Optional
				beginInfo.pInheritanceInfo = nullptr; // Optional

				if (vkBeginCommandBuffer(activeFrame.commandBuffer, &beginInfo) != VK_SUCCESS) {
					throw std::runtime_error("failed to begin recording command buffer!");
				}

				VkRenderPassBeginInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.renderPass = renderPass;
				renderPassInfo.framebuffer = swapChain.swapChainFramebuffers[activeFrame.imageIndex.value()];

				renderPassInfo.renderArea.offset = { 0, 0 };
				renderPassInfo.renderArea.extent = swapChain.swapChainExtent;

				VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
				renderPassInfo.clearValueCount = 1;
				renderPassInfo.pClearValues = &clearColor;

				vkCmdBeginRenderPass(activeFrame.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(activeFrame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

				vkCmdBindDescriptorSets(activeFrame.commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
					0, 1, &activeFrame.globalDS,
					0, nullptr
				);

				VkViewport viewport{};
				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = static_cast<float>(swapChain.swapChainExtent.width);
				viewport.height = static_cast<float>(swapChain.swapChainExtent.height);
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				vkCmdSetViewport(activeFrame.commandBuffer, 0, 1, &viewport);

				VkRect2D scissor{};
				scissor.offset = { 0, 0 };
				scissor.extent = swapChain.swapChainExtent;
				vkCmdSetScissor(activeFrame.commandBuffer, 0, 1, &scissor);

				//make a model view matrix for rendering the object
				//camera position

				VkDeviceSize offset = 0;
				for (int i = 0; i < meshes.size(); ++i) {
					uint32_t dynamicOffset = materials[meshMatIndices[i]]->getResourceOffset();
					vkCmdBindDescriptorSets(
						activeFrame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineLayout, 1, 1, &materialsDescriptorSet, 1, &dynamicOffset
					);

					MeshPushConstants constants{ transforms[i] };
					vkCmdPushConstants(activeFrame.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

					vkCmdBindVertexBuffers(activeFrame.commandBuffer, 0, 1, &meshes[i].vertexBuffer->buffer, &offset);
					vkCmdBindIndexBuffer(activeFrame.commandBuffer, meshes[i].indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT16);
					vkCmdDrawIndexed(activeFrame.commandBuffer, meshes[i].numIndices, 1, 0, 0, 0);
				}

				vkCmdEndRenderPass(activeFrame.commandBuffer);

				if (vkEndCommandBuffer(activeFrame.commandBuffer) != VK_SUCCESS) {
					throw std::runtime_error("failed to record command buffer!");
				}

				activeFrame.endFrame(swapChain);

				current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
				glfwPollEvents();
			}
			catch (const OutOfDateSwapChainError& err) {
				recreateSwapChain();
				std::cout << "Recreated swapchain" << std::endl;
			}
		}
		vkDeviceWaitIdle(core->device);
	}

	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = core->findQueueFamilies();

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		commandPool = core->createCommandPool(poolInfo);
	}

	void createRenderPass(VkFormat swapChainFormat) {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		renderPass = core->createRenderPass(renderPassInfo);
	}

	void createGraphicsPipeline() {
		auto vertShaderCode = compileGlslToSpv("triangleVert.glsl", shaderc_shader_kind::shaderc_vertex_shader);
		auto fragShaderCode = compileGlslToSpv("triangleFrag.glsl", shaderc_shader_kind::shaderc_fragment_shader);

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

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
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		//setup push constants
		VkPushConstantRange push_constant;
		//this push constant range starts at the beginning
		push_constant.offset = 0;
		//this push constant range takes up the size of a MeshPushConstants struct
		push_constant.size = sizeof(MeshPushConstants);
		//this push constant range is accessible only in the vertex shader
		push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 2;
		VkDescriptorSetLayout layouts[2] = {
			core->getLayout(frames.front().globalDS),
			core->getLayout(this->materialsDescriptorSet)
		};
		pipelineLayoutInfo.pSetLayouts = layouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &push_constant;

		pipelineLayout = core->createPipelineLayout(pipelineLayoutInfo);

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = pipelineLayout;

		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		graphicsPipeline = core->createGraphicsPipeline(pipelineInfo);

		vkDestroyShaderModule(core->device, fragShaderModule, nullptr);
		vkDestroyShaderModule(core->device, vertShaderModule, nullptr);
	}

	VkShaderModule createShaderModule(const std::vector<uint32_t>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size() * sizeof(uint32_t);
		createInfo.pCode = code.data();

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(core->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, core->surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, core->surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, core->surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, core->surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, core->surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(core->window, &width, &height);
			VkExtent2D actualExtent = VkExtent2D{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};
			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void recreateSwapChain() {
		int width, height;
		glfwGetFramebufferSize(core->window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(core->window, &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(core->device);

		auto oldSwapChainFormat = this->swapChain.swapChainImageFormat;
		this->swapChain.cleanupFramebuffers(core->device);
		this->swapChain.cleanupSwapChain(core->device);

		auto swapCapabilities = querySwapChainSupport(core->physicalDevice);
		auto swapChainFormat = chooseSwapSurfaceFormat(swapCapabilities.formats);
		auto swapPresentMode = chooseSwapPresentMode(swapCapabilities.presentModes);

		if (oldSwapChainFormat != swapChainFormat.format) {
			//Recreate renderpass
		}
		swapChain = core->createSwapChain(swapChainFormat, swapPresentMode, chooseSwapExtent(swapCapabilities.capabilities), renderPass);
	}

	std::string readFile(std::string filePath) {
		std::ifstream file;
		std::string text;
		file.open(filePath);
		if (!file.is_open())
			throw std::runtime_error("Error opening file: " + filePath);
		file.seekg(0, std::ios::end);
		text.reserve(file.tellg());
		file.seekg(0, std::ios::beg);
		text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		return text;
	}

	std::vector<uint32_t> compileGlslToSpv(std::string sourceFile, shaderc_shader_kind shaderKind) {
		std::string sourceCode = readFile(sourceFile);
		shaderc::CompileOptions options;
		auto compilationResults = compiler.CompileGlslToSpv(sourceCode, shaderKind, sourceFile.c_str());
		if (compilationResults.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success) {
			std::cerr << "Error Compiling " << sourceFile << " with:\n\t" << compilationResults.GetNumErrors() << " errors\n\t" << compilationResults.GetNumWarnings() << " warnings" << std::endl;
			std::cerr << compilationResults.GetErrorMessage();
			throw std::runtime_error("Error Compiling " + sourceFile);
		}
		std::cout << "Successfully Compiled " << sourceFile << " with:\n\t" << compilationResults.GetNumErrors() << " errors\n\t" << compilationResults.GetNumWarnings() << " warnings" << std::endl;
		return std::vector<uint32_t>(compilationResults.begin(), compilationResults.end());
	}

};


int main() {
	HelloTriangleApplication app;
	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}




/*
	.Make the core class, which contains VkDevice, VmaAllocator, etc.
	.Make the other classes for Buffer, Semaphore, Fence, CommandPool that take core in the constructor
	.Store pointer to core everywhere

*/