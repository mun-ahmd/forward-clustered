#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <assert.h>

constexpr int chunkWidth = 16;
constexpr int chunkHeight = 256;
constexpr int chunkDepth = 16;

constexpr bool checkVoxelChunkBounds(int x, int y, int z) {
	bool xOK = (x < (chunkWidth / 2 - 1)) || (x > chunkWidth / 2);
	bool yOK = (y < (chunkHeight / 2 - 1)) || (y > chunkHeight / 2);
	bool zOK = (z < (chunkDepth / 2 - 1)) || (z > chunkDepth / 2);
	return xOK && yOK && zOK;
}

class VoxelChunk {
	std::unique_ptr<int[]> voxels;

public:
	VoxelChunk() :
		voxels(std::make_unique<int[]>(
			chunkWidth * chunkHeight * chunkDepth
		)) {}

	inline size_t getIndex(int x, int y, int z) const{
		return (z + chunkDepth / 2) * chunkHeight * chunkWidth + (y + chunkHeight / 2) * chunkWidth + (x + chunkWidth / 2);
	}

	inline int& operator()(int x, int y, int z) {
		assert(checkVoxelChunkBounds(x, y, z) && "Voxel Coordinates Out of Bounds!");
		return voxels[getIndex(x, y, z)];
	}

	inline int operator()(int x, int y, int z) const{
		assert(checkVoxelChunkBounds(x, y, z) && "Voxel Coordinates Out of Bounds!");
		return voxels[getIndex(x, y, z)];
	}
};

//
//class CubeVoxel
//{
//	glm::ivec4 pos_mat;
//
//public:
//	CubeVoxel(glm::ivec4 pos_mat) : pos_mat(pos_mat) {}
//	CubeVoxel() : CubeVoxel({0, 0, 0, -1}) {}
//	CubeVoxel(glm::ivec3 pos, int mat) : CubeVoxel(glm::ivec4(pos, mat)) {}
//
//	inline void setPos(glm::ivec3 pos) {
//		this->pos_mat = glm::ivec4(pos, this->pos_mat.w);
//	}
//	inline glm::ivec3 getPos() {
//		return this->pos_mat;
//	}
//	inline void setMatID(int id){
//		this->pos_mat.w = id;
//	}
//	inline int getMatID() {
//		return this->pos_mat.w;
//	}
//
//	inline glm::ivec4 get() {
//		return pos_mat;
//	}
//
//	inline void set(glm::ivec4 pos_mat_in) {
//		this->pos_mat = pos_mat_in;
//	}
//};
//
//typedef CubeVoxel CubeVoxelInfo;
//
//struct alignas(sizeof(CubeVoxelInfo)) CubeVoxelLength {
//	int length;
//};
//
//class CubeVoxelBuffer : public IAResource<CubeVoxelLength, CubeVoxelInfo> {
//public:
//	size_t count = 0;
//	void addCubes(VulkanCore core, VkCommandPool pool, CubeVoxelInfo* cubes, size_t cubesCount) {
//		CubeVoxelLength length{ (this->count + cubesCount) };
//		this->updateWhole(core, pool, length, cubes, cubesCount, this->count);
//		this->count += cubesCount;
//	}
//};
//
//class CubeVoxelMinimizerCompute {
//private:
//	VkPipelineLayout layout;
//	VkPipeline pipe;
//	VulkanCore core;
//public:
//	CubeVoxelMinimizerCompute() {};
//	void init(VulkanCore core_in) {
//		core = core_in;
//		//pipeline creation
//		VkShaderModule  computeShaderModule = glslCompileFunc(
//			"shaders/clusters.comp",
//			shaderc_shader_kind::shaderc_compute_shader
//		);
//
//		VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
//		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
//		computeShaderStageInfo.module = computeShaderModule;
//		computeShaderStageInfo.pName = "main";
//
//		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//		pipelineLayoutInfo.setLayoutCount = 3;
//		VkDescriptorSetLayout layouts[3] = {
//			core->getLayout(frames.front().data.globalDS),
//			core->getLayout(frames.front().data.pointLightsDS),
//			core->getLayout(frames.front().data.clusterCompDS)
//		};
//		pipelineLayoutInfo.pSetLayouts = layouts;
//
//		VkPushConstantRange pushConstantRange{};
//		pushConstantRange.offset = 0;
//		pushConstantRange.size = sizeof(glm::vec4);
//		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
//		pipelineLayoutInfo.pushConstantRangeCount = 1;
//		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
//		this->clusterComp.layout = core->createPipelineLayout(pipelineLayoutInfo);
//
//		VkComputePipelineCreateInfo pipelineInfo{};
//		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
//		pipelineInfo.layout = this->clusterComp.layout;
//		pipelineInfo.stage = computeShaderStageInfo;
//
//		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
//		pipelineInfo.basePipelineIndex = -1; // Optional
//
//		clusterComp.pipe = core->createComputePipeline(pipelineInfo);
//
//		vkDestroyShaderModule(core->device, computeShaderModule, nullptr);
//	}
//};
//
//class CubeVoxelRenderer {
//
//};