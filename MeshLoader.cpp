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
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "json.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"
using namespace tinygltf;
using json = nlohmann::json;

#include "MeshLoader.h"

void loadOBJ(const char* filepath);

struct AccessorInfo {
    Accessor accessor;
    BufferView bufferView;
    tinygltf::Buffer buffer;
};

AccessorInfo loadAccessor(Model& model, int accessor_index) {
    AccessorInfo info;
    info.accessor = model.accessors[accessor_index];
    info.bufferView = model.bufferViews[info.accessor.bufferView];
    info.buffer = model.buffers[info.bufferView.buffer];
    return info;
}

std::vector<std::pair<MeshData<Vertex3>, int>> loadMeshes(Model& model) {
    std::vector<std::pair<MeshData<Vertex3>, int>> meshes;
    size_t num_total_primitives = 0;
    for (const auto& mesh : model.meshes)
        num_total_primitives += mesh.primitives.size();
    meshes.reserve(num_total_primitives);
    std::array<std::string, 3> required = { "POSITION", "NORMAL", "TEXCOORD_0" };
    std::array<AccessorInfo, 3> accessorsInfo;
    for (auto& mesh : model.meshes) {
        for (auto& primitive : mesh.primitives) {
            MeshData<Vertex3> data;
            // todo make sure primitive mode = 4 (TRIANGLES)
            if (primitive.mode != 4) {
                //! panic
                continue;
            }
            for (int i = 0; i < 3; ++i) {
                if (primitive.attributes.find(required[i]) ==
                    primitive.attributes.end()) {
                    throw std::runtime_error("Could not find required attribute in primitive");
                    //! panic
                }
                accessorsInfo[i] =
                    loadAccessor(model, primitive.attributes[required[i]]);
                if (accessorsInfo[i].accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    throw std::runtime_error("Primitive accessor component type not supported");
                    //! panic
                }
            }
            unsigned char* position = accessorsInfo[0].buffer.data.data() +
                accessorsInfo[0].bufferView.byteOffset +
                accessorsInfo[0].accessor.byteOffset;
            int position_stride = accessorsInfo[0].bufferView.byteStride == 0
                ? sizeof(glm::vec3)
                : accessorsInfo[0].bufferView.byteStride;

            unsigned char* normal = accessorsInfo[1].buffer.data.data() +
                accessorsInfo[1].bufferView.byteOffset +
                accessorsInfo[1].accessor.byteOffset;
            int normal_stride = accessorsInfo[1].bufferView.byteStride == 0
                ? sizeof(glm::vec3)
                : accessorsInfo[1].bufferView.byteStride;

            unsigned char* uv = accessorsInfo[2].buffer.data.data() +
                accessorsInfo[2].bufferView.byteOffset +
                accessorsInfo[2].accessor.byteOffset;
            int uv_stride = accessorsInfo[2].bufferView.byteStride == 0
                ? sizeof(glm::vec2)
                : accessorsInfo[2].bufferView.byteStride;

            data.vertices.reserve(accessorsInfo.front().accessor.count);
            for (int i = 0; i < accessorsInfo.front().accessor.count; ++i) {
                Vertex3 curr;
                memcpy(&curr.pos, position, sizeof(glm::vec3));
                position += position_stride;
                memcpy(&curr.norm, normal, sizeof(glm::vec3));
                normal += normal_stride;
                memcpy(&curr.uv, uv, sizeof(glm::vec2));
                uv += uv_stride;
                data.vertices.push_back(curr);
            }
            auto indicesAccessorInfo = loadAccessor(model, primitive.indices);
            if (indicesAccessorInfo.accessor.componentType ==
                TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                auto bufferIterator = indicesAccessorInfo.buffer.data.begin() +
                    indicesAccessorInfo.bufferView.byteOffset +
                    indicesAccessorInfo.accessor.byteOffset;
                data.indices = std::vector<unsigned int>(
                    bufferIterator,
                    bufferIterator + indicesAccessorInfo.accessor.count);
            }
            else if (indicesAccessorInfo.accessor.componentType ==
                TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                unsigned short* buffer = reinterpret_cast<unsigned short*>(
                    indicesAccessorInfo.buffer.data.data() +
                    indicesAccessorInfo.accessor.byteOffset +
                    indicesAccessorInfo.bufferView.byteOffset);
                data.indices = std::vector<unsigned int>(
                    buffer, buffer + indicesAccessorInfo.accessor.count);
            }
            else if (indicesAccessorInfo.accessor.componentType ==
                TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                unsigned int* buffer = reinterpret_cast<unsigned int*>(
                    indicesAccessorInfo.buffer.data.data() +
                    indicesAccessorInfo.accessor.byteOffset +
                    indicesAccessorInfo.bufferView.byteOffset);
                data.indices = std::vector<unsigned int>(
                    buffer, buffer + indicesAccessorInfo.accessor.count);
            }
            else {
                // invalid component type for indices !panic
            }
            meshes.push_back(std::make_pair(std::move(data), primitive.material));
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
        else if(mat->contains("extensions") && (*mat)["extensions"].contains("KHR_materials_pbrSpecularGlossiness")) {
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

inline void appendMesh(ModelData* modelData, MeshData<Vertex3> mesh, int matIndex, glm::mat4 transform) {
    modelData->meshData.meshes.push_back(mesh);
    modelData->meshData.matIndex.push_back(matIndex);
    modelData->meshData.transforms.push_back(transform);
}

glm::mat4 getNodeTransform(Node& node) {
    glm::mat4 value;
    if (node.matrix.size() == 16)
        value = glm::make_mat4(node.matrix.data());
    else {
        glm::vec3 translation = glm::vec3(0.f);
        glm::quat rotation = glm::quat(glm::vec3(0.f, 0.f , 0.f));
        glm::vec3 scale = glm::vec3(1.f);

        if (node.translation.size() == 3)
            translation = glm::make_vec3(node.translation.data());
        if (node.rotation.size() == 4)
            rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
        if (node.scale.size() == 3)
            scale = glm::make_vec3(node.scale.data());

        //TRS order
        value =
            glm::translate(glm::mat4(1.f), translation)
            * glm::mat4_cast(rotation)
            * glm::scale(glm::mat4(1.f), scale);
    }
    return value;
}

std::optional<ModelData> loadGLTF(const char* filepath) {
    Model model;
    TinyGLTF loader;
    std::string err;
    std::string warn;
    std::filesystem::path path(filepath);
    bool ret;
    if (path.extension() == ".gltf")
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
    else if (path.extension() == ".glb")
        ret = loader.LoadBinaryFromFile(&model, &err, &warn,
            filepath); // for binary glTF(.glb)
    else
        ret = false;
    if (!warn.empty()) {
        std::cerr << "Warn: " << warn.c_str() << "\n";
    }

    if (!err.empty()) {
        std::cerr << "Err: " << err.c_str() << "\n";
        return std::nullopt;
    }

    if (!ret) {
        std::cerr << "glTF: Failed to parse file: " << filepath << " \n";
        return std::nullopt;
    }
    auto loadedMeshes = loadMeshes(model);
    std::set<int> addedMeshes;

    ModelData modelData;
    modelData.materials = loadMaterials(filepath);

    modelData.meshData.meshes.reserve(loadedMeshes.size());
    modelData.meshData.transforms.reserve(loadedMeshes.size());
    modelData.meshData.matIndex.reserve(loadedMeshes.size());
    for (auto& node : model.nodes) {
        if (node.mesh != -1 && addedMeshes.find(node.mesh) == addedMeshes.end()) {
            appendMesh(&modelData, loadedMeshes[node.mesh].first, loadedMeshes[node.mesh].second, getNodeTransform(node));
            addedMeshes.insert(node.mesh);
        }
    }
//    loadOBJ("3DModels/bedroom/iscv2.obj");
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