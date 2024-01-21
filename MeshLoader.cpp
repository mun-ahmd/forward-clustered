#include <algorithm>
#include <iostream>
#include <array>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <numeric>
#include <vector>
#include <algorithm>
#include <any>
#include <filesystem>
#include <set>
#include <optional>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "json.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
using json = nlohmann::json;

#include "MeshLoader.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

void loadOBJ(const char* filepath);

class ModelInterface {
public:
	template<typename T>
	class VectorInterface {
		// user must ensure ptr is alive while this is in use
	private:
		T* ptr;
		size_t len;
	public:
		VectorInterface(T* ptr, size_t len) : ptr(ptr), len(len) {};
		VectorInterface() : VectorInterface(nullptr, 0) {};

		inline T& operator[](size_t index) {
			assert(index < len && "Index out of bounds");
			return ptr[index];
		}
		inline const T& operator[](size_t index) const {
			assert(index < len && "Index out of bounds");
			return ptr[index];
		}
		inline bool empty() { return len == 0; }
		inline size_t size() { return len; }
		inline T* data() { return ptr; }
		inline const T* data() const { return ptr; }
		inline T* begin() { return ptr; }
		inline const T* begin() const { return ptr; }
		inline T* end() { return ptr + len; }
		inline const T* end() const { return ptr + len; }
	};

	ModelInterface() = default;
	ModelInterface(cgltf_data* data) :
		accessors(data->accessors, data->accessors_count), animations(data->animations, data->animations_count), buffers(data->buffers, data->buffers_count), bufferViews(data->buffer_views, data->buffer_views_count), materials(data->materials, data->materials_count), meshes(data->meshes, data->meshes_count), nodes(data->nodes, data->nodes_count), textures(data->textures, data->textures_count), images(data->images, data->images_count), skins(data->skins, data->skins_count), samplers(data->samplers, data->samplers_count), cameras(data->cameras, data->cameras_count), scenes(data->scenes, data->scenes_count), lights(data->lights, data->lights_count)
	{};

	VectorInterface<cgltf_accessor> accessors;
	VectorInterface<cgltf_animation> animations;
	VectorInterface<cgltf_buffer> buffers;
	VectorInterface<cgltf_buffer_view> bufferViews;
	VectorInterface<cgltf_material> materials;
	VectorInterface<cgltf_mesh> meshes;
	VectorInterface<cgltf_node> nodes;
	VectorInterface<cgltf_texture> textures;
	VectorInterface<cgltf_image> images;
	VectorInterface<cgltf_skin> skins;
	VectorInterface<cgltf_sampler> samplers;
	VectorInterface<cgltf_camera> cameras;
	VectorInterface<cgltf_scene> scenes;
	VectorInterface<cgltf_light> lights;
};


struct AccessorInfo {
	cgltf_accessor accessor;
	cgltf_buffer_view bufferView;
	cgltf_buffer buffer;
	unsigned char* data;
};

inline AccessorInfo loadAccessor(ModelInterface& model, cgltf_accessor* accessor) {
	return AccessorInfo{
		*accessor,
		*accessor->buffer_view,
		*accessor->buffer_view->buffer,
		//if buffer view data is not null, use it, otherwise use buffer data
		//read cgltf buffer_view class code to understand
		reinterpret_cast<unsigned char*>(
			(accessor->buffer_view->data == NULL) ? 
			(accessor->buffer_view->buffer->data) : (accessor->buffer_view->data)
		)
	};
}

glm::vec3 coordinateSystemCorrection(glm::vec3 vec) {
	return glm::vec3(vec.x, -vec.y, vec.z);
}

