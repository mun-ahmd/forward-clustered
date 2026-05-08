#pragma once
#include <stdint.h>
#include <string>
#include <array>
#include <sol/sol.hpp>
#include "scripting/RenderingCreateInfo.hpp"

#define EXPORTPROP(name)
#define EXPORTCLASS()

using SceneObjectID = uint32_t;

namespace SceneObj {
	struct Mesh {
		Rendering::ResourceID vertexBuffer = 0;
		Rendering::ResourceID indexBuffer  = 0;
		uint32_t    indexCount  = 0;
		std::string indexType;   // "uint16" | "uint32"
	};

	struct Light {
		std::string type = "point";   // "point" | "directional"
		float posX = 0, posY = 0, posZ = 0;
		float colR = 1, colG = 1, colB = 1;
		float intensity = 1.0f;
	};

	struct Material {
		Rendering::ResourceID descriptorSet  = 0;
		uint32_t              dynamicOffset  = 0;
	};

	struct Instance {
		SceneObjectID mesh     = 0;
		SceneObjectID material = 0;
		std::array<float, 16> transform = {
			1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
		};
	};
}

struct EXPORTCLASS() SceneMeshCreateInfo {
	Rendering::ResourceID EXPORTPROP("vertexBuffer") vertexBuffer = 0;
	Rendering::ResourceID EXPORTPROP("indexBuffer")  indexBuffer  = 0;
	uint32_t    EXPORTPROP("indexCount")  indexCount = 0;
	std::string EXPORTPROP("indexType")   indexType;
};

struct EXPORTCLASS() SceneLightCreateInfo {
	std::string EXPORTPROP("type")      type      = "point";
	float       EXPORTPROP("posX")      posX      = 0;
	float       EXPORTPROP("posY")      posY      = 0;
	float       EXPORTPROP("posZ")      posZ      = 0;
	float       EXPORTPROP("colR")      colR      = 1;
	float       EXPORTPROP("colG")      colG      = 1;
	float       EXPORTPROP("colB")      colB      = 1;
	float       EXPORTPROP("intensity") intensity = 1.0f;
};

struct EXPORTCLASS() SceneLightProperties {
	std::string EXPORTPROP("type")      type      = "point";
	float       EXPORTPROP("posX")      posX      = 0;
	float       EXPORTPROP("posY")      posY      = 0;
	float       EXPORTPROP("posZ")      posZ      = 0;
	float       EXPORTPROP("colR")      colR      = 1;
	float       EXPORTPROP("colG")      colG      = 1;
	float       EXPORTPROP("colB")      colB      = 1;
	float       EXPORTPROP("intensity") intensity = 1.0f;
};

struct EXPORTCLASS() SceneMaterialCreateInfo {
	Rendering::ResourceID EXPORTPROP("descriptorSet")  descriptorSet  = 0;
	uint32_t              EXPORTPROP("dynamicOffset")  dynamicOffset  = 0;
};

struct EXPORTCLASS() SceneInstanceCreateInfo {
	SceneObjectID EXPORTPROP("mesh")      mesh      = 0;
	SceneObjectID EXPORTPROP("material")  material  = 0;
	sol::table    EXPORTPROP("transform") transform;   // 16-element column-major table
};

struct EXPORTCLASS() SceneInstanceProperties {
	sol::table EXPORTPROP("transform") transform;    // 16-element column-major table
};
