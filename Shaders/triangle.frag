#version 450 core
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in flat vec3 cameraPos;


layout(location = 0) out vec4 fragColor;

struct PL{
	vec4 color;
	vec4 position_radius;
};

struct DirectionalL{
    vec4 color_intensity;
    vec4 direction_waste;
};

layout(std430, set = 1, binding = 0) readonly buffer LightsCount{
	int pointLightCount;
};

layout(std430, set = 1, binding = 1) readonly buffer LightsData{
	PL arr[];
} lights;

layout(std430, set = 1, binding = 1) readonly buffer SunLight{
	vec4 color_intensity;
    vec4 direction;
} sun;

layout(set = 2, binding = 0) uniform  Material{
	vec4 baseColorFactor;
	vec4 metallicRoughness_waste2;
	uvec4 baseTexId_metallicRoughessTexId_waste2;
};

layout(set=2, binding=1) uniform sampler2D MaterialTextures[1024];
// layout(set = 2, binding = 1) uniform sampler2D tex;

#define PI 3.14159265359

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main(){
    vec3 albedo     = texture(MaterialTextures[baseTexId_metallicRoughessTexId_waste2.x], vec2(fragUV.x, fragUV.y)).rgb * baseColorFactor.rgb;
	vec2 metallicRoughness = texture(MaterialTextures[baseTexId_metallicRoughessTexId_waste2.y], fragUV).rg * metallicRoughness_waste2.rg;
    float metallic  = metallicRoughness.r;
    float roughness = metallicRoughness.g;
    float ao        = 0.3;

    vec3 N = normalize(fragNorm);
    vec3 V = normalize(cameraPos - fragPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

	for(int i = 0; i < pointLightCount; i++){
		PL cLight = lights.arr[i];
        // calculate per-light radiance
        vec3 L = normalize(cLight.position_radius.xyz - fragPos);
        float dist = length(cLight.position_radius.xyz - fragPos);
        float attenuation = 1.0 / (dist * dist);

		
        vec3 H = normalize(V + L);

        vec3 radiance = 0.5 * cLight.color.rgb * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    {
        //apply directional light from sun
        vec3 L = normalize(sun.direction.xyz);		
        vec3 H = normalize(V + L);
        vec3 radiance = sun.color_intensity.rgb * 4.0;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  

        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.02) * ao;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));

    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    fragColor = vec4(color, 1.0);

	// vec3 normal = normalize(fragNorm);
	// fragColor = vec4(0.0);
	// vec4 baseColor = texture(MaterialTextures[baseTexId_metallicRoughessTexId_waste2.x], fragUV);
	// for(int i = 0; i < lightsCount.val; i++){
	// 	PL cLight = lights.arr[i];
	// 	fragColor += baseColor * baseColorFactor * (
	// 		0.1 + cLight.color * max(0.0, dot(normal, cLight.position_radius.xyz - fragPos))
	// 	) * 0.1;
	// }
	//need to use nonuniform extension when the index is not dynamically uniform
}