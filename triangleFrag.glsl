#version 450

layout(location = 0)in vec3 vertexColor;
layout(location = 0)out vec4 fragColor;

layout(set = 1, binding = 0) uniform  Materials{
	vec4 diffuseColor;
	vec4 specularGlossiness;
} materials;

void main(){
	fragColor = vec4(materials.diffuseColor);
}