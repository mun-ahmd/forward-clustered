#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include <numeric>
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <set>
#include <optional>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <functional>

#include <shaderc/shaderc.hpp>

#include "MeshLoader.h"
#include "cameraObj.h"

#include "core.hpp"
#include "resources.hpp"

#include "imgui_helper.h"
#include "CubeVoxel.h"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr int lightCount = 10;

//RC = Reference Counted
template<typename T>
using RC = std::shared_ptr<T>;

struct AABB {
	glm::vec4 minPos;
	glm::vec4 maxPos;
};

struct ClusterLights {
	int offset;
	int count;
};

struct globalDescriptor {
	glm::mat4 proj;
	glm::mat4 view;
	glm::mat4 projView;

	void setView(glm::mat4& view) {
		this->view = view;
		this->projView = proj * view;
	}
};


template<class FrameData>
class FrameBase {
private:
	std::optional<std::function<void(FrameData* frame)>> destroyFunc;

	//to avoid reallocating each frame
	std::vector<VkSemaphore> waitSemaphores;
	std::vector<VkShaderStageFlags> waitStageMasks;
	std::vector<VkFence> waitFences;
public:

	VulkanCore core;

	VkCommandPool commandBufferPool;
	VkCommandBuffer commandBuffer;

	VkFence inFlightFence;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	//if it does not have a value then frame is not in flight
	std::optional<uint32_t> imageIndex;

	//Frame Data:
	FrameData data;

	FrameBase() = default;

	FrameBase(VulkanCore core, VkCommandPool commandPool, std::function<void(FrameData*)> initializer, std::function<void(FrameData*)> destroyFunc)
		: core(core), commandBufferPool(commandPool), destroyFunc(destroyFunc) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandBufferPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		commandBuffer = core->createCommandBuffer(allocInfo);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		imageAvailableSemaphore = core->createSemaphore(semaphoreInfo);
		renderFinishedSemaphore = core->createSemaphore(semaphoreInfo);
		inFlightFence = core->createFence(fenceInfo);

		initializer(&this->data);
	}

	void performFrame(
		SwapChain& swapChain,
		std::function<void(FrameBase<FrameData>&)> performer,
		std::vector<VkFence> additionalWaitFences,
		std::vector<std::pair<VkSemaphore, VkPipelineStageFlags>> additionalWaitSemaphores
	) {
		imageIndex.reset();

		this->waitFences.clear();
		for (auto& ele : additionalWaitFences) {
			this->waitFences.push_back(ele);
		}
		this->waitFences.push_back(this->inFlightFence);
		vkWaitForFences(core->device, this->waitFences.size(), this->waitFences.data(), VK_TRUE, std::numeric_limits<uint64_t>::max());

		try {
			imageIndex = swapChain.acquireNextImage(core->device, imageAvailableSemaphore);
		}
		catch (const OutOfDateSwapChainError& err) {
			throw err;
		}

		vkResetFences(core->device, this->waitFences.size(), this->waitFences.data());
		vkResetCommandBuffer(commandBuffer, 0);

		performer(*this);

		if (!imageIndex.has_value()) {
			//no frame in flight
			throw std::runtime_error("No Frame in Flight!");
		}
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		this->waitSemaphores.clear();
		this->waitStageMasks.clear();
		for (auto& pair : additionalWaitSemaphores) {
			this->waitSemaphores.push_back(pair.first);
			this->waitStageMasks.push_back(pair.second);
		}
		this->waitSemaphores.push_back(imageAvailableSemaphore);
		this->waitStageMasks.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		submitInfo.waitSemaphoreCount = this->waitSemaphores.size();
		submitInfo.pWaitSemaphores = this->waitSemaphores.data();
		submitInfo.pWaitDstStageMask = this->waitStageMasks.data();

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
		if (destroyFunc.has_value())
			destroyFunc.value()(&this->data);
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroyFence(device, inFlightFence, nullptr);
		vkFreeCommandBuffers(device, commandBufferPool, 1, &commandBuffer);
	}
};

struct FrameData {
	void* matricesMappedPointer;
	RC<Buffer> matricesBuffer;
	VkDescriptorSet globalDS;

	PointLightsBuffer pointLights;
	VkDescriptorSet pointLightsDS;