std::vector<std::vector<std::pair<MeshData<Vertex3>, int>>> loadMeshes(ModelInterface& model) {
	std::vector<std::vector<std::pair<MeshData<Vertex3>, int>>> meshes;
	size_t num_total_primitives = 0;
	meshes.reserve(model.meshes.size());	
	//std::array<std::string, 3> required = { "POSITION", "NORMAL", "TEXCOORD_0" };
	
	std::array<cgltf_attribute_type, 3> requiredAttributeTypes = {
		cgltf_attribute_type_position,
		cgltf_attribute_type_normal,
		cgltf_attribute_type_texcoord
	};
	std::array<AccessorInfo, 3> accessorsInfo;

	std::vector<AccessorInfo> preloadedAccessorInfo(model.accessors.size());

	for (auto& mesh : model.meshes) {
		meshes.push_back({});
		meshes.back().reserve(mesh.primitives_count);
		for (size_t mesh_primitive_index = 0; mesh_primitive_index < mesh.primitives_count; mesh_primitive_index++) {
			auto& primitive = mesh.primitives[mesh_primitive_index];
			MeshData<Vertex3> data;
			// todo make sure primitive mode = 4 (TRIANGLES)
			if (primitive.type != cgltf_primitive_type_triangles) {
				//! panic
				continue;
			}

			for (int i = 0; i < 3; ++i) {
				int attributeIndex = -1;
				for (int j = 0; j < primitive.attributes_count; j++) {
					if (primitive.attributes[j].type == requiredAttributeTypes[i]) {
						attributeIndex = j;
						break;
					}
				}
				if (attributeIndex == -1){
					throw std::runtime_error("Could not find required attribute in primitive");
						//! panic
				}

				accessorsInfo[i] =
					loadAccessor(model, primitive.attributes[attributeIndex].data);
					if (accessorsInfo[i].accessor.component_type != cgltf_component_type_r_32f) {
						throw std::runtime_error("Primitive accessor component type not supported");
							//! panic
					}
			}

			unsigned char* position = accessorsInfo[0].data +
				accessorsInfo[0].bufferView.offset +
				accessorsInfo[0].accessor.offset;
			int position_stride = accessorsInfo[0].bufferView.stride == 0
				? sizeof(glm::vec3)
				: accessorsInfo[0].bufferView.stride;

			unsigned char* normal = accessorsInfo[1].data +
				accessorsInfo[1].bufferView.offset +
				accessorsInfo[1].accessor.offset;
			int normal_stride = accessorsInfo[1].bufferView.stride == 0
				? sizeof(glm::vec3)
				: accessorsInfo[1].bufferView.stride;

			unsigned char* uv = accessorsInfo[2].data +
				accessorsInfo[2].bufferView.offset +
				accessorsInfo[2].accessor.offset;
			int uv_stride = accessorsInfo[2].bufferView.stride == 0
				? sizeof(glm::vec2)
				: accessorsInfo[2].bufferView.stride;

			data.vertices.reserve(accessorsInfo.front().accessor.count);
			for (int i = 0; i < accessorsInfo.front().accessor.count; ++i) {
				Vertex3 curr;
				memcpy(&curr.pos, position, sizeof(glm::vec3));
				curr.pos = coordinateSystemCorrection(curr.pos);
				position += position_stride;
				memcpy(&curr.norm, normal, sizeof(glm::vec3));
				normal += normal_stride;
				memcpy(&curr.uv, uv, sizeof(glm::vec2));
				uv += uv_stride;
				data.vertices.push_back(curr);
			}
			auto indicesAccessorInfo = loadAccessor(model, primitive.indices);

			if (indicesAccessorInfo.accessor.component_type == cgltf_component_type_r_8u) {
				auto bufferIterator = (
					reinterpret_cast<unsigned char*>(indicesAccessorInfo.buffer.data)
					+ indicesAccessorInfo.accessor.offset
					+ indicesAccessorInfo.bufferView.offset
				);
				data.indices = std::vector<unsigned int>(
					bufferIterator,
					bufferIterator + indicesAccessorInfo.accessor.count);
			}
			else if (indicesAccessorInfo.accessor.component_type == cgltf_component_type_r_16u) {
				unsigned short* buffer = reinterpret_cast<unsigned short*>(
						(reinterpret_cast<unsigned char*>(indicesAccessorInfo.buffer.data)
						+ indicesAccessorInfo.accessor.offset
						+ indicesAccessorInfo.bufferView.offset)
					);
				data.indices = std::vector<unsigned int>(
					buffer, buffer + indicesAccessorInfo.accessor.count);
			}
			else if (indicesAccessorInfo.accessor.component_type == cgltf_component_type_r_32u) {
				unsigned int* buffer = reinterpret_cast<unsigned int*>(
					(reinterpret_cast<unsigned char*>(indicesAccessorInfo.buffer.data)
						+ indicesAccessorInfo.accessor.offset
						+ indicesAccessorInfo.bufferView.offset)
					);
				data.indices = std::vector<unsigned int>(
					buffer, buffer + indicesAccessorInfo.accessor.count);
			}
			else {
				// invalid component type for indices !panic
			}
			meshes.back().push_back(
				std::make_pair(
					std::move(data),
					primitive.material - model.materials.begin()
				)
			);
		}
	}
	return meshes;
}

