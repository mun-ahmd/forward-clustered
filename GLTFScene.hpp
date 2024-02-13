#pragma once
#include <vector>
#include <algorithm>
#include <functional>

#include "MeshLoader.hpp"
#include "ImageLoader.hpp"
#include "asyncImageLoader.hpp"

#include "mesh.hpp"
#include "material.hpp"
#include "light.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

constexpr unsigned int MAX_MATERIALS_COUNT = 1000;
constexpr unsigned int MAX_LIGHTS_COUNT = 10000;
//#define ADD_RANDOM_LIGHTS
//#define NO_TEXTURES

class GLTFScene {
private:
	uint64_t drawIndex = 0;

	RC<AsyncImageLoader> imageLoader;
	RC<Sampler> vSampler;

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
			VulkanUtils::utils().getCore(),
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

	void loadMaterials(std::vector<MaterialPBR>&& materialsIN) {
		materials.addMaterialImage(
			loadImage(
				R"(C:\Users\munee\source\repos\VulkanRenderer\3DModels\blankVaibhav.png)",
				4,
				true
			), vSampler
		);

		std::unordered_map<std::string, unsigned int> indexFromImagePath;
		indexFromImagePath[""] = 0;

		for (auto& mat : materialsIN) {
			assert(mat.isMetallicRoughness);
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
					indexFromImagePath.find(tex) == indexFromImagePath.end()
					) {
					indexFromImagePath[tex] =
						materials.addMaterialImage(
							loadImage(
								tex.c_str(),
								i == 0 ? 4 : 2,
								false,
								std::nullopt
							), vSampler
						);
				}
			}
		}

		for (MaterialPBR& loadedMat : materialsIN) {
			assert(loadedMat.isMetallicRoughness);
			MaterialInfo matInf{};
			matInf.baseColorFactor = loadedMat.metallicRoughness.baseColorFactor;
			matInf.metallicRoughness_waste2 =
				glm::vec4(
					glm::vec2(
						loadedMat.metallicRoughness.metallic_factor,
						loadedMat.metallicRoughness.roughness_factor
					), 1.0, 1.0
				);

			matInf.baseTexId_metallicRoughessTexId_waste2 =
				glm::uvec4(
					indexFromImagePath[
						loadedMat.metallicRoughness.baseColorTex
					],
					indexFromImagePath[
						loadedMat.metallicRoughness.metallicRoughnessTex
					],
					0,
					0
				);

			materials.addMaterial(VulkanUtils::utils().getCore(), VulkanUtils::utils().getCommandPool(), matInf);
		}
	}

#ifdef ADD_RANDOM_LIGHTS
	void addRandomLights() {
		while (this->pointLights.size() < MAX_LIGHTS_COUNT) {
			auto rm = randomMaterial();
			PointLightInfo light{};
			light.position = glm::normalize(glm::vec3(rm.baseColorFactor)) * (10.0f * rm.baseColorFactor.length());
			light.radius = 10.0f;
			light.color = rm.baseColorFactor;
			light.intensity = 10.0f;
			this->pointLights.push_back(light);
		}
	}
#endif

	static uint64_t calcMeshMat(uint64_t mesh, uint64_t mat) {
		return ((mesh << 32) & 0xFFFFFFFF00000000) | (mat & 0xFFFFFFFF);
	}


public:
	GLTFScene(RC<AsyncImageLoader> imageLoader) : imageLoader(imageLoader)
	{
		this->materials.init(MAX_MATERIALS_COUNT);
		this->materialsDescriptorSet = this->materials.getDescriptorSet();

		this->lightsBuffer = LightsBuffer(
			VulkanUtils::utils().getCore(),
			MAX_LIGHTS_COUNT,
			MAX_FRAMES_IN_FLIGHT
		);
	}
	LightsBuffer lightsBuffer;

	Materials materials;
	VkDescriptorSet materialsDescriptorSet;

	std::vector<Mesh> meshes;
	std::vector<GLTFDrawable> singularMeshes;
	std::vector<std::vector<GLTFDrawable>> instancedMeshes;

	std::vector<PointLightInfo> pointLights;
	DirectionalLightInfo directionalLight;

	static std::unique_ptr<GLTFScene> create(RC<AsyncImageLoader> imageLoader) {
		return std::make_unique<GLTFScene>(imageLoader);
	}

	void clearScene() {
		this->meshes.clear();
		this->meshes.shrink_to_fit();
		
		this->singularMeshes.clear();
		this->singularMeshes.shrink_to_fit();

		this->instancedMeshes.clear();
		this->instancedMeshes.shrink_to_fit();

		this->pointLights.clear();
		this->pointLights.shrink_to_fit();
		//this->directionalLight;

		this->materials.clear();
		this->lightsBuffer.clear();
	}

	void loadScene(const char* filepath)
	{
		clearScene();

		auto data = loadGLTF(filepath);
		assert(data.has_value() && "COULD NOT LOAD GLTFScene");

		//init sampler
		{
			this->vSampler = Sampler::create(VulkanUtils::utils().getCore(), Sampler::makeCreateInfo(
				{ VK_FILTER_LINEAR, VK_FILTER_LINEAR },
				{ VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
				{},
				VK_SAMPLER_MIPMAP_MODE_LINEAR,
				VulkanUtils::utils().getCore()->gpuProperties.limits.maxSamplerAnisotropy
			));
		}

		//init meshes
		{
			std::unordered_map<uint64_t, size_t> drawableIndexFromMeshMat;
			for (uint64_t mat = 0; mat < data->materials.size(); mat++) {
				for (uint64_t mesh = 0; mesh < data->meshes.size(); mesh++) {
					drawableIndexFromMeshMat[calcMeshMat(mesh, mat)] = this->instancedMeshes.size();
					this->instancedMeshes.push_back({});
				}
			}

			for (GLTFDrawable& drawable : data->drawables) {
				size_t index = drawableIndexFromMeshMat[calcMeshMat(drawable.mesh, drawable.material)];
				this->instancedMeshes[index].push_back(drawable);
			}

			std::vector<bool> shouldRemove(this->instancedMeshes.size(), false);
			for (size_t i = 0; i < this->instancedMeshes.size(); i++) {
				auto& instancedMesh = this->instancedMeshes[i];
				if (instancedMesh.size() == 0) {
					shouldRemove[i] = true;
				}
				else if (instancedMesh.size() == 1) {
					shouldRemove[i] = true;
					this->singularMeshes.push_back(instancedMesh[0]);
				}
			}

			{
				size_t i = 0;
				this->instancedMeshes.erase(
					std::remove_if(
						instancedMeshes.begin(), instancedMeshes.end(),
						[&i, &shouldRemove](const decltype(instancedMeshes)::value_type&)
						{ return shouldRemove[i++]; }
					),
					instancedMeshes.end()
				);
			}

			this->meshes.reserve(data->meshes.size());
			VulkanCore core = VulkanUtils::utils().getCore();
			VkCommandPool commandPool = VulkanUtils::utils().getCommandPool();
			for (auto& meshdata : data->meshes) {
				this->meshes.push_back(
					Mesh(
						core,
						commandPool,
						meshdata
					)
				);
			}
		}

		//init materials
		{
			this->loadMaterials(std::move(data->materials));
		}

		//init lights
		{
			this->directionalLight = data->directionalLight;
			this->pointLights = std::move(data->pointLights);

#ifdef ADD_RANDOM_LIGHTS
			addRandomLights();
#endif

			this->lightsBuffer.addPointLights(
				VulkanUtils::utils().getCore(),
				VulkanUtils::utils().getCommandPool(),
				this->pointLights.data(),
				this->pointLights.size()
			);

			for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				this->lightsBuffer.setSunLight(this->directionalLight, i);
			}
		}
	}

	void drawAll(
		VkCommandBuffer cmd,
		std::function<void(VkCommandBuffer, GLTFScene&, const GLTFDrawable&)> preDrawCallback
	) {
		VkDeviceSize offset = 0;
		for (auto& meshInfo : singularMeshes) {
			preDrawCallback(cmd, *this, meshInfo);
			Mesh& mesh = this->meshes[meshInfo.mesh];
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer->buffer, &offset);
			vkCmdBindIndexBuffer(cmd, mesh.indexBuffer->buffer, 0, mesh.indexType);
			vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);
		}
		for (auto& instancedMesh : instancedMeshes) {
			assert(instancedMesh.size() > 0);
			preDrawCallback(cmd, *this, instancedMesh[0]);
			Mesh& mesh = this->meshes[instancedMesh[0].mesh];
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer->buffer, &offset);
			vkCmdBindIndexBuffer(cmd, mesh.indexBuffer->buffer, 0, mesh.indexType);
			vkCmdDrawIndexed(cmd, mesh.numIndices, instancedMesh.size(), 0, 0, 0);
		}
	}

	uint64_t numDrawables() {
		return this->singularMeshes.size() + this->instancedMeshes.size();
	}
};