#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out flat uint materialID;

layout(set = 0, binding = 0) uniform  Matrices{
    mat4 proj;
    mat4 view;
    mat4 projView;
    vec4 cameraPos_time;
	vec4 fovY_aspectRatio_zNear_zFar;
} matrices;

layout(std140, set = 3, binding = 1) readonly buffer Voxel{
    uvec4 array[];
} position_mats;


/*
    imp
    instead of drawing cubes
    draw 6 * numCubes squares
    and get face index by doing gl_InstanceIndex % 6
    and get instance index by doing gl_InstanceIndex / 6
    then store a 6 bit mask in the MSB of materialID (materials will be 32 - 6 = 24 bits now)
    and finally discard face by setting vertex to N.A.N if masked

    draw numCubes only
    get face index by doing gl_VertexIndex % 6
*/

void main(){
    fragNorm = inNormal;
    uvec4 position_mat = position_mats.array[gl_InstanceIndex];
    
    fragPos = (inPosition + vec3(position_mat.xyz) + 0.5).xyz;
    
    // encodedMaterialID (32 bits) is in a format where:
    // the faceMask is the Most Significant 6 bits and
    // actual materialID is the remaining 26 bits
    uint encodedMaterialID = position_mat.w;
    materialID = bitfieldExtract(encodedMaterialID, 0, (32 - 6));
    uint faceMask =  bitfieldExtract(encodedMaterialID, (32 - 6), 6);

    uint faceIndex = gl_VertexIndex / 6;
    bool isFaceMasked = (bool(faceMask & (1 << faceIndex)));
    // isFaceMasked = 0.0;
    // materialID = uint(faceIndex);
    
    gl_Position = matrices.projView * vec4(fragPos, 1.0);
    //if the face mask is set to 1, the position is set to 1,1,1,0 which will be discarded during clipping
    if(isFaceMasked){
        gl_Position = vec4(0);
    }
}