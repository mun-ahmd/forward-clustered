#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in flat uint materialID;

layout(location = 0) out vec4 fragColor;

struct PL{
	vec4 color;
	vec4 position_radius;
};

layout(set = 1, binding = 0) readonly buffer LightsCount{
	int val;
} lightsCount;

layout(set = 1, binding = 1) readonly buffer LightsData{
	PL arr[];
} lights;

layout(set = 2, binding = 0) uniform  Materials{
	vec4 diffuseColor;
	vec4 specularGlossiness;
} material;

void main(){
	// vec3 normal = normalize(fragNorm);
	// fragColor = vec4(0.0);
	// for(int i = 0; i < lightsCount.val; i++){
	// 	PL cLight = lights.arr[i];
	// 	fragColor += material.diffuseColor * (
	// 		0.1 + cLight.color * max(0.0, dot(normal, cLight.position_radius.xyz - fragPos))
	// 	) * 0.1;
	// }
	float matIDDeg = 3.14 * float(materialID);
	fragColor = vec4(
		// sin(matIDDeg) * 0.5 + 0.5,
		// cos(matIDDeg) * 0.5 + 0.5,
		// float(materialID) / 255.0,
		vec3(float(materialID) / 15)*0.75 + 0.25,
		1.0);
}