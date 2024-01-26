#version 450 core
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;

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

layout(set = 2, binding = 0) uniform  Material{
	vec4 baseColorFactor;
	vec4 metallicRoughness_waste2;
	uvec4 baseTexId_metallicRoughessTexId_waste2;
};

layout(set=2, binding=1) uniform sampler2D MaterialTextures[1024];
// layout(set = 2, binding = 1) uniform sampler2D tex;

void main(){
	vec3 normal = normalize(fragNorm);
	fragColor = vec4(0.0);
	vec4 baseColor = texture(MaterialTextures[baseTexId_metallicRoughessTexId_waste2.x], fragUV);
	for(int i = 0; i < lightsCount.val; i++){
		PL cLight = lights.arr[i];
		fragColor += baseColor * baseColorFactor * (
			0.1 + cLight.color * max(0.0, dot(normal, cLight.position_radius.xyz - fragPos))
		) * 0.1;
	}
	//need to use nonuniform extension when the index is not dynamically uniform
}