	//IAResource<NullResourceInfo, AABB> clustersBuffer;
	IAResource<PointLightLength, int> lightIndexBuffer;
	RC<Buffer> clusterLightsBuffer;
	VkDescriptorSet clusterCompDS;
	VkSemaphore clusterCompSemaphore;
	VkFence clusterCompFence;

	VkCommandBuffer computeCommandBuffer;

	VoxelChunkBuffer vcb;
	VkDescriptorSet vcbDescriptorSet;
	VoxelDrawBuffer vdb;
	VkDescriptorSet vdbDescriptorSet;
};

typedef FrameBase<FrameData> Frame;

class Application {
	Application() {}
public:
	static Application& app() {
		static Application application;
		return application;
	}

	void run() {
		initVulkan();
		mainLoop();
	}

	~Application() {
		meshes.clear();
		meshes.shrink_to_fit();

		for (auto& frame : frames)
			frame.cleanup(core->device);

		vkDestroyCommandPool(core->device, commandPool, nullptr);

		vkDestroyPipeline(core->device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(core->device, pipelineLayout, nullptr);
		vkDestroyPipeline(core->device, depthPrePass.pipe, nullptr);
		vkDestroyPipelineLayout(core->device, depthPrePass.layout, nullptr);
		vkDestroyPipeline(core->device, clusterComp.pipe, nullptr);
		vkDestroyPipelineLayout(core->device, clusterComp.layout, nullptr);
		this->imgui.destroy();

		vkDestroyRenderPass(core->device, renderPass, nullptr);

		swapChain.cleanupFramebuffers();
		swapChain.cleanupSwapChain();
	}

	inline VulkanCore getCore() { return core; }
	inline VkCommandPool getCommandPool() { return commandPool; }

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

	std::array<Frame, MAX_FRAMES_IN_FLIGHT> frames;
	uint32_t current_frame = 0;

private:
	VulkanCore core;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	struct {
		VkPipelineLayout layout;
		VkPipeline pipe;
	} depthPrePass;

	struct {
		VkPipelineLayout layout;
		VkPipeline pipe;
	} clusterComp;

	SwapChain swapChain;

	VkCommandPool commandPool;

	shaderc::Compiler compiler = shaderc::Compiler();

	std::vector<Mesh> meshes;
	std::vector<RC<Material>> materials;
	std::vector<uint32_t> meshMatIndices;
	std::vector<glm::mat4> transforms;

	std::vector<PointLightInfo> pointLights;
	VkDescriptorSet materialsDescriptorSet;

	//RC<Buffer> buffer;
	MyImgui imgui;

	CamHandler camera;

	float nearPlane = 0.1f;
	float farPlane = 200.f;



	void initMeshesMaterials() {
		camera = CamHandler(core->window);
		constexpr int chunkCount = 1;
	
		VoxelChunk chunks[chunkCount];
		for (size_t i = 0; i < chunkCount; i++)
		{
			auto& chunk = chunks[i];
			
			int j = i + 1;
			for (auto& [voxel, coord] : chunks[i]) {
				*voxel = j * (int)glm::all(glm::equal(coord % (j+1), glm::ivec3(0)));
			}
		}

		std::vector<unsigned int> cubeIndices(36);
		std::iota(cubeIndices.begin(), cubeIndices.end(), 0);
		auto cubeMeshData = MeshData<Vertex3>(getCubeVertices(), cubeIndices);
		auto cubeMesh = Mesh(
			core,
			commandPool,
			cubeMeshData
		);

		meshes.clear();
		transforms.clear();
		meshMatIndices.clear();
		materials.clear();
		for (int i = 0; i < chunkCount; i++) {
			glm::vec3 baseTranslate = glm::vec3(
				VoxelChunk::chunkSize.x * (float)chunkCount, 0.0f, 0.0f
			);

			for (auto&[voxel, coord] : chunks[i]) {
				if (*voxel != 0) {
					
					meshes.push_back(cubeMesh);

					transforms.push_back(
						glm::translate(
							glm::translate(
								glm::scale(
									glm::mat4(1.f),
									glm::vec3(0.5)
								),
								baseTranslate
							),
							glm::vec3(coord)
						)
					);

					meshMatIndices.push_back(*voxel - 1);
				}
			}
			materials.push_back(Material::create(core, commandPool, randomMaterial()));
		}


		return;

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

	void initLights() {
		this->pointLights.resize(lightCount);

		for (auto& light : pointLights) {
			auto rm = randomMaterial();
			light.position = glm::normalize(glm::vec3(rm.diffuseColor));
			light.radius = 1.0f;
			light.color = rm.diffuseColor;
		}

		for (auto& frame : this->frames) {
			frame.data.pointLights.addLights(core, commandPool, this->pointLights.data(), this->pointLights.size());
		}
	}

	void frameInitializer(FrameData* frame) {
		frame->matricesBuffer = Buffer::create(core, sizeof(globalDescriptor), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VmaAllocationCreateFlagBits)(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), 0);
		frame->matricesMappedPointer = frame->matricesBuffer->allocation->GetMappedData();

		{
			VkDescriptorSetLayoutBinding binding{};
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			binding.stageFlags = VK_SHADER_STAGE_ALL;
			binding.binding = 0;
			frame->globalDS = core->createDescriptorSet({ binding });

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorCount = 1;
			write.descriptorType = binding.descriptorType;
			write.dstSet = frame->globalDS;
			write.dstBinding = binding.binding;
			write.pNext = nullptr;
			VkDescriptorBufferInfo info{};
			info.buffer = frame->matricesBuffer->buffer;
			info.offset = 0;
			info.range = sizeof(globalDescriptor);
			write.pBufferInfo = &info;

			vkUpdateDescriptorSets(core->device, 1, &write, 0, nullptr);
		}

		frame->pointLights.init(core);
		frame->pointLightsDS = frame->pointLights.createDescriptor(core, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

		glm::uvec3 clusterSize = glm::uvec3(32, 32, 4);
		//frame->clustersBuffer.init(core, sizeof(AABB) * clusterSize.x * clusterSize.y * clusterSize.z);
		//frame->clusterCompDS = frame->clustersBuffer.createDescriptor(core, VK_SHADER_STAGE_COMPUTE_BIT);
		frame->lightIndexBuffer.init(core, 10000);

		frame->clusterLightsBuffer = Buffer::create(core, clusterSize.x * clusterSize.y * clusterSize.z * sizeof(ClusterLights),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			0);
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			binding.binding = 0;

			VkDescriptorSetLayoutBinding binding2{};
			binding2.descriptorCount = 1;
			binding2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding2.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			binding2.binding = 1;

			VkDescriptorSetLayoutBinding binding3{};
			binding3.descriptorCount = 1;
			binding3.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding3.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			binding3.binding = 2;
			frame->clusterCompDS = core->createDescriptorSet({ binding, binding2, binding3 });


			VkDescriptorBufferInfo info{};
			info.buffer = frame->lightIndexBuffer.resourceBuf->buffer;
			info.offset = 0;
			info.range = sizeof(PointLightLength);
			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorCount = 1;
			write.descriptorType = binding.descriptorType;
			write.dstSet = frame->clusterCompDS;
			write.dstBinding = binding.binding;
			write.pNext = nullptr;
			write.pBufferInfo = &info;

			VkDescriptorBufferInfo info2{};
			info2.buffer = frame->lightIndexBuffer.resourceBuf->buffer;
			info2.offset = sizeof(PointLightLength);
			info2.range = sizeof(int) * frame->lightIndexBuffer.maxLength;
			VkWriteDescriptorSet write2{};
			write2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write2.descriptorCount = 1;
			write2.descriptorType = binding2.descriptorType;
			write2.dstSet = frame->clusterCompDS;
			write2.dstBinding = binding2.binding;
			write2.pNext = nullptr;
			write2.pBufferInfo = &info2;

			VkDescriptorBufferInfo info3{};
			info3.buffer = frame->clusterLightsBuffer->buffer;
			info3.offset = 0;
			info3.range = sizeof(ClusterLights) * clusterSize.x * clusterSize.y * clusterSize.z;
			VkWriteDescriptorSet write3{};
			write3.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write3.descriptorCount = 1;
			write3.descriptorType = binding3.descriptorType;
			write3.dstSet = frame->clusterCompDS;
			write3.dstBinding = binding3.binding;
			write3.pNext = nullptr;
			write3.pBufferInfo = &info3;

			VkWriteDescriptorSet writes[3] = { write, write2, write3 };
			vkUpdateDescriptorSets(core->device, 3, writes, 0, nullptr);
		}

		VkCommandBufferAllocateInfo allocInf{};
		allocInf.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInf.commandBufferCount = 1;
		allocInf.commandPool = this->commandPool;
		allocInf.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		frame->computeCommandBuffer = core->createCommandBuffer(allocInf);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		frame->clusterCompSemaphore = core->createSemaphore(semaphoreInfo);

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		frame->clusterCompFence = core->createFence(fenceInfo);
	}

	void frameDestructor(FrameData* frame) {
		vkDestroySemaphore(core->device, frame->clusterCompSemaphore, nullptr);
		vkDestroyFence(core->device, frame->clusterCompFence, nullptr);
	}

	void initVulkan() {
		core = createVulkanCore();
		createCommandPool();
		for (auto& frame : frames)
			frame = Frame(
				core, commandPool,
				std::bind(&Application::frameInitializer, this, std::placeholders::_1),
				std::bind(&Application::frameDestructor, this, std::placeholders::_1)
			);
		auto swapCapabilities = core->querySwapChainSupport();
		auto swapChainFormat = chooseSwapSurfaceFormat(swapCapabilities.formats);
		auto swapPresentMode = chooseSwapPresentMode(swapCapabilities.presentModes);
		Material::init(core);
		materialsDescriptorSet = Material::createDescriptor(core, VK_SHADER_STAGE_FRAGMENT_BIT);
		initLights();
		createRenderPass(swapChainFormat.format);
		createGraphicsPipeline();
		createDepthPrePassPipeline();
		createClusterComputePipeline();
		swapChain = SwapChain(core, swapChainFormat, swapPresentMode, chooseSwapExtent(swapCapabilities.capabilities), renderPass);
		initMeshesMaterials();
		imgui.init(core, this->renderPass, this->frames[0].commandBuffer, 1);
	}

	void mainLoop() {
		//make a model view matrix for rendering the object
		glm::vec3 camPos = { 0.f,0.f,-1.f };
		//camera position
		glm::mat4 view = camera.getView();
		//camera projection
		glm::mat4 projection = glm::perspective(glm::radians(70.f), static_cast<float>(swapChain.swapChainExtent.width) / swapChain.swapChainExtent.height, nearPlane, farPlane);

		globalDescriptor gDescValue{};
		gDescValue.proj = projection;

		auto inputManager = getInputManager(core->window);
		inputManager->addCallback(GLFW_KEY_0, "0call", [](int state, int mods) {
			if (state == GLFW_PRESS) {
				std::cout << "\nPressed zero" << std::endl;
			}
			});

		double startTime = glfwGetTime();
		bool shouldShowMouse = false;
		double lastCursorPos[2];
		inputManager->addCallback(GLFW_KEY_ESCAPE, "shouldShowMouseToggle", [&shouldShowMouse, this, &lastCursorPos](int state, int mods) {
			if (state == GLFW_PRESS) {
				if (mods & GLFW_MOD_SHIFT) {
					glfwSetWindowShouldClose(core->window, GLFW_TRUE);
				}
				else {
					shouldShowMouse = !shouldShowMouse;
					if (shouldShowMouse) {
						glfwGetCursorPos(core->window, &lastCursorPos[0], &lastCursorPos[1]);
						glfwSetInputMode(core->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
					}
					else {
						glfwSetCursorPos(core->window, lastCursorPos[0], lastCursorPos[1]);
						glfwSetInputMode(core->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					}
				}
			}
			});

		//compute clusters buffer once before first frame
		//computeClusters(frames.front());

		auto frameActions =
			[&](Frame& activeFrame) {
			memcpy(activeFrame.data.matricesMappedPointer, &gDescValue, sizeof(globalDescriptor));
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0; // Optional
			beginInfo.pInheritanceInfo = nullptr; // Optional

			if (vkBeginCommandBuffer(activeFrame.commandBuffer, &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			auto activeSwapChainFramebuffer = swapChain.swapChainFramebuffers[activeFrame.imageIndex.value()];


			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = activeSwapChainFramebuffer;

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChain.swapChainExtent;

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(activeFrame.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			//Start of depth prepass

			vkCmdBindPipeline(activeFrame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, depthPrePass.pipe);
			vkCmdBindDescriptorSets(activeFrame.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				0, 1, &activeFrame.data.globalDS,
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

			VkDeviceSize offset = 0;
			for (int i = 0; i < meshes.size(); ++i) {
				MeshPushConstants constants{ transforms[i] };
				vkCmdPushConstants(activeFrame.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

				vkCmdBindVertexBuffers(activeFrame.commandBuffer, 0, 1, &meshes[i].vertexBuffer->buffer, &offset);
				vkCmdBindIndexBuffer(activeFrame.commandBuffer, meshes[i].indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT16);
				vkCmdDrawIndexed(activeFrame.commandBuffer, meshes[i].numIndices, 1, 0, 0, 0);
			}

			//end of depth prepass
			vkCmdNextSubpass(activeFrame.commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(activeFrame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkDescriptorSet globalAndLightsSet[2] = { activeFrame.data.globalDS, activeFrame.data.pointLightsDS };
			vkCmdBindDescriptorSets(activeFrame.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				0, 2, globalAndLightsSet,
				0, nullptr
			);

			vkCmdSetViewport(activeFrame.commandBuffer, 0, 1, &viewport);

			vkCmdSetScissor(activeFrame.commandBuffer, 0, 1, &scissor);

			//make a model view matrix for rendering the object
			//camera position

			offset = 0;
			for (int i = 0; i < meshes.size(); ++i) {
				uint32_t dynamicOffset = materials[meshMatIndices[i]]->getResourceOffset();
				vkCmdBindDescriptorSets(
					activeFrame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineLayout, 2, 1, &materialsDescriptorSet, 1, &dynamicOffset
				);

				MeshPushConstants constants{ transforms[i] };
				vkCmdPushConstants(activeFrame.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

				vkCmdBindVertexBuffers(activeFrame.commandBuffer, 0, 1, &meshes[i].vertexBuffer->buffer, &offset);
				vkCmdBindIndexBuffer(activeFrame.commandBuffer, meshes[i].indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT16);
				vkCmdDrawIndexed(activeFrame.commandBuffer, meshes[i].numIndices, 1, 0, 0, 0);
			}

			imgui.drawWithinRenderPass(activeFrame.commandBuffer);

			vkCmdEndRenderPass(activeFrame.commandBuffer);

			if (vkEndCommandBuffer(activeFrame.commandBuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
			};

		while (!glfwWindowShouldClose(core->window)) {
			try {
				double deltaTime = glfwGetTime() - startTime;
				startTime = glfwGetTime();
				if (!shouldShowMouse) {
					camera.moveAround(deltaTime);
					camera.lookAround();
				}

				view = camera.getView();
				gDescValue.setView(view);

				this->imgui.newFrame();
				this->imgui.test();
				computeClusters(frames[current_frame]);

				frames[current_frame].performFrame(
					swapChain,
					frameActions,
					{ frames[current_frame].data.clusterCompFence },
					{ {frames[current_frame].data.clusterCompSemaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT} }
				);

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

	void computeClusters(Frame& anyFrame) {
		PointLightLength length;
		length.length = 0;
		anyFrame.data.lightIndexBuffer.updateBase(core, commandPool, length);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(anyFrame.data.computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		vkCmdBindPipeline(anyFrame.data.computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, clusterComp.pipe);
		glm::vec4 zBoundsVec = glm::vec4(nearPlane, farPlane, 0.0, 0.0);
		vkCmdPushConstants(anyFrame.data.computeCommandBuffer, clusterComp.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::vec4), &zBoundsVec);
		VkDescriptorSet descriptors[3] = { anyFrame.data.globalDS, anyFrame.data.pointLightsDS, anyFrame.data.clusterCompDS };
		vkCmdBindDescriptorSets(
			anyFrame.data.computeCommandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			clusterComp.layout, 0, 3, descriptors,
			0, 0
		);

		vkCmdDispatch(anyFrame.data.computeCommandBuffer, 32, 32, 4);

		if (vkEndCommandBuffer(anyFrame.data.computeCommandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &anyFrame.data.computeCommandBuffer;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &anyFrame.data.clusterCompSemaphore;
		vkQueueSubmit(core->graphicsQueue, 1, &submitInfo, anyFrame.data.clusterCompFence);
	}

	void createClusterComputePipeline() {
		//pipeline creation
		auto computeShaderCode = compileGlslToSpv("shaders/clusters.comp", shaderc_shader_kind::shaderc_compute_shader);
		VkShaderModule computeShaderModule = createShaderModule(computeShaderCode);

		VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = computeShaderModule;
		computeShaderStageInfo.pName = "main";

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 3;
		VkDescriptorSetLayout layouts[3] = {
			core->getLayout(frames.front().data.globalDS),
			core->getLayout(frames.front().data.pointLightsDS),
			core->getLayout(frames.front().data.clusterCompDS)
		};
		pipelineLayoutInfo.pSetLayouts = layouts;

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::vec4);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		this->clusterComp.layout = core->createPipelineLayout(pipelineLayoutInfo);

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = this->clusterComp.layout;
		pipelineInfo.stage = computeShaderStageInfo;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		clusterComp.pipe = core->createComputePipeline(pipelineInfo);

		vkDestroyShaderModule(core->device, computeShaderModule, nullptr);
	}

	void createDepthPrePassPipeline() {
		//pipeline creation
		auto vertShaderCode = compileGlslToSpv("Shaders/prePass.vert", shaderc_shader_kind::shaderc_vertex_shader);
		auto fragShaderCode = compileGlslToSpv("Shaders/prePass.frag", shaderc_shader_kind::shaderc_fragment_shader);

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
		pipelineLayoutInfo.setLayoutCount = 1;
		VkDescriptorSetLayout layouts[1] = {
			core->getLayout(frames.front().data.globalDS)
		};
		pipelineLayoutInfo.pSetLayouts = layouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &push_constant;

		this->depthPrePass.layout = core->createPipelineLayout(pipelineLayoutInfo);

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
		//pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &depthStencilCI;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = this->depthPrePass.layout;

		pipelineInfo.renderPass = this->renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		this->depthPrePass.pipe = core->createGraphicsPipeline(pipelineInfo);

		vkDestroyShaderModule(core->device, fragShaderModule, nullptr);
		vkDestroyShaderModule(core->device, vertShaderModule, nullptr);
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

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = core->findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription prePass{};
		prePass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		prePass.colorAttachmentCount = 0;
		prePass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency depthDependency = {};
		depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		depthDependency.dstSubpass = 0;
		depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.srcAccessMask = 0;
		depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 1;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency passDependency{};
		passDependency.srcSubpass = 0;
		passDependency.dstSubpass = 1;
		passDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		passDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		passDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		passDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		passDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


		VkSubpassDependency dependencies[3] = { dependency, depthDependency, passDependency };

		VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

		VkSubpassDescription subpasses[2] = { prePass, subpass };

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 2;
		renderPassInfo.pSubpasses = subpasses;
		renderPassInfo.dependencyCount = 3;
		renderPassInfo.pDependencies = dependencies;

		renderPass = core->createRenderPass(renderPassInfo);
	}

	void createGraphicsPipeline() {
		auto vertShaderCode = compileGlslToSpv("Shaders/triangle.vert", shaderc_shader_kind::shaderc_vertex_shader);
		auto fragShaderCode = compileGlslToSpv("Shaders/triangle.frag", shaderc_shader_kind::shaderc_fragment_shader);

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
		rasterizer.lineWidth = 10.0f;
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
		depthStencilCI.depthTestEnable = VK_TRUE;
		depthStencilCI.depthWriteEnable = VK_FALSE;
		depthStencilCI.depthCompareOp = VK_COMPARE_OP_EQUAL;
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
		pipelineLayoutInfo.setLayoutCount = 3;
		VkDescriptorSetLayout layouts[3] = {
			core->getLayout(frames.front().data.globalDS),
			core->getLayout(frames.front().data.pointLightsDS),
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
		pipelineInfo.pDepthStencilState = &depthStencilCI;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = pipelineLayout;

		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		graphicsPipeline = core->createGraphicsPipeline(pipelineInfo);

		vkDestroyShaderModule(core->device, fragShaderModule, nullptr);
		vkDestroyShaderModule(core->device, vertShaderModule, nullptr);
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
		this->swapChain.cleanupFramebuffers();
		this->swapChain.cleanupSwapChain();

		auto swapCapabilities = querySwapChainSupport(core->physicalDevice);
		auto swapChainFormat = chooseSwapSurfaceFormat(swapCapabilities.formats);
		auto swapPresentMode = chooseSwapPresentMode(swapCapabilities.presentModes);

		if (oldSwapChainFormat != swapChainFormat.format) {
			//todo Recreate renderpass
		}
		swapChain = SwapChain(core, swapChainFormat, swapPresentMode, chooseSwapExtent(swapCapabilities.capabilities), renderPass);
	}
};



/*
	future externs:
		glslToSpirvFunc
		spirvToModuleFunc
		VulkanCore 
*/