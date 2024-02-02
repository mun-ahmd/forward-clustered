#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out flat vec3 cameraPos;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 modelMatrix;
} PushConstants;

layout(set = 0, binding = 0) uniform  Matrices{
    mat4 proj;
    mat4 view;
    mat4 projView;
    vec4 cameraPos_time;
	vec4 fovY_aspectRatio_zNear_zFar;
};

void main(){
    fragNorm = transpose(inverse(mat3(PushConstants.modelMatrix))) * inNormal;
    fragPos = (PushConstants.modelMatrix * vec4(inPosition, 1.0)).xyz;
    fragUV = inTexCoord;
    
    cameraPos = cameraPos_time.xyz;
    gl_Position = projView * vec4(fragPos, 1.0);
}