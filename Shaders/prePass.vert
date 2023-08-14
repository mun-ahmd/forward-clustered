#version 450

layout(location = 0) in vec3 inPosition;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 modelMatrix;
} PushConstants;

layout(set = 0, binding = 0) uniform  Matrices{
    mat4 proj;
    mat4 view;
    mat4 projView;
    mat4 invProj;
    mat4 invView;
} matrices;

void main(){
    gl_Position = matrices.projView * PushConstants.modelMatrix * vec4(inPosition, 1.0);
}