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
#include <glm/gtx/matrix_decompose.hpp>

#include "MeshLoader.hpp"

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
		inline size_t size() const { return len; }
		inline T* data() { return ptr; }
		inline const T* data() const { return ptr; }
		inline T* begin() { return ptr; }
		inline const T* begin() const { return ptr; }
		inline T* end() { return ptr + len; }
		inline const T* end() const { return ptr + len; }
	};

	ModelInterface() = default;
	ModelInterface(cgltf_data* data) :
		asset(data->asset), accessors(data->accessors, data->accessors_count), animations(data->animations, data->animations_count), buffers(data->buffers, data->buffers_count), bufferViews(data->buffer_views, data->buffer_views_count), materials(data->materials, data->materials_count), meshes(data->meshes, data->meshes_count), nodes(data->nodes, data->nodes_count), textures(data->textures, data->textures_count), images(data->images, data->images_count), skins(data->skins, data->skins_count), samplers(data->samplers, data->samplers_count), cameras(data->cameras, data->cameras_count), scenes(data->scenes, data->scenes_count), lights(data->lights, data->lights_count)
	{};

	cgltf_asset asset;
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

inline glm::vec3 coordinateSystemCorrection(glm::vec3 vec) {
	return glm::vec3(vec.x, -vec.y, vec.z);
}
inline glm::vec2 coordinateSystemCorrection(glm::vec2 vec) {
	return glm::vec2(vec.x, -vec.y);
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

	std::array<cgltf_type, 3> requiredAttributeVectorType = {
		cgltf_type_vec3,
		cgltf_type_vec3,
		cgltf_type_vec2
	};

	std::array<AccessorInfo, 3> accessorsInfo;

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
					if (accessorsInfo[i].accessor.type != requiredAttributeVectorType[i]) {
						throw std::runtime_error("Primitive accessor type not supported");
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
				position += position_stride;
				memcpy(&curr.norm, normal, sizeof(glm::vec3));
				normal += normal_stride;
				memcpy(&curr.uv, uv, sizeof(glm::vec2));
				uv += uv_stride;

				curr.pos = coordinateSystemCorrection(curr.pos);
				curr.norm = coordinateSystemCorrection(curr.norm);
				//curr.uv = coordinateSystemCorrection(curr.uv);

				data.vertices.push_back(curr);
			}

			auto indicesAccessorInfo = loadAccessor(model, primitive.indices);

			if (indicesAccessorInfo.accessor.component_type == cgltf_component_type_r_8u) {

				uint32_t indicesStride = indicesAccessorInfo.accessor.buffer_view->stride == 0 ?
					indicesAccessorInfo.accessor.stride : indicesAccessorInfo.accessor.buffer_view->stride;

				unsigned char* dataPtr = (
					indicesAccessorInfo.data + indicesAccessorInfo.bufferView.offset + indicesAccessorInfo.accessor.offset
					);

				data.indices = std::vector<unsigned int>(indicesAccessorInfo.accessor.count);
				for (unsigned int i = 0; i < indicesAccessorInfo.accessor.count; i++) {
					unsigned int curr = *dataPtr;
					data.indices[i] = curr;
					dataPtr += indicesStride;
				}
			}
			else if (indicesAccessorInfo.accessor.component_type == cgltf_component_type_r_16u) {
				uint32_t indicesStride = indicesAccessorInfo.accessor.buffer_view->stride == 0 ?
					indicesAccessorInfo.accessor.stride : indicesAccessorInfo.accessor.buffer_view->stride;

				unsigned char* dataPtr = (
					indicesAccessorInfo.data + indicesAccessorInfo.bufferView.offset + indicesAccessorInfo.accessor.offset
					);

				data.indices = std::vector<unsigned int>(indicesAccessorInfo.accessor.count);
				for (unsigned int i = 0; i < indicesAccessorInfo.accessor.count; i++) {
					unsigned int curr;
					memcpy(&curr, dataPtr, sizeof(uint16_t));
					data.indices[i] = curr;
					dataPtr += indicesStride;
				}
			}
			else if (indicesAccessorInfo.accessor.component_type == cgltf_component_type_r_32u) {
				uint32_t indicesStride = indicesAccessorInfo.accessor.buffer_view->stride == 0 ?
					indicesAccessorInfo.accessor.stride : indicesAccessorInfo.accessor.buffer_view->stride;

				unsigned char* dataPtr = (
					indicesAccessorInfo.data + indicesAccessorInfo.bufferView.offset + indicesAccessorInfo.accessor.offset
					);

				data.indices = std::vector<unsigned int>(indicesAccessorInfo.accessor.count);
				for (unsigned int i = 0; i < indicesAccessorInfo.accessor.count; i++) {
					unsigned int curr;
					memcpy(&curr, dataPtr, sizeof(uint32_t));
					data.indices[i] = curr;
					dataPtr += indicesStride;
				}
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

inline std::string texturePathHelper(std::string& gltfFilePath, std::string texPath) {
	return (
			std::filesystem::path(gltfFilePath).parent_path() /
			std::filesystem::path(texPath)
		).generic_string();
}

inline bool hasTexture(const cgltf_texture_view& view) {
	return view.texture != NULL;
}

PointLightInfo getLightInfo(glm::vec3 position, cgltf_light& light) {
	PointLightInfo info{};
	info.color = glm::make_vec3(light.color);
	info.intensity = light.intensity;
	info.position = position;
	info.radius = light.range;
	return info;
}

std::vector<MaterialPBR> loadMaterials(std::string mPath, const ModelInterface& model) {
	std::vector<MaterialPBR> materials;
	materials.reserve(model.materials.size());
	for (auto& material : model.materials) {
		MaterialPBR pbrMaterial;
		
		if (hasTexture(material.normal_texture)) {
			pbrMaterial.normalScale = material.normal_texture.scale;
			pbrMaterial.normalMap = texturePathHelper(mPath, material.normal_texture.texture->image->uri);
		}
		
		if (material.has_pbr_metallic_roughness) {
			cgltf_pbr_metallic_roughness pbr = material.pbr_metallic_roughness;
			MetallicRoughnessMat mat;
			if(hasTexture(pbr.metallic_roughness_texture))
			{
				mat.metallicRoughnessTex = texturePathHelper(mPath, pbr.metallic_roughness_texture.texture->image->uri);
			}
			if (hasTexture(pbr.base_color_texture))
				mat.baseColorTex = texturePathHelper(mPath, pbr.base_color_texture.texture->image->uri);
			mat.metallic_factor = pbr.metallic_factor;
			mat.roughness_factor = pbr.roughness_factor;
			mat.baseColorFactor = glm::make_vec4(pbr.base_color_factor);

			pbrMaterial.isMetallicRoughness = true;
			pbrMaterial.metallicRoughness = mat;
		}
		else if (material.has_pbr_specular_glossiness) {
			cgltf_pbr_specular_glossiness pbr = material.pbr_specular_glossiness;
			DiffuseSpecularMat mat;
			if(hasTexture(pbr.diffuse_texture))
				mat.diffuseTex = texturePathHelper(mPath, pbr.diffuse_texture.texture->image->uri);
			if (hasTexture(pbr.specular_glossiness_texture))
				mat.specularGlossTex = texturePathHelper(mPath, pbr.specular_glossiness_texture.texture->image->uri);
			mat.diffuseFactor = glm::make_vec4(pbr.diffuse_factor);
			mat.specularFactor = glm::make_vec3(pbr.specular_factor);
			mat.glossinessFactor = pbr.glossiness_factor;
			pbrMaterial.isMetallicRoughness = false;
			pbrMaterial.diffuseSpecular = mat;
		}
		else {
			std::cerr << "Could not load material: " << material.name << std::endl;
		}
		materials.push_back(pbrMaterial);
	}
	return materials;
}


inline glm::vec3 getNodeTranslation(cgltf_node* node) {
	//returns local translation of a node
	// vec3(0) if not available
	return node->has_translation ? coordinateSystemCorrection(glm::make_vec3(node->translation)) : glm::vec3(0);
}

inline glm::vec3 getNodeScale(cgltf_node* node) {
	//returns local scale of a node
	// vec3(1) if not available
	return node->has_scale ? glm::make_vec3(node->scale) : glm::vec3(1);
}

inline glm::quat getNodeRotation(cgltf_node* node) {
	//returns local rotation of a node

	return node->has_rotation ?
		glm::quat(
			-node->rotation[3],
			node->rotation[0], -node->rotation[1], node->rotation[2]
		) :
		glm::quat();
}

glm::mat4 getNodeLocalTransform(cgltf_node* node) {
	//returns local transformation of a node
	glm::mat4 localTransform;
	if (node->has_matrix) {
		//todo still havent figured out how to convert matrix
		throw std::runtime_error("it had matrix your honor");
		localTransform = glm::make_mat4x4(node->matrix);
		//glm::vec3 translation, scale, skew;
		//glm::vec4 perspective;
		//glm::quat rotation;
		//glm::decompose(localTransform, scale, rotation, translation, skew, perspective);
		//
		//translation = coordinateSystemCorrection(translation);
		//rotation.w -= rotation.w;
		//rotation.y -= rotation.y;

		//localTransform = (
		//	glm::translate(
		//		glm::mat4(1.f),
		//		translation
		//	)
		//	* glm::mat4(rotation)
		//	* glm::scale(
		//		glm::mat4(1.f),
		//		scale
		//	)
		//	);
	}
	else {
		localTransform = glm::mat4(1.0f);
		if (node->has_translation) {
			glm::vec3 translation = getNodeTranslation(node);
			localTransform = glm::translate(localTransform, translation);
		}
		if (node->has_rotation) {
			glm::quat q = getNodeRotation(node);
			localTransform *= glm::mat4(q);
		}
		if (node->has_scale) {
			glm::vec3 scale = getNodeScale(node);
			localTransform *= glm::scale(glm::mat4(1.f), scale);
		}
		//TRS order
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
	return globalTransform;
}

glm::vec3 getNodeGlobalTranslation(cgltf_node* node) {
	//returns global translation of a node
	glm::vec3 globalTranslation = getNodeTranslation(node);
	cgltf_node* curr = node->parent;
	while (curr != NULL) {
		globalTranslation += getNodeTranslation(curr);
		curr = curr->parent;
	}
	return globalTranslation;
}

glm::vec3 getNodeGlobalScale(cgltf_node* node) {
	//returns global scale of a node
	glm::vec3 globalScale = getNodeScale(node);
	cgltf_node* curr = node->parent;
	while (curr != NULL) {
		globalScale *= getNodeScale(curr);
		curr = curr->parent;
	}
	return globalScale;
}

glm::quat getNodeGlobalRotation(cgltf_node* node) {
	glm::quat globalRot = getNodeRotation(node);
	cgltf_node* curr = node->parent;
	while (curr != NULL) {
		globalRot = getNodeRotation(curr) * globalRot;
		curr = curr->parent;
	}
	return globalRot;
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

	//for (auto& matID : modelData.meshData.matIndex) {
	//	std::cout << "confirm " << (model.materials.end() - 1 - matID)->name << std::endl;
	//}
	modelData.materials = loadMaterials(filepath, model);

	modelData.meshes.reserve(loadedMeshes.size());
	modelData.drawables.reserve(loadedMeshes.size());
	modelData.pointLights.reserve(model.lights.size());

	std::vector<cgltf_node*> nodesQueue;
	nodesQueue.reserve(model.nodes.size());
	for (int i = 0; i < data->scene->nodes_count; i++) {
		nodesQueue.push_back(data->scene->nodes[i]);
	}

	while(!nodesQueue.empty()){
		cgltf_node& node = *nodesQueue.back();
		if (node.light != nullptr) {
			if (node.light->type == cgltf_light_type_point) {
				modelData.pointLights.push_back(
					getLightInfo(
						getNodeGlobalTranslation(&node),
						*node.light
					)
				);
			}
			else if (node.light->type == cgltf_light_type_directional) {
				modelData.directionalLight.color = glm::make_vec3(node.light->color);
				modelData.directionalLight.intensity = node.light->intensity;
				modelData.directionalLight.direction = (
						getNodeGlobalRotation(&node) * glm::vec3(0, 0, -1)
					);
				modelData.directionalLight.waste = -1.9872612;
			}
		}
		if (node.mesh != nullptr) {
			glm::vec3 pos = getNodeGlobalTranslation(&node);
			glm::quat rot = getNodeGlobalRotation(&node);
			glm::vec3 scale = getNodeGlobalScale(&node);
			//assert(scale.x == scale.y && scale.x == scale.z);
			glm::mat4 transform = getNodeGlobalTransform(&node);

			int mIndex = node.mesh - model.meshes.begin();
			addedMeshes.insert(node.mesh);
			for (auto& primitive : loadedMeshes[mIndex]) {
				GLTFDrawable drawable{};
				drawable.transform = transform;
				drawable.position = pos;
				drawable.rotation = rot;
				drawable.scale = scale;
				drawable.material = primitive.second;
				drawable.mesh = modelData.meshes.size();
				modelData.meshes.push_back(primitive.first);

				modelData.drawables.push_back(drawable);
			}
		}
		
		nodesQueue.pop_back();
		for (int i = 0; i < node.children_count; i++)
		{
			nodesQueue.push_back(node.children[i]);
		}
	}

	cgltf_free(data);

	int tricount = 0, vertexcount = 0;
	for (auto& mesh : modelData.meshes) {
		vertexcount += mesh.vertices.size();
		tricount += mesh.indices.size() / 3;
	}

	return modelData;
}