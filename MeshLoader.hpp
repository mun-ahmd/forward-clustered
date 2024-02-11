#pragma once
#include <any>
#include <string>
#include <vector>
#include <optional>

#include <glm/glm.hpp>

template<typename T>
struct MeshData {
	std::vector<T> vertices;
	std::vector<unsigned int> indices;
	std::optional<std::any> other;
	MeshData() = default;
	MeshData(std::vector<T> vertices, std::vector<unsigned int> indices) : vertices(vertices), indices(indices) {};
};

struct Vertex3
{
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 uv;
};

struct MetallicRoughnessMat {
	std::string metallicRoughnessTex = "";
	float metallic_factor = 1.0;
	float roughness_factor = 1.0;
	std::string baseColorTex = "";
	glm::vec4 baseColorFactor = glm::vec4(1.0);
};

struct DiffuseSpecularMat {
	std::string diffuseTex = "";
	glm::vec4 diffuseFactor = glm::vec4(1.0);

	std::string specularGlossTex = "";
	glm::vec3 specularFactor = glm::vec4(1.0);
	float glossinessFactor;
};

//struct TextureInfo {
//	std::string path = "";
//	int mag_filter;
//	int min_filter;
//	int wrap_s;
//	int wrap_t;
//};

struct MaterialPBR {
	//can use union but don't bother
	MetallicRoughnessMat metallicRoughness;
	DiffuseSpecularMat diffuseSpecular;
	bool isMetallicRoughness = true;
	std::string normalMap = "";
	float normalScale = 1.0;
};

struct PointLightInfo {
	glm::vec3 color;
	float intensity;
	glm::vec3 position;
	float radius;
};

struct DirectionalLightInfo {
	glm::vec3 color;
	float intensity;
	glm::vec3 direction;
	float waste;
};

struct ModelData {
	struct {
		//all of same length
		std::vector<MeshData<Vertex3>> meshes;
		std::vector<glm::mat4> transforms;
		std::vector<int> matIndex;
	} meshData;
	std::vector<MaterialPBR> materials;
	std::vector<PointLightInfo> pointLights;
	DirectionalLightInfo directionalLight;
};

std::optional<ModelData> loadGLTF(const char* filepath);