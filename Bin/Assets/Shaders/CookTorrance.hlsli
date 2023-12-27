#ifndef __COOK_TORRANCE_HLSLI__
#define __COOK_TORRANCE_HLSLI__
#include "CbLighting.hlsli"

struct MaterialData {
    float3 diffuseAlbedo;   
    float  roughness;       
    float3 fresnelFactor;   
    float  metallic;
};

float3 ComputeDirectionLight(DirectionalLight light, MaterialData mat, float3 N, float3 V);
float3 ComputeAmbientLight(AmbientLight light, MaterialData mat, float ao);
float3 ComputePointLight(PointLight pointLight, MaterialData mat, float3 N, float3 V, float3 worldPosition);
float3 ComputeSpotLight(SpotLight light, MaterialData mat, float3 N, float3 V, float3 worldPosition);

// implement

#ifndef USE_CARTOON_SHADING
    #define DIFF_SHADING_FACTOR(NdotL) (NdotL)
    #define SPEC_SHADING_FACTOR(NdotH) (NdotH)
#else
    float CarToonDiffShadingFactor(float NdotL) {
        return NdotL <= 0.f ? 0.4 : (NdotL <= 0.5 ? 0.6 : 1.0);
    }

    float CarToonSpecShadingFactor(float NdotH) {
        return NdotH <= 0.1 ? 0.0 : (NdotH <= 0.8 ? 0.5 : 0.8);
    }
    #define DIFF_SHADING_FACTOR(NdotL) CarToonDiffShadingFactor(NdotL)
    #define SPEC_SHADING_FACTOR(NdotH) CarToonSpecShadingFactor(NdotH)
#endif

MaterialData CalcMaterialData(float3 diffuseAlbedo, float roughness, float metallic) {
    MaterialData mat;
    mat.diffuseAlbedo = diffuseAlbedo;
    mat.roughness = max(roughness, 0.00001);
    mat.fresnelFactor = lerp(float3(0.04, 0.04, 0.04), diffuseAlbedo, metallic);
    mat.metallic = metallic;
    return mat;
}


static const float PI     = 3.14159265359;
static const float INV_PI = 1.0 / 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = SPEC_SHADING_FACTOR(max(dot(N, H), 0.0));
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float k) {
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySmith(float NdotL, float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) * (1.0 / 8.0);
    float ggx2 = GeometrySchlickGGX(NdotV, k);
    float ggx1 = GeometrySchlickGGX(NdotL, k);
    return ggx1 * ggx2;
}

// ----------------------------------------------------------------------------
float3 FresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 LambertDiffuse(float3 diffuse) {
    return diffuse * INV_PI;
}

// ----------------------------------------------------------------------------
float3 CookTorrance(float3 radiance, float3 L, float3 N, float3 V, MaterialData mat, float NdotL) {
    // Cook-Torrance BRDF
    float NdotV = max(dot(N, V), 0.0);

    float3 H = normalize(V + L);
    float NDF = DistributionGGX(N, H, mat.roughness);
    float G = GeometrySmith(NdotL, NdotV, mat.roughness);
    float3 F = FresnelSchlick(dot(H, V), mat.fresnelFactor);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001; // + 0.0001 to prevent divide by zero
    float3 specular = numerator / denominator;

    // kS is equal to Fresnel
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - mat.metallic;
    float3 diffuse = LambertDiffuse(kD * mat.diffuseAlbedo);
    return (diffuse + specular) * radiance;
}

// ----------------------------------------------------------------------------
float CalcAttenuation(float d, float falloffStart, float falloffEnd) {
    return 1.0 - saturate((d - falloffStart) / (falloffEnd - falloffStart));
}

// ----------------------------------------------------------------------------
float3 ComputeDirectionLight(DirectionalLight light, MaterialData mat, float3 N, float3 V) {
    float3 L = light.direction;
    float NdotL = DIFF_SHADING_FACTOR(max(dot(N, L), 0.0));
    float3 lightStrength = light.color * NdotL * light.intensity;
    return CookTorrance(lightStrength, L, N, V, mat, NdotL);
}

float3 ComputeAmbientLight(AmbientLight ambientLight, MaterialData mat, float ao) {
	float3 ambient = ambientLight.intensity * ao * ambientLight.color * mat.diffuseAlbedo;
	return ambient;
}

// ----------------------------------------------------------------------------
float3 ComputePointLight(PointLight pointLight, MaterialData mat, float3 N, float3 V, float3 worldPosition) {
    float3 lightVec = pointLight.position - worldPosition;
    float dis = length(lightVec);
    if (dis > pointLight.range)
        return 0.f;

    float3 L = lightVec / dis;
    float NdotL = DIFF_SHADING_FACTOR(max(dot(N, L), 0.0));
    float attenuation = rcp(dis * dis);
    float3 lightStrength = pointLight.color * NdotL * attenuation * pointLight.intensity;
    return CookTorrance(lightStrength, L, N, V, mat, NdotL);
}

// ----------------------------------------------------------------------------
float3 ComputeSpotLight(SpotLight light, MaterialData mat, float3 N, float3 V, float3 worldPosition) {
    float3 lightVec = light.position - worldPosition;
    float dis = length(lightVec);
    if (dis > light.falloffEnd)
        return 0.f;

    float3 L = lightVec / dis;
    float NdotL = DIFF_SHADING_FACTOR(max(dot(N, L), 0.0));
    float3 lightStrength = light.strength * NdotL;
    float attenuation = CalcAttenuation(dis, light.falloffStart, light.falloffEnd);
    lightStrength *= attenuation;
    float spotFactor = pow(saturate(dot(L, light.direction)), light.spotPower);
    lightStrength *= spotFactor;
    return CookTorrance(lightStrength, L, N, V, mat, NdotL);
}

#endif