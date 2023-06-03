#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 modelMatrix;
} PushConstants;

layout(set = 0, binding = 0) uniform  Matrices{
    mat4 proj;
    mat4 view;
    mat4 projView;
} matrices;

void main(){
    fragNorm = transpose(inverse(mat3(PushConstants.modelMatrix))) * inNormal;
    fragPos = (PushConstants.modelMatrix * vec4(inPosition, 1.0)).xyz;
    gl_Position = matrices.projView * vec4(fragPos, 1.0);
}