inline std::string texturePathHelper(std::string& gltfFilePath, json& gltf, json& tex) {
	json baseColorTexture =
		(gltf["textures"][tex["index"]
			.get<int>()]);
	return std::filesystem::path(gltfFilePath).parent_path().string() +
		static_cast<char>(std::filesystem::path::preferred_separator) +
		gltf["images"][baseColorTexture["source"].get<int>()]["uri"]
		.get<std::string>();
}

std::vector<MaterialPBR> loadMaterials(std::string gltfFile) {
	std::vector<MaterialPBR> materials;
	std::ifstream file(gltfFile);
	json gltf = json::parse(file);
	json materials_gltf = gltf["materials"];
	std::string materialname = gltf["materials"][0]["name"].get<std::string>();
	for (auto mat = materials_gltf.begin(); mat != materials_gltf.end(); mat++) {
		MaterialPBR curr;
		if (mat->contains("normalTexture")) {
			curr.normalMap = texturePathHelper(gltfFile, gltf, (*mat)["normalTexture"]);
			if ((*mat)["normalTexture"].contains("scale")) {
				curr.normalScale = (*mat)["normalTexture"]["scale"].get<float>();
			}
		}
		if (mat->contains("pbrMetallicRoughness")) {
			MetallicRoughnessMat MRMat;
			auto currMetallicRoughness = (*mat)["pbrMetallicRoughness"];
			if (currMetallicRoughness.contains("baseColorTexture")) {
				MRMat.baseColorTex = texturePathHelper(gltfFile, gltf, currMetallicRoughness["baseColorTexture"]);
			}
			if (currMetallicRoughness.contains("metallicRoughnessTexture")) {
				MRMat.metallicRoughnessTex = texturePathHelper(gltfFile, gltf, currMetallicRoughness["metallicRoughnessTexture"]);
			}
			if (currMetallicRoughness.contains("baseColorFactor")) {
				auto baseColor =
					currMetallicRoughness["baseColorFactor"].get<std::vector<float>>();
				memcpy(&MRMat.baseColorFactor, baseColor.data(),
					sizeof(MRMat.baseColorFactor));
			}
			if (currMetallicRoughness.contains("metallicFactor")) {
				MRMat.metallic_factor =
					currMetallicRoughness["metallicFactor"].get<float>();
			}
			if (currMetallicRoughness.contains("roughnessFactor")) {
				MRMat.roughness_factor =
					currMetallicRoughness["roughnessFactor"].get<float>();
			}
			curr.isMetallicRoughness = true;
			curr.metallicRoughness = MRMat;
		}
		else if (mat->contains("extensions") && (*mat)["extensions"].contains("KHR_materials_pbrSpecularGlossiness")) {
			DiffuseSpecularMat DFMat;
			auto diffSpec = (*mat)["extensions"]["KHR_materials_pbrSpecularGlossiness"];
			if (diffSpec.contains("diffuseTexture")) {
				DFMat.diffuseTex = texturePathHelper(gltfFile, gltf, diffSpec["diffuseTexture"]);
			}
			if (diffSpec.contains("specularGlossinessTexture")) {
				DFMat.specularGlossTex = texturePathHelper(gltfFile, gltf, diffSpec["specularGlossinessTexture"]);
			}
			if (diffSpec.contains("diffuseFactor")) {
				auto diffFactor =
					diffSpec["diffuseFactor"].get<std::vector<float>>();
				memcpy(&DFMat.diffuseFactor, diffFactor.data(),
					sizeof(DFMat.diffuseFactor));
			}
			if (diffSpec.contains("specularFactor")) {
				auto specularFactor =
					diffSpec["specularFactor"].get<std::vector<float>>();
				memcpy(&DFMat.specularFactor, specularFactor.data(),
					sizeof(DFMat.specularFactor));
			}
			if (diffSpec.contains("glossinessFactor")) {
				DFMat.glossinessFactor = diffSpec["glossinessFactor"].get<float>();
			}
			curr.isMetallicRoughness = false;
			curr.diffuseSpecular = DFMat;
		}
		materials.push_back(curr);
	}
	return materials;
}

glm::mat4 getNodeLocalTransform(cgltf_node* node) {
	//returns local transformation of a node
	glm::mat4 localTransform;
	if (node->has_matrix) {
		//throw std::runtime_error("it had matrix your honor");
		localTransform = glm::make_mat4x4(node->matrix);
	}
	else {
		localTransform = glm::mat4(1.0f);
		if (node->has_translation) {
			glm::vec3 translation = coordinateSystemCorrection(glm::make_vec3(node->translation));
			localTransform = glm::translate(localTransform, translation);
		}
		if (node->has_rotation) {
			glm::quat q = glm::quat(
				-node->rotation[3],
				node->rotation[0], -node->rotation[1], node->rotation[2]
			);
			localTransform *= glm::mat4(q);
		}
		if (node->has_scale) {
			glm::vec3 scale = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);
			localTransform = glm::scale(localTransform, scale);
		}
		//TRS order
		//write local transform to node for caching
		memcpy(node->matrix, &localTransform, sizeof(localTransform));
		node->has_matrix = 1;
	}
	return localTransform;
}

glm::mat4 getNodeGlobalTransform(cgltf_node* node) {
	//returns global transformation of a node
	//todo can optimize by caching global transform somewhere too if possible

	glm::mat4 globalTransform = getNodeLocalTransform(node);
	cgltf_node* curr = node->parent;
	while (curr != NULL) {
		globalTransform = getNodeLocalTransform(curr) * globalTransform;
		curr = curr->parent;
	}
	//std::cout << node->name << " -> xyz: (" << globalTransform[3][0] <<  ", " << globalTransform[3][1] << ", "  << globalTransform[3][2] << ")" << std::endl;
	return globalTransform;
}

std::optional<ModelData> loadGLTF(const char* filepath) {
	cgltf_options options{};
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data* data = NULL;

	cgltf_result result = cgltf_parse_file(&options, filepath, &data);
	if (result != cgltf_result_success) {
		return std::nullopt;
	}

	result = cgltf_load_buffers(&options, data, filepath);
	if (result != cgltf_result_success) {
		return std::nullopt;
	}

	/* TODO make awesome stuff */
	ModelInterface model = ModelInterface(data);
	auto loadedMeshes = loadMeshes(model);
	std::set<cgltf_mesh*> addedMeshes;

	ModelData modelData;
	modelData.materials = loadMaterials(filepath);

	modelData.meshData.meshes.reserve(loadedMeshes.size());
	modelData.meshData.transforms.reserve(loadedMeshes.size());
	modelData.meshData.matIndex.reserve(loadedMeshes.size());

	for (auto& node : model.nodes) {		
		if (node.mesh != nullptr && addedMeshes.find(node.mesh) == addedMeshes.end()) {
			int mIndex = node.mesh - model.meshes.begin();
			auto transform = getNodeGlobalTransform(&node);
			addedMeshes.insert(node.mesh);
			for (auto& primitive : loadedMeshes[mIndex]) {
				modelData.meshData.meshes.push_back(primitive.first);
				modelData.meshData.matIndex.push_back(primitive.second);
				modelData.meshData.transforms.push_back(transform);
			}
		}
	}
	cgltf_free(data);
	return modelData;
}
//
//#include "fast_obj.h"
//void loadOBJ(const char* filepath) {
//    auto loaded = fast_obj_read(filepath);
//    ModelData modelData;
//    modelData.materials.reserve(loaded->material_count);
//    for (int i = 0; i < loaded->material_count; i++) {
//        auto cats = loaded->materials[i];
//        int is = &cats - &cats;
//        MaterialPBR mat;
//        mat.isMetallicRoughness = false;
//        DiffuseSpecularMat dfMat;
//        dfMat.diffuseFactor = glm::vec4(glm::make_vec3(cats.Kd), 1.0);
//        if (cats.map_Kd.path) {
//            dfMat.diffuseTex = cats.map_Kd.path;
//        }
//        modelData.materials.push_back(mat)
////        modelData.materials.push_back();
//    }
//}