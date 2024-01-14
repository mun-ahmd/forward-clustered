#pragma once
#include <vector>
#include <assert.h>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>

#include "VulkanStartingOut.hpp"
#include "resources.hpp"

constexpr int chunkWidth = 16;
constexpr int chunkHeight = 256;
constexpr int chunkDepth = 16;

class VoxelChunk {
	std::vector<int> voxels;

public:
	VoxelChunk() :
		voxels(std::vector<int>(chunkWidth * chunkHeight * chunkDepth, 0))
	{}

	constexpr inline static glm::ivec3 chunkSize = glm::ivec3(chunkWidth, chunkHeight, chunkDepth);
	constexpr inline static glm::ivec3 chunkLowerLimit = -chunkSize / 2;
	constexpr inline static glm::ivec3 chunkUpperLimit = (chunkSize / 2) - 1;
	constexpr inline static int chunkVoxelCount = (chunkSize.x * chunkSize.y * chunkSize.z);
	constexpr inline static glm::ivec3 unsignedCoords(glm::ivec3 coords) {
		return coords - chunkLowerLimit;
	}
	constexpr inline static glm::ivec3 signedCoords(glm::ivec3 unsignedCoords) {
		return unsignedCoords + chunkLowerLimit;
	}
	constexpr inline static int coordsToIndex(glm::ivec3 coords) {
		glm::ivec3 temp = (
			unsignedCoords(coords) * 
			glm::ivec3(1, chunkWidth * chunkDepth, chunkWidth)
		);
		return temp.x + temp.y + temp.z;
	}
	constexpr inline static glm::ivec3 indexToCoords(int index) {
		int y = index / (chunkSize.x * chunkSize.z);
		index -= (y * chunkSize.x * chunkSize.z);
		return signedCoords({
			index % chunkSize.x,
			y,
			index / chunkSize.x }
		);
	}
	constexpr inline static bool voxelChunkBoundsOK(glm::ivec3 coords) {
		return (
			glm::all(glm::lessThanEqual(coords, chunkUpperLimit))
			&&
			glm::all(glm::greaterThanEqual(coords, chunkLowerLimit))
		);
	}
	inline int& operator[](glm::ivec3 coords) {
		assert(voxelChunkBoundsOK(coords) && "Voxel Coordinates Out of Bounds!");
		return voxels[coordsToIndex(coords)];
	}
	inline int operator[](glm::ivec3 coords) const{
		assert(voxelChunkBoundsOK(coords) && "Voxel Coordinates Out of Bounds!");
		return voxels[coordsToIndex(coords)];
	}

	int* data() {
		return voxels.data();
	}

	const int* data() const {
		return voxels.data();
	}

	class ChunkIterator {
		decltype(voxels.data()) ptr;
		size_t index;
		std::pair<int*, glm::ivec3> data;

	public:
		ChunkIterator(decltype(ptr) voxelPtr, size_t index = 0)
			: index(index), ptr(voxelPtr), data(std::make_pair(&ptr[index], indexToCoords(index))) {}
		
		decltype(data)& operator*() {
			return data;
		}

		ChunkIterator& operator++() {
			++index;
			data = std::make_pair(&ptr[index], indexToCoords(index));
			return *this;
		}
		
		bool operator==(const ChunkIterator& other) const {
			return this->index == other.index;
		}
		bool operator!=(const ChunkIterator& other) const {
			return this->index != other.index;
		}
	};

	ChunkIterator begin() {
		return ChunkIterator(voxels.data());
	}
	ChunkIterator end() {
		return ChunkIterator(voxels.data(), voxels.size());
	}
	//todo implement const chunk iterator
	decltype(std::as_const(voxels).begin()) begin() const{
		return voxels.begin();
	}
	decltype(std::as_const(voxels).end()) end() const {
		return voxels.end();
	}
};

class PointLightsBuffera : public IAResource<PointLightLength, PointLightInfo> {
public:
	size_t count = 0;
	void addLights(VulkanCore core, VkCommandPool pool, PointLightInfo* lights, size_t lightsCount) {
		PointLightLength length;
		length.length = this->count + lightsCount;
		this->updateWhole(core, pool, length, lights, lightsCount, this->count);
		this->count += lightsCount;
	}
};

class VoxelChunkBuffer : public IAResource<NullResourceInfo, int> {
public:
	// todo for better performance,
	// it would be okay to maintain a staging buffer with the size of one chunk
	// and use the same staging buffer every time, no more allocations
	// to implement add an overload to the updateArray func which accepts staging buffer

	void setChunks(VoxelChunk* chunks, size_t count) {
		VulkanUtils& utils= VulkanUtils::utils();
		for (size_t i = 0; i < count; i++) {
			this->updateArray(
				utils.getCore(), utils.getCommandPool(),
				chunks[i].data(), VoxelChunk::chunkVoxelCount,
				VoxelChunk::chunkVoxelCount * i
			);
		}
	}

	void updateChunk(VoxelChunk& chunk, size_t chunkIndex) {
		VulkanUtils& utils = VulkanUtils::utils();
		this->updateArray(
			utils.getCore(), utils.getCommandPool(),
			chunk.data(), VoxelChunk::chunkVoxelCount * sizeof(int),
			VoxelChunk::chunkVoxelCount * sizeof(int) * chunkIndex
		);
	}
};


typedef glm::ivec4 VoxelDrawInfo;

struct alignas(sizeof(VoxelDrawInfo)) VoxelDrawCount {
	uint32_t    vertexCount;
	uint32_t    instanceCount;
	uint32_t    firstVertex;
	uint32_t    firstInstance;
};

class VoxelDrawBuffer : public IAResource<VoxelDrawCount, VoxelDrawInfo> {
public:
	void initializeCountToZero() {
		//set count to zero
		VoxelDrawCount zeroDrawCount = { 36, 0, 0, 0 };
		this->updateBase(
			VulkanUtils::utils().getCore(),
			VulkanUtils::utils().getCommandPool(),
			zeroDrawCount
		);
	}
	//no need to have any add/update functions as this is only written and read from the gpu
};

class VoxelRenderer {
private:
	VulkanCore core;

	//the global descriptor sets
	// matrices, lights, materials
	std::array<VkDescriptorSetLayout, 3> descriptorLayouts;

	struct {
		VkPipeline pipeline;
		VkPipelineLayout layout;
		//VkRenderPass renderpass_internal;
	} draw;

	struct {
		VkPipeline pipeline;
		VkPipelineLayout layout;
	} compute;

	Mesh cubeMesh;

	void createDrawPipeline(VkRenderPass renderpass, VkDescriptorSetLayout voxelDrawBufferLayout) {
		//pipeline creation
		VulkanUtils& utils = VulkanUtils::utils();
		auto vertShaderCode = utils.compileGlslToSpv("Shaders/voxel.vert", shaderc_shader_kind::shaderc_vertex_shader);
		auto fragShaderCode = utils.compileGlslToSpv("Shaders/voxel.frag", shaderc_shader_kind::shaderc_fragment_shader);

		VkShaderModule vertShaderModule = utils.createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = utils.createShaderModule(fragShaderCode);

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
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;

		auto attributeDescriptions = VertexInputDescription::getAttributeDescriptions();
		vertexInputInfo.vertexAttributeDescriptionCount = 3;
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
		rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
		rasterizer.lineWidth = 1.0f;
		//todo change to cull 3 out of 6 faces of a cube
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
		pipelineLayoutInfo.setLayoutCount = 4;
		VkDescriptorSetLayout layouts[4] = {
			descriptorLayouts[0],
			descriptorLayouts[1],
			descriptorLayouts[2],
			voxelDrawBufferLayout
		};
		pipelineLayoutInfo.pSetLayouts = layouts;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		this->draw.layout = core->createPipelineLayout(pipelineLayoutInfo);

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

		pipelineInfo.layout = this->draw.layout;

		pipelineInfo.renderPass = renderpass;
		pipelineInfo.subpass = 1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		this->draw.pipeline = core->createGraphicsPipeline(pipelineInfo);

		vkDestroyShaderModule(core->device, fragShaderModule, nullptr);
		vkDestroyShaderModule(core->device, vertShaderModule, nullptr);
	}

	void createComputePipeline(
		VkDescriptorSetLayout chunkBufferDescriptorLayout,
		VkDescriptorSetLayout voxelDrawBufferLayout
	) {
		//pipeline creation
		VulkanUtils& utils = VulkanUtils::utils();
		auto computeShaderCode = utils.compileGlslToSpv("shaders/voxel.comp", shaderc_shader_kind::shaderc_compute_shader);
		VkShaderModule computeShaderModule = utils.createShaderModule(computeShaderCode);

		VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = computeShaderModule;
		computeShaderStageInfo.pName = "main";

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 2;
		VkDescriptorSetLayout layouts[2] = {
			chunkBufferDescriptorLayout,
			voxelDrawBufferLayout
		};
		pipelineLayoutInfo.pSetLayouts = layouts;

		//VkPushConstantRange pushConstantRange{};
		//pushConstantRange.offset = 0;
		//pushConstantRange.size = sizeof(glm::vec4);
		//pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		pipelineLayoutInfo.pushConstantRangeCount = 0;
		this->compute.layout = core->createPipelineLayout(pipelineLayoutInfo);

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = this->compute.layout;
		pipelineInfo.stage = computeShaderStageInfo;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		this->compute.pipeline = core->createComputePipeline(pipelineInfo);

		vkDestroyShaderModule(core->device, computeShaderModule, nullptr);
	}


public:
	VoxelRenderer() = default;
	void init(
		std::array<VkDescriptorSetLayout, 3> globalDescLayouts,
		VkDescriptorSetLayout chunkBufferDescriptorLayout,
		VkDescriptorSetLayout voxelDrawBufferLayout,
		VkRenderPass renderpass_ext
	){
		this->core = VulkanUtils::utils().getCore();
		descriptorLayouts = globalDescLayouts;
		createDrawPipeline(renderpass_ext, voxelDrawBufferLayout);
		createComputePipeline(chunkBufferDescriptorLayout, voxelDrawBufferLayout);
		
		std::vector<unsigned int>indices(36);
		std::iota(indices.begin(), indices.end(), 0);
		MeshData<Vertex3> cubeMeshData(getCubeVertices(), indices);
		cubeMesh = Mesh(core, VulkanUtils::utils().getCommandPool(), cubeMeshData);
	};

	VkPipelineLayout getDrawPipelineLayout() {
		return draw.layout;
	}

	void recordRenderCommands(
		VkCommandBuffer& commandBuffer,
		VoxelDrawBuffer& vdb, VkDescriptorSet vdbDescriptorSet,
		VkViewport viewport, VkRect2D scissor) {
		//ensure renderpass is already started
		//ensure that all 3 descriptors global, lights, materials are bound before calling

		VkDebugUtilsLabelEXT label{};
		label.pLabelName = "Voxel Render start";
		label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		label.color[1] = 1.0;
		label.color[3] = 1.0;
		core->VkDebugUtils.CmdBeginDebugUtilsLabelEXT(commandBuffer, &label);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.pipeline);

		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS, draw.layout,
			3, 1, &vdbDescriptorSet,
			0, nullptr
		);

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		//make a model view matrix for rendering the object
		//camera position

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cubeMesh.vertexBuffer->buffer, &offset);
		vkCmdDrawIndirect(
			commandBuffer,
			vdb.resourceBuf->buffer,
			vdb.getBaseInfoOffset(),
			1,
			0	//stride does not matter here
		);
		core->VkDebugUtils.CmdEndDebugUtilsLabelEXT(commandBuffer);
	}

	void recordComputeCommands(
		VkCommandBuffer& commandBuffer,
		VoxelChunkBuffer& vcb, VkDescriptorSet vcbDescriptorSet,
		VoxelDrawBuffer& vdb, VkDescriptorSet vdbDescriptorSet
	){
		vdb.initializeCountToZero();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
		VkDescriptorSet descriptors[2] = { vcbDescriptorSet, vdbDescriptorSet };
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			compute.layout, 0, 2, descriptors,
			0, 0
		);

		vkCmdDispatch(commandBuffer, 4, VoxelChunk::chunkSize.y, 4);
	}

};