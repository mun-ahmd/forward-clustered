#pragma once
#include "core.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct VertexInputDescription {
	//helper class to describe vertex input
	//limits usable vertex type to a single type for the application
	//eases creating pipelines
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex3);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex3, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex3, norm);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex3, uv);
		return attributeDescriptions;
	}
};

const std::vector<float> vertices = {
	// positions          // normals           // texture coords
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
};

std::vector<Vertex3> getCubeVertices() {
	auto start = reinterpret_cast<const Vertex3*>(vertices.data());
	return std::vector<Vertex3>(start, start + vertices.size());
}

struct BufferCopyInfo {
	VkBuffer src;
	VkBuffer dst;
	VkBufferCopy inf;

	BufferCopyInfo() = default;
	BufferCopyInfo(VkBuffer src, VkBuffer dst, VkBufferCopy inf) : src(src), dst(dst), inf(inf) {}
};

struct MeshPushConstants {
	glm::mat4 transform;
};

class Buffer {
public:
	VkBuffer buffer;
	VmaAllocation allocation;

	static std::shared_ptr<Buffer> create(VulkanCore core,
		VkDeviceSize size,
		VkBufferUsageFlags usage, VmaAllocationCreateFlagBits vmaFlags,
		VkMemoryPropertyFlags requiredMemoryTypeFlags, VkMemoryPropertyFlags preferredMemoryTypeFlags = 0)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = vmaFlags;
		allocInfo.requiredFlags = requiredMemoryTypeFlags;
		allocInfo.preferredFlags = preferredMemoryTypeFlags;

		Buffer buf;
		if (vmaCreateBuffer(core->allocator, &bufferInfo, &allocInfo, &buf.buffer, &buf.allocation, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}
		return std::shared_ptr<Buffer>(new Buffer(buf),
			[core](Buffer* buf) {
				vmaDestroyBuffer(core->allocator, buf->buffer, buf->allocation);
				delete buf;
			}
		);
	}
};

void copyBuffer(VulkanCore core, VkCommandPool commandPool, std::vector<BufferCopyInfo> buffers) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = core->beginSingleTimeCommands(commandPool);
	vkAllocateCommandBuffers(core->device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	for (auto& copyInfo : buffers) {
		vkCmdCopyBuffer(commandBuffer, copyInfo.src, copyInfo.dst, 1, &copyInfo.inf);
	}

	core->endSingleTimeCommands(commandPool, commandBuffer);
}

auto prepareStagingBuffer(VulkanCore core, const void* data, size_t dataSize) {
	auto stagingBuffer = Buffer::create(
		core, dataSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);
	void* stagingBufferData;
	vmaMapMemory(core->allocator, stagingBuffer->allocation, &stagingBufferData);
	memcpy(stagingBufferData, data, dataSize);
	vmaUnmapMemory(core->allocator, stagingBuffer->allocation);
	return stagingBuffer;
}

class Image {
public:
	VkImage image;
	VmaAllocation allocation;

	static std::shared_ptr<Image> create(VulkanCore core) {

	}
};

template<class ResourceInfo>
class Resource {
private:
	uint32_t index;
public:
	ResourceInfo resourceInfo;

	inline static std::shared_ptr<Buffer> resourceBuf;
	inline static std::shared_ptr<Buffer> stagingBuf;
	inline static void* stagingBufMap;
	inline static uint32_t storedResources;
	inline static uint32_t paddedElementSize;
	inline static uint32_t maximumResources;

	static void init(VulkanCore core, uint32_t maximumResourcesValue = 100) {
		maximumResources = maximumResourcesValue;
		paddedElementSize =
			core->gpuProperties.limits.minUniformBufferOffsetAlignment > sizeof(ResourceInfo) ?
			core->gpuProperties.limits.minUniformBufferOffsetAlignment :
			sizeof(ResourceInfo) / core->gpuProperties.limits.minUniformBufferOffsetAlignment + 1;

		resourceBuf = Buffer::create(
			core,
			maximumResources * paddedElementSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 0
		);
		storedResources = 0;
		stagingBuf = Buffer::create(
			core,
			sizeof(ResourceInfo),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			(VmaAllocationCreateFlagBits)(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT),
			0
		);
		stagingBufMap = stagingBuf->allocation->GetMappedData();
	}

	uint32_t getResourceOffset() {
		return Resource<ResourceInfo>::paddedElementSize * this->index;
	}

	void makeResource(VulkanCore core, VkCommandPool pool, ResourceInfo info) {
		this->index = storedResources;
		storedResources++;
		memcpy(stagingBufMap, &info, sizeof(info));
		VkBufferCopy copier{};
		copier.srcOffset = 0;
		copier.size = sizeof(ResourceInfo);
		copier.dstOffset = this->index * (paddedElementSize);
		copyBuffer(core, pool, { BufferCopyInfo(stagingBuf->buffer, resourceBuf->buffer, copier) });
		this->resourceInfo = info;
	}

	static VkDescriptorSet createDescriptor(VulkanCore core) {
		VkDescriptorSetLayoutBinding binding0{};
		binding0.binding = 0;
		binding0.descriptorCount = 1;
		binding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		binding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		auto descriptor = core->createDescriptorSet({ binding0 });

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		write.dstSet = descriptor;
		write.dstBinding = 0;
		VkDescriptorBufferInfo inf{};
		inf.buffer = resourceBuf->buffer;
		inf.offset = 0;
		inf.range = sizeof(ResourceInfo);
		write.pBufferInfo = &inf;
		vkUpdateDescriptorSets(core->device, 1, &write, 0, nullptr);

		return descriptor;
	}
};

struct MaterialInfo {
	glm::vec4 diffuseColor;
	glm::vec4 specularGlossiness;
};

class Material : public Resource<MaterialInfo> {
public:
	static std::shared_ptr<Material> create(VulkanCore core, VkCommandPool pool, MaterialInfo info) {
		Material mat{};
		mat.makeResource(core, pool, info);
		return std::make_shared<Material>(mat);
	}
};

inline MaterialInfo randomMaterial() {
	auto r = []() {
		return (float)std::rand() / (float)RAND_MAX;
	};
	auto mat = MaterialInfo{ glm::vec4(r(), r(), r(), r()), glm::vec4(r(), r(), r(), r()) };
	return mat;
}

struct PointLightInfo {
	glm::vec4 color;
	glm::vec3 position;
	float radius;
};

class PointLight : public Resource<PointLightInfo> {
public:
	static std::shared_ptr<PointLight> create(VulkanCore core, VkCommandPool pool, PointLightInfo info) {
		PointLight light{};
		light.makeResource(core, pool, info);
		return std::make_shared<PointLight>(light);
	}
};

class Mesh {
public:
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indexBuffer;
	uint32_t numIndices;
	Mesh() = default;
	Mesh(VulkanCore core, const VkCommandPool commandPool, MeshData<Vertex3>& meshData) {
		const auto& vertexData = meshData.vertices;
		size_t vertexDataSize = vertexData.size() * sizeof(Vertex3);
		this->vertexBuffer = Buffer::create(
			core, vertexDataSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			(VmaAllocationCreateFlagBits)0,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		auto vertexStagingBuffer = prepareStagingBuffer(core, vertexData.data(), vertexDataSize);


		std::vector<uint16_t> indexData(meshData.indices.begin(), meshData.indices.end());
		size_t indexDataSize = indexData.size() * sizeof(uint16_t);
		this->numIndices = indexData.size();
		this->indexBuffer = Buffer::create(
			core, indexDataSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			(VmaAllocationCreateFlagBits)0,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		auto indexStagingBuffer = prepareStagingBuffer(core, indexData.data(), indexDataSize);

		VkBufferCopy copier{};
		copier.dstOffset = 0; copier.srcOffset = 0;
		copier.size = vertexDataSize;
		BufferCopyInfo vBufferCI(vertexStagingBuffer->buffer, vertexBuffer->buffer, copier);
		copier.size = indexDataSize;
		BufferCopyInfo iBufferCI(indexStagingBuffer->buffer, indexBuffer->buffer, copier);

		copyBuffer(core, commandPool, { vBufferCI, iBufferCI });
	}
};