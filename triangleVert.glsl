#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 vertexColor;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 render_matrix;
} PushConstants;

void main(){
    gl_Position = PushConstants.render_matrix * vec4(inPosition, 1.0);
    vertexColor = vec3(inNormal * 0.5 + 0.5);
}