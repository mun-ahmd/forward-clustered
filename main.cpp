#include <numeric>
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <set>
#include <optional>
#include <functional>

//#define NO_TEXTURES

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include <glm/gtx/string_cast.hpp>


#include "vulkan_utils.hpp"
#include "MeshLoader.hpp"
#include "ImageLoader.hpp"
#include "cameraObj.h"
#include "material.hpp"
#include "light.hpp"
#include "imgui_helper.hpp"
#include <filesystem>
#include "swapchain.hpp"
#include "mesh.hpp"
#include "frame.hpp"
#include "skybox.hpp"
#include "asyncImageLoader.hpp"

constexpr int lightCount = 10;

struct MeshPushConstants {
	glm::mat4 transform;
};

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
	glm::vec4 cameraPos_time;
	glm::vec4 fovY_aspectRatio_zNear_zFar;

	void updateValues(
		glm::mat4 proj,
		float fovY,
		float aspectRatio,
		float zNear,
		float zFar,
		glm::mat4 view,
		glm::vec3 cameraPos,
		float time_seconds
	) {
		this->proj = proj;
		this->view = view;
		this->projView = proj * view;
		this->fovY_aspectRatio_zNear_zFar = { fovY, aspectRatio, zNear, zFar };
		this->cameraPos_time = glm::vec4(cameraPos, time_seconds);
	}

	void setView(glm::mat4& view) {
		this->view = view;
		this->projView = proj * view;
	}
};

struct FrameData {
	void* globalDescBufferMappedPointer;
	RC<Buffer> globalDescBuffer;
	VkDescriptorSet globalDS;

	VkDescriptorSet pointLightsDS;

	//IAResource<NullResourceInfo, AABB> clustersBuffer;
	IAResource<PointLightLength, int> lightIndexBuffer;
	RC<Buffer> clusterLightsBuffer;
	VkDescriptorSet clusterCompDS;
	VkSemaphore clusterCompSemaphore;
	VkFence clusterCompFence;

	VkCommandBuffer computeCommandBuffer;
};

typedef FrameBase<FrameData> Frame;

class Application {
public:
	Application() {}

	void run() {
		initVulkan();
		mainLoop();
	}

	~Application() {
		meshes.clear();
		meshes.shrink_to_fit();
		materials.clear();

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

private:
	VulkanCore core;

	std::array<Frame, MAX_FRAMES_IN_FLIGHT> frames;
	uint32_t current_frame = 0;

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

	std::vector<Mesh> meshes;

	Materials materials;
	std::vector<uint32_t> meshMatIndices;
	std::vector<glm::mat4> transforms;

	std::vector<PointLightInfo> pointLights;
	LightsBuffer lightsBuffer;

	VkDescriptorSet materialsDescriptorSet;

	//RC<Buffer> buffer;
	MyImgui imgui;

	CamHandler camera;

	float nearPlane = 0.1f;
	float farPlane = 200.f;

	uint32_t numFramesInFlight = MAX_FRAMES_IN_FLIGHT;

	void initMeshesMaterialsLights() {
		this->meshes.clear();
		this->transforms.clear();
		this->meshMatIndices.clear();
		this->materials.clear();
		this->pointLights.clear();

		auto loadedModel = loadGLTF(gltfModelSelector.loadedModelPath.c_str()).value();

		meshes.reserve(loadedModel.meshData.meshes.size());
		materials.reserve(loadedModel.meshData.meshes.size());
		transforms = loadedModel.meshData.transforms;
		meshMatIndices = std::vector<uint32_t>(
			loadedModel.meshData.matIndex.begin(),
			loadedModel.meshData.matIndex.end()
		);

		materials.addMaterialImage(
			loadImage(
				R"(C:\Users\munee\source\repos\VulkanRenderer\3DModels\blankVaibhav.png)",
				4,
				true
			), vSampler
		);
																			
		std::unordered_map<std::string, unsigned int> imagePathToIndex;
		std::vector<std::string> imagePaths = { R"(C:\Users\munee\source\repos\VulkanRenderer\3DModels\blankVaibhav.png)" };
		//empty path means default 0 texture
		imagePathToIndex[""] = 0;
#ifndef NO_TEXTURES
		for (auto& mat : loadedModel.materials) {
			std::string textures[2];
			if (mat.isMetallicRoughness) {
				textures[0] = mat.metallicRoughness.baseColorTex;
				textures[1] = mat.metallicRoughness.metallicRoughnessTex;
			}
			else {
				throw std::runtime_error("");
				textures[0] = mat.diffuseSpecular.diffuseTex;
				textures[1] = mat.diffuseSpecular.specularGlossTex;
			}
			
			for (int i = 0; i < 2; i++) {
				std::string tex = textures[i];
				if (
					imagePathToIndex.find(tex) == imagePathToIndex.end()
					) {
					imagePaths.push_back(tex);
					imagePathToIndex[tex] =
						materials.addMaterialImage(
							loadImage(
								tex.c_str(),
								i == 0 ? 4 : 2,
								false,
								std::nullopt
							), vSampler
						);
				}
				//std::cout << std::endl << std::endl;
			}
		}
#endif
		
		for (MaterialPBR& loadedMat : loadedModel.materials) {
			MaterialInfo matInf{};
#ifdef NO_TEXTURES
			loadedMat.metallicRoughness.baseColorTex = "";
			loadedMat.metallicRoughness.metallicRoughnessTex = "";
			loadedMat.diffuseSpecular.diffuseTex = "";
			loadedMat.diffuseSpecular.specularGlossTex = "";
#endif
			matInf.baseColorFactor = loadedMat.isMetallicRoughness ?
				loadedMat.metallicRoughness.baseColorFactor :
				loadedMat.diffuseSpecular.diffuseFactor;

			matInf.metallicRoughness_waste2 = loadedMat.isMetallicRoughness ?
				glm::vec4(
					glm::vec2(
						loadedMat.metallicRoughness.metallic_factor,
						loadedMat.metallicRoughness.roughness_factor
					), 1.0, 1.0
				) :
				glm::vec4(
					loadedMat.diffuseSpecular.specularFactor,
					loadedMat.diffuseSpecular.glossinessFactor
				);

				matInf.baseTexId_metallicRoughessTexId_waste2 =
					glm::uvec4(
						loadedMat.isMetallicRoughness ?
						imagePathToIndex[
							loadedMat.metallicRoughness.baseColorTex
						] :
						imagePathToIndex[
							loadedMat.diffuseSpecular.diffuseTex
						],
						loadedMat.isMetallicRoughness ?
						imagePathToIndex[
							loadedMat.metallicRoughness.metallicRoughnessTex
						] :
						imagePathToIndex[
							loadedMat.diffuseSpecular.specularGlossTex
						],
						0,
						0
					);
			//std::cout << std::endl << "Material: " << matIndex;
			//std::cout << imagePaths[matInf.baseTexId_metallicRoughessTexId_waste2.x] << " ";
			//std::cout << imagePaths[matInf.baseTexId_metallicRoughessTexId_waste2.y] << std::endl;

			materials.addMaterial(core, commandPool, matInf);
		}

		for (size_t i = 0; i < loadedModel.meshData.meshes.size(); ++i)
			meshes.push_back(Mesh(core, commandPool, loadedModel.meshData.meshes[i]));

		this->pointLights = std::move(loadedModel.pointLights);
		for (int i = 0; i < frames.size(); i++) {
			auto& frame = frames[i];
			
			this->lightsBuffer.setSunLight(loadedModel.directionalLight, i);

			this->lightsBuffer.addPointLights(
				core, commandPool,
				this->pointLights.data(), this->pointLights.size()
			);
		}
	}

	RC<Sampler> vSampler;

	SkyboxRenderer skyboxR;

	RC<AsyncImageLoader> imageLoader;

	void initVulkan() {
		core = VulkanUtils::utils().getCore();

		commandPool = VulkanUtils::utils().getCommandPool();

		this->lightsBuffer = LightsBuffer(core, 1000, numFramesInFlight);

		int i = 0;
		for (auto& frame : frames) {
			this->current_frame = i;
			frame = Frame(
				core, commandPool,
				std::bind(&Application::frameInitializer, this, std::placeholders::_1),
				std::bind(&Application::frameDestructor, this, std::placeholders::_1)
			);
			i++;
		}
		this->current_frame = 0;

		auto swapCapabilities = core->querySwapChainSupport();
		auto swapChainFormat = chooseSwapSurfaceFormat(swapCapabilities.formats);
		auto swapPresentMode = chooseSwapPresentMode(swapCapabilities.presentModes);
		
		materials.init(1000);
		materialsDescriptorSet = materials.getDescriptorSet();
		
		vSampler = Sampler::create(core, Sampler::makeCreateInfo(
			{ VK_FILTER_LINEAR, VK_FILTER_LINEAR },
			{VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
			{},
			VK_SAMPLER_MIPMAP_MODE_LINEAR,
			core->gpuProperties.limits.maxSamplerAnisotropy
		));

		createRenderPass(swapChainFormat.format);
		createGraphicsPipeline();
		createDepthPrePassPipeline();
		createClusterComputePipeline();
		swapChain = SwapChain(core, swapChainFormat, swapPresentMode, chooseSwapExtent(swapCapabilities.capabilities), renderPass);
		
		camera = CamHandler(core->window);
		if (Store::itemInStore("cameraMovementSpeed")) {
			auto loadedBytes = Store::fetchBytes("cameraMovementSpeed");
			float loadedMovementSpeed = camera.movementSpeed();
			memcpy(&loadedMovementSpeed, loadedBytes.get(), sizeof(loadedMovementSpeed));
			camera.movementSpeed() = loadedMovementSpeed;
		}

		imageLoader = std::make_shared<AsyncImageLoader>(
			glm::ivec3(8192, 4096, 1), 4, 2
		);
		imageLoader->init();
		imageLoader->start();

		skyboxR.initialize(imageLoader, renderPass, 1);
		initMeshesMaterialsLights();

		imgui.init(core, this->renderPass, this->frames[0].commandBuffer, 1);		
	}
	 

	bool hasModelChanged = false;
	bool rebuildShadingPipe = false;

	globalDescriptor gDescValue{};

	void frameInitializer(FrameData* frame) {
		frame->globalDescBuffer = Buffer::create(core, sizeof(globalDescriptor), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VmaAllocationCreateFlagBits)(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), 0);
		frame->globalDescBufferMappedPointer = frame->globalDescBuffer->allocation->GetMappedData();

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
			info.buffer = frame->globalDescBuffer->buffer;
			info.offset = 0;
			info.range = sizeof(globalDescriptor);
			write.pBufferInfo = &info;

			vkUpdateDescriptorSets(core->device, 1, &write, 0, nullptr);
		}

		frame->pointLightsDS = this->lightsBuffer.createDescriptorSet(
			current_frame,
			core,
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT
		);

		glm::uvec3 clusterSize = glm::uvec3(32, 32, 4);
		//frame->clustersBuffer.init(core, sizeof(AABB) * clusterSize.x * clusterSize.y * clusterSize.z);
		//frame->clusterCompDS = frame->clustersBuffer.createDescriptor(core, VK_SHADER_STAGE_COMPUTE_BIT);
		frame->lightIndexBuffer.init(core, 10000);

		frame->clusterLightsBuffer = Buffer::create(core, clusterSize.x * clusterSize.y * clusterSize.z * sizeof(ClusterLights),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			0
		);

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

	void dataTransferActions(FrameData& transferFrameData) {
		//cpu<-->gpu transfers should be done here
		// this frame is currently not in flight
		// next image has not yet been acquired

		memcpy(
			transferFrameData.globalDescBufferMappedPointer,
			&gDescValue,
			sizeof(globalDescriptor)
		);

		PointLightLength length;
		length.length = 0;
		transferFrameData.lightIndexBuffer.updateBase(core, commandPool, length);
	}

	void frameActions(Frame& activeFrame,
		std::vector<VkFence>& additionalWaitFences,
		std::vector<VkSemaphore>& additionalWaitSemaphores,
		std::vector<VkPipelineStageFlags>& waitPipelineStageFlags) {
		//perform no cpu<->gpu transfers here

		this->imgui.newFrame();

		ImGui::Begin("UI");
		{
			if(ImGui::CollapsingHeader("Model Selection Menu"))
			{
				gltfModelSelector.render([this]() { hasModelChanged = true; });
			}

			if (ImGui::CollapsingHeader("Camera settings"))
			{
				if (ImGui::SliderFloat("movement speed", &camera.movementSpeed(), 0.1, 10.0)) {
					Store::storeBytes("cameraMovementSpeed", (char*)&camera.movementSpeed(), sizeof(camera.movementSpeed()));
				}
				float camF3[3] = { camera.get_pos().x, camera.get_pos().y, camera.get_pos().z };
				if (ImGui::InputFloat3("position", camF3)) {
					camera.set_pos(glm::make_vec3(camF3));
				}
				float camViewF3[3] = { camera.get_front().x, camera.get_front().y, camera.get_front().z };
				if (ImGui::InputFloat3("camera direction", camViewF3)) {
					glm::vec3 newFront = glm::normalize(glm::make_vec3(camViewF3));
					if (glm::all(glm::equal(newFront, glm::vec3(0.0, 0.0, 0.0))));
					camera.set_front(newFront);
				}
			}

			if (ImGui::Button("Rebuild Shading Pipeline")) {
				this->rebuildShadingPipe = true;
			}
		}
		ImGui::End();

		//compute clusters
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			if (vkBeginCommandBuffer(activeFrame.data.computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			vkCmdBindPipeline(activeFrame.data.computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, clusterComp.pipe);
			glm::vec4 zBoundsVec = glm::vec4(nearPlane, farPlane, 0.0, 0.0);
			vkCmdPushConstants(activeFrame.data.computeCommandBuffer, clusterComp.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::vec4), &zBoundsVec);
			VkDescriptorSet descriptors[3] = { activeFrame.data.globalDS, activeFrame.data.pointLightsDS, activeFrame.data.clusterCompDS };
			vkCmdBindDescriptorSets(
				activeFrame.data.computeCommandBuffer,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				clusterComp.layout, 0, 3, descriptors,
				0, 0
			);

			vkCmdDispatch(activeFrame.data.computeCommandBuffer, 32, 32, 4);


			if (vkEndCommandBuffer(activeFrame.data.computeCommandBuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &activeFrame.data.computeCommandBuffer;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &activeFrame.data.clusterCompSemaphore;
			vkQueueSubmit(core->graphicsQueue, 1, &submitInfo, activeFrame.data.clusterCompFence);
		}
		//end compute clusters

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
			vkCmdBindIndexBuffer(activeFrame.commandBuffer, meshes[i].indexBuffer->buffer, 0, meshes[i].indexType);
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
			int matIndex = meshMatIndices[i];
			uint32_t dynamicOffset = materials.getResourceOffset(meshMatIndices[i]);
			vkCmdBindDescriptorSets(
				activeFrame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout, 2, 1, &materialsDescriptorSet, 1, &dynamicOffset
			);

			MeshPushConstants constants{ transforms[i] };
			vkCmdPushConstants(activeFrame.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

			vkCmdBindVertexBuffers(activeFrame.commandBuffer, 0, 1, &meshes[i].vertexBuffer->buffer, &offset);
			vkCmdBindIndexBuffer(activeFrame.commandBuffer, meshes[i].indexBuffer->buffer, 0, meshes[i].indexType);
			vkCmdDrawIndexed(activeFrame.commandBuffer, meshes[i].numIndices, 1, 0, 0, 0);
		}

		skyboxR.beginRender(activeFrame.commandBuffer, viewport, scissor);
		SkyboxRenderer::CubemapPushConstants cpushConst{};
		cpushConst.inverseProjView = glm::inverse(glm::mat4(glm::mat3(gDescValue.view))) * glm::inverse(gDescValue.proj);
		//cpushConst.inverseProjView = gDescValue.proj * glm::mat4(glm::mat3(gDescValue.view));

		skyboxR.endRender(
			activeFrame.commandBuffer,
			this->current_frame, //MAJOR todo change to actual frame index
			cpushConst
		);

		imgui.drawWithinRenderPass(activeFrame.commandBuffer);

		vkCmdEndRenderPass(activeFrame.commandBuffer);

		if (vkEndCommandBuffer(activeFrame.commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		additionalWaitFences.push_back(activeFrame.data.clusterCompFence);
		additionalWaitSemaphores.push_back(activeFrame.data.clusterCompSemaphore);
		waitPipelineStageFlags.push_back(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	void mainLoop() {
		//camera projection
		float fovY = glm::radians(70.f),
			aspectRatio = static_cast<float>(swapChain.swapChainExtent.width) / swapChain.swapChainExtent.height,
			zNear = nearPlane,
			zFar = farPlane;
		glm::mat4 projection = glm::perspective(fovY, aspectRatio, zNear, zFar);

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

		hasModelChanged = false;
		auto dataTransferActionsBound = std::bind(
			&Application::dataTransferActions,
			this,
			std::placeholders::_1
		);
		auto frameActionsBound = std::bind(
			&Application::frameActions,
			this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3,
			std::placeholders::_4
		);

		while (!glfwWindowShouldClose(core->window)) {
			try {
				if (hasModelChanged) {
					//wait for meshes to go out of use
					vkDeviceWaitIdle(core->device);

					auto start = std::chrono::high_resolution_clock::now();
					// Reload the models
					this->initMeshesMaterialsLights();
					auto end = std::chrono::high_resolution_clock::now();
					auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
					std::cout << "Loading new model took: " << duration.count() << " ms" << std::endl;
					hasModelChanged = false;
				}

				if (rebuildShadingPipe) {
					vkDeviceWaitIdle(core->device);

					auto start = std::chrono::high_resolution_clock::now();
					
					{
						vkDestroyPipelineLayout(core->device, this->pipelineLayout, nullptr);
						vkDestroyPipeline(core->device, this->graphicsPipeline, nullptr);
					}
					this->createGraphicsPipeline();

					auto end = std::chrono::high_resolution_clock::now();
					auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
					std::cout << "Rebuilding took: " << duration.count() << " ms" << std::endl;

					rebuildShadingPipe = false;
				}

				//transfer async images loaded
				//this a gpu -> gpu transfer through preexisting staging buffers
				{
					this->imageLoader->processResults();
				}

				double deltaTime = glfwGetTime() - startTime;
				startTime = glfwGetTime();
				if (!shouldShowMouse) {
					camera.moveAround(deltaTime);
					camera.lookAround();
				}

				gDescValue.updateValues(
					projection,
					fovY,
					aspectRatio,
					zNear,
					zFar,
					camera.getView(),
					camera.get_pos(),
					(float)glfwGetTime()
				);

				frames[current_frame].performFrame(
					swapChain,
					dataTransferActionsBound,
					frameActionsBound
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

	RC<Image> loadImage(
		const char* filepath,
		int desiredChannels,
		bool isSRGB,
		std::optional<glm::ivec2> desiredResolution = {}
	) {
		assert(desiredChannels == 4 || desiredChannels == 2 || desiredChannels == 1);
		//loads images from files and then
		//creates a VkImage trying its best to find the right format

		glm::ivec3 info = getImageInfo(filepath);

		constexpr VkFormat formats[4][2] = {
			{
				VK_FORMAT_R8_UNORM,
				VK_FORMAT_R8_SRGB
			},
			{
				VK_FORMAT_R8G8_UNORM,
				VK_FORMAT_R8G8_SRGB
			},
			{
				VK_FORMAT_R8G8B8_UNORM,
				VK_FORMAT_R8G8B8_SRGB
			},
			{
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_FORMAT_R8G8B8A8_SRGB
			}
		};

		auto iCreateInf = Image::makeCreateInfo(
			formats[desiredChannels - 1][isSRGB],
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VkExtent3D{ (unsigned int)info.x, (unsigned int)info.y, 1 }
		);

		RC<Image> vImage = Image::create(
			core,
			iCreateInf,
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
		);
		vImage->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		ImageLoadRequest imageLoadRequest{};
		imageLoadRequest.desiredChannels = desiredChannels;
		imageLoadRequest.resolution = glm::ivec3(info.x, info.y, 1);
		imageLoadRequest.path = filepath;
		imageLoadRequest.isSRGB = isSRGB;

		imageLoadRequest.callback = [vImage](ImageLoadRequest& request, RC<Buffer> stagingBuf) {
			//std::cout << "doing callback for: " << request.path << std::endl;

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = vImage->extent;

			vImage->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			vImage->copyFromBuffer(stagingBuf, region);
			vImage->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		};

		imageLoader->request(imageLoadRequest);

		return vImage;
	}

	void createClusterComputePipeline() {
		//pipeline creation
		auto computeShaderCode = VulkanUtils::utils().compileGlslToSpv("shaders/clusters.comp", shaderc_shader_kind::shaderc_compute_shader);
		VkShaderModule computeShaderModule = VulkanUtils::utils().createShaderModule(computeShaderCode);

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


int main() {
	Application app;
	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << "TOP LEVEL ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}




/*
	.Make the core class, which contains VkDevice, VmaAllocator, etc.
	.Make the other classes for Buffer, Semaphore, Fence, CommandPool that take core in the constructor
	.Store pointer to core everywhere

*/