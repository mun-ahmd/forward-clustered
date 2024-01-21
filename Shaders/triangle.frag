#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
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
	vec3 normal = normalize(fragNorm);
	fragColor = vec4(0.0);
	for(int i = 0; i < lightsCount.val; i++){
		PL cLight = lights.arr[i];
		fragColor += material.diffuseColor * (
			0.1 + cLight.color * max(0.0, dot(normal, cLight.position_radius.xyz - fragPos))
		) * 0.1;
	}
}