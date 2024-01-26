#pragma once
#include <any>
#include <glm/glm.hpp>

typedef std::unique_ptr<unsigned char[], void(*)(void*)> ImagePtr;

ImagePtr loadImageFromFile(
	const char* filename,
	int* width,
	int* height,
	int* actualChannels,
	int desiredChannels
);

ImagePtr resizeImage(
	ImagePtr image,
	int numChannels,
	int oldWidth,
	int oldHeight,
	int newWidth,
	int newHeight,
	bool isSRGB
);

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

struct MaterialPBR {
	//can use union but don't bother
	MetallicRoughnessMat metallicRoughness;
	DiffuseSpecularMat diffuseSpecular;
	bool isMetallicRoughness = true;
	std::string normalMap = "";
	float normalScale = 1.0;
};

struct ModelData {
	struct {
		//all of same length
		std::vector<MeshData<Vertex3>> meshes;
		std::vector<glm::mat4> transforms;
		std::vector<int> matIndex;
	} meshData;
	std::vector<MaterialPBR> materials;
};

std::optional<ModelData> loadGLTF(const char* filepath);