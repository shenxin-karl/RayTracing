#include "CbLighting.hlsli"
#include "CbPrePass.hlsli"
#include "CbPreObject.hlsli"
#include "CookTorrance.hlsli"

struct CbPreMaterial {
    float4      albedo;
    float4      emission;
    float4      tilingAndOffset;
    float       cutoff;
    float       roughness;
    float       metallic;
    float       normalScale;
    int         samplerStateIndex;
    int         albedoTexIndex;
    int         ambientOcclusionTexIndex;
    int         emissionTexIndex;
    int         metalRoughnessTexIndex;
    int         normalTexIndex;
};

struct VertexIn {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 uv0      : TEXCOORD0;
    float4 color    : COLOR;
};

struct VertexOut {
    float4 SVPosition  : SV_POSITION;
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float3 tangent     : TANGENT;
    float4 color       : COLOR;
    float2 uv0         : uv0;
};


ConstantBuffer<CBPrePass>       gCbPrePass              : register(b0);
ConstantBuffer<CBPreObject>     gCbPreObject            : register(b1);
ConstantBuffer<CbLighting>      gCbLighting             : register(b3);
SamplerState                    gStaticSamplerState[]   : register(s0);

ConstantBuffer<CbPreMaterial>   gCbMaterial             : register(b2);
Texture2D<float4>               gTextureList[]          : register(t0);

VertexOut VSMain(VertexIn vin) {
    VertexOut vout = (VertexOut)0;
    float4 worldPosition = mul(gCbPreObject.gMatWorld, float4(vin.position, 1.0));
    vout.SVPosition = mul(gCbPrePass.gMatViewProj, float4(vin.position, 1.0));
    vout.position = worldPosition.xyz;
    vout.normal = mul((float3x3)gCbPreObject.gMatNormal, vin.normal);
    vout.tangent.xyz = mul((float3x3)gCbPreObject.gMatWorld, vin.tangent.xyz);
    vout.tangent *= vin.tangent.w;
    vout.color = vin.color;
    vout.uv0 = vin.uv0 * gCbMaterial.tilingAndOffset.xy + gCbMaterial.tilingAndOffset.zw;
    return vout;
}

/*
 *  ENABLE_ALPHA_TEST
 *  ENABLE_EMISSION
 *  ENABLE_NORMAL_SCALE
 *  ENABLE_ALBEDO_TEXTURE
 *  ENABLE_AMBIENT_OCCLUSION_TEXTURE
 *  ENABLE_EMISSION_TEXTURE
 *  ENABLE_METAL_ROUGHNESS_TEXTURE
 *  ENABLE_NORMAL_TEX
 */

float4 GetAlbedo(VertexOut pin) {
	float4 albedo = gCbMaterial.albedo;
	float4 sampleColor = (float4)1.0;

	#if ENABLE_ALBEDO_TEXTURE
		SamplerState samplerState = gStaticSamplerState[gCbMaterial.samplerStateIndex];
        sampleColor = gTextureList[gCbMaterial.albedoTexIndex].Sample(samplerState, pin.uv0);
		albedo *= sampleColor;
	#endif

	#if ENABLE_ALPHA_TEST
		float alpha = albedo.a * sampleColor.a;
		clip(alpha - gCbMaterial.cutoff);
	#endif
    return albedo;
}

float2 GetMetallicAndRoughness(VertexOut pin) {
    float metallic = gCbMaterial.metallic;
    float roughness = gCbMaterial.roughness;
	#if ENABLE_METAL_ROUGHNESS_TEXTURE
		SamplerState samplerState = gStaticSamplerState[gCbMaterial.samplerStateIndex];
        float2 sampleTexture = gTextureList[gCbMaterial.metalRoughnessTexIndex].Sample(samplerState, pin.uv0).rg;
        metallic *= sampleTexture.r;
		roughness *= sampleTexture.g;
	#endif
    return float2(metallic, roughness);
}

float3 GetNormal(VertexOut pin) {
    float3 N = normalize(pin.normal);
	#if ENABLE_ALBEDO_TEXTURE
		SamplerState samplerState = gStaticSamplerState[gCbMaterial.samplerStateIndex];
		float3 sampleNormal = gTextureList[gCbMaterial.normalTexIndex].Sample(samplerState, pin.uv0);
		sampleNormal = sampleNormal * 2.f - 1.f;
        float3 T = normalize(pin.tangent);
        float3 B = cross(N, T);
		N = T * sampleNormal.x + B * sampleNormal.y + N * sampleNormal.z;
	#endif
    return N;
}

float GetAmbientOcclusion(VertexOut pin) {
    float ao = 1.0;
    #if ENABLE_AMBIENT_OCCLUSION_TEXTURE
		SamplerState samplerState = gStaticSamplerState[gCbMaterial.samplerStateIndex];
        ao *= gTextureList[gCbMaterial.ambientOcclusionTexIndex].Sample(samplerState, pin.uv0).r;
    #endif
    return ao;
}

float3 GetEmission(VertexOut pin) {
	float3 emission = 0.f;
	#if ENABLE_EMISSION_TEXTURE
		SamplerState samplerState = gStaticSamplerState[gCbMaterial.samplerStateIndex];
		emission = gTextureList[gCbMaterial.emissionTexIndex].Sample(samplerState, pin.uv0).rgb;
	#endif
    return emission;
}

float4 PSMain(VertexOut pin) : SV_TARGET {
    float2 metallicAndRoughness = GetMetallicAndRoughness(pin);
    float metallic = metallicAndRoughness.r;
    float roughness = metallicAndRoughness.y;
    float4 albedo = GetAlbedo(pin);
    float ao = GetAmbientOcclusion(pin);
    float3 N = GetNormal(pin);
    float3 V = normalize(gCbPrePass.gCameraPos - pin.position);
    float3 L = gCbLighting.directionalLight.direction;
    float3 H = normalize(V + L);

    MaterialData materialData;
    materialData.diffuseAlbedo = albedo.rgb;
    materialData.roughness = roughness;
    materialData.fresnelFactor = ComputeFresnelFactor(N, H, albedo, metallic);
    materialData.metallic = metallic;

    float3 finalColor = ComputeDirectionLight(gCbLighting.directionalLight, materialData, N, V);
    finalColor += ComputeAmbientLight(gCbLighting.directionalLight, materialData, ao);
    finalColor += GetEmission(pin);
    return float4(finalColor, albedo.a);
}