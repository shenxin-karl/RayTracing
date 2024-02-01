#include "CbLighting.hlsli"
#include "CbPrePass.hlsli"
#include "CbPreObject.hlsli"
#include "CookTorrance.hlsli"
#include "DepthUtil.hlsli"

#include "NRDEncoding.hlsli"
#include "NRD.hlsli"

struct CbMaterial {
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


/*
 *  ENABLE_ALPHA_TEST
 *  ENABLE_ALBEDO_TEXTURE
 *  ENABLE_AMBIENT_OCCLUSION_TEXTURE
 *  ENABLE_EMISSION_TEXTURE
 *  ENABLE_METAL_ROUGHNESS_TEXTURE
 *  ENABLE_NORMAL_TEX
 *  ENABLE_VERTEX_COLOR
 */

#define ENABLE_VERTEX_UV (ENABLE_ALBEDO_TEXTURE            ||           \
						  ENABLE_AMBIENT_OCCLUSION_TEXTURE ||           \
						  ENABLE_EMISSION_TEXTURE          ||           \
						  ENABLE_METAL_ROUGHNESS_TEXTURE   ||           \
                          ENABLE_NORMAL_TEX)

struct VertexIn {
    float3 position         : POSITION;
    float3 normal           : NORMAL;
    #if ENABLE_NORMAL_TEX
        float4 tangent      : TANGENT;
    #endif
    #if ENABLE_VERTEX_UV
        float2 uv0          : TEXCOORD0;
    #endif
    #if ENABLE_VERTEX_COLOR
        float4 color        : COLOR;
    #endif
};

struct VertexOut {
    float4 SVPosition       : SV_POSITION;
    float3 position         : POSITION;
    float3 normal           : NORMAL;
    #if ENABLE_NORMAL_TEX
        float3 tangent      : TANGENT;
    #endif 
    #if ENABLE_VERTEX_UV
        float2 uv0          : TEXCOORD0;
    #endif
    #if ENABLE_VERTEX_COLOR
        float4 color        : COLOR;
    #endif
	#if GENERATE_MOTION_VECTOR
		float4 currentClipPos   : MOTION_VECTOR0;
		float4 previousClipPos  : MOTION_VECTOR1;
	#endif
};

ConstantBuffer<CBPrePass>       gCbPrePass              : register(b0);
ConstantBuffer<CBPreObject>     gCbPreObject            : register(b1);
ConstantBuffer<CbLighting>      gCbLighting             : register(b3);
SamplerState                    gStaticSamplerState[]   : register(s0);

ConstantBuffer<CbMaterial>      gCbMaterial             : register(b2);
Texture2D<float4>               gTextureList[]          : register(t0);

VertexOut VSMain(VertexIn vin) {
    VertexOut vout = (VertexOut)0;
    float4 localPosition = float4(vin.position, 1.0);
    float4 worldPosition = mul(gCbPreObject.matWorld, localPosition);
    vout.SVPosition = mul(gCbPrePass.matJitterViewProj, worldPosition);
    vout.position = worldPosition.xyz;
    vout.normal = mul((float3x3)gCbPreObject.matNormal, vin.normal);
    #if ENABLE_NORMAL_TEX
        vout.tangent.xyz = mul((float3x3)gCbPreObject.matWorld, vin.tangent.xyz);
        vout.tangent *= vin.tangent.w;
    #endif
    #if ENABLE_VERTEX_COLOR
        vout.color = vin.color;
    #endif
    #if ENABLE_VERTEX_UV
        vout.uv0 = vin.uv0 * gCbMaterial.tilingAndOffset.xy + gCbMaterial.tilingAndOffset.zw;
    #endif
    #if GENERATE_MOTION_VECTOR
		vout.currentClipPos = vout.SVPosition;
		vout.previousClipPos = mul(gCbPrePass.matViewProjPrev, mul(gCbPreObject.matWorldPrev, localPosition));
	#endif
    return vout;
}

float4 GetAlbedo(VertexOut pin) {
	float4 albedo = gCbMaterial.albedo;

	#if ENABLE_VERTEX_COLOR
		albedo *= pin.color;
	#endif

	float4 sampleColor = (float4)1.0;
	#if ENABLE_ALBEDO_TEXTURE
		SamplerState samplerState = gStaticSamplerState[NonUniformResourceIndex(gCbMaterial.samplerStateIndex)];
        uint textureIndex = NonUniformResourceIndex(gCbMaterial.albedoTexIndex);
        sampleColor = gTextureList[textureIndex].SampleBias(samplerState, pin.uv0, gCbPrePass.mipBias);
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
		SamplerState samplerState = gStaticSamplerState[NonUniformResourceIndex(gCbMaterial.samplerStateIndex)];
        uint textureIndex = NonUniformResourceIndex(gCbMaterial.metalRoughnessTexIndex);
        float2 sampleTexture = gTextureList[textureIndex].SampleBias(samplerState, pin.uv0, gCbPrePass.mipBias).rg;
        metallic *= sampleTexture.r;
		roughness *= sampleTexture.g;
	#endif
    return float2(metallic, roughness);
}

float3 GetNormal(VertexOut pin) {
    float3 N = normalize(float3(pin.normal.xy * gCbMaterial.normalScale, pin.normal.z));
	#if ENABLE_NORMAL_TEX
		SamplerState samplerState = gStaticSamplerState[NonUniformResourceIndex(gCbMaterial.samplerStateIndex)];
        uint textureIndex = NonUniformResourceIndex(gCbMaterial.normalTexIndex);
		float3 sampleNormal = gTextureList[textureIndex].SampleBias(samplerState, pin.uv0, gCbPrePass.mipBias);
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
		SamplerState samplerState = gStaticSamplerState[NonUniformResourceIndex(gCbMaterial.samplerStateIndex)];
        uint textureIndex = NonUniformResourceIndex(gCbMaterial.ambientOcclusionTexIndex);
        ao *= gTextureList[textureIndex].SampleBias(samplerState, pin.uv0, gCbPrePass.mipBias).r;
    #endif
    return ao;
}

float3 GetEmission(VertexOut pin) {
	float3 emission = gCbMaterial.emission.rgb;
	#if ENABLE_EMISSION_TEXTURE
		SamplerState samplerState = gStaticSamplerState[NonUniformResourceIndex(gCbMaterial.samplerStateIndex)];
        uint textureIndex = NonUniformResourceIndex(gCbMaterial.emissionTexIndex);
		emission += gTextureList[textureIndex].SampleBias(samplerState, pin.uv0, gCbPrePass.mipBias).rgb;
	#endif
    return emission;
}

float2 CalcMotionVector(float4 currentClipPos, float4 previousClipPos) {
    // current clip pos is jittered
    float2 currNDC = (currentClipPos.xy / currentClipPos.w) - gCbPrePass.currViewportJitter;
    float2 prevNDC = (previousClipPos.xy / previousClipPos.w);
	float2 motionVector = prevNDC - currNDC;
    motionVector *= float2(+0.5f, -0.5f);
    return motionVector;
}

float4 ForwardPSMain(VertexOut pin) : SV_TARGET {
    float2 metallicAndRoughness = GetMetallicAndRoughness(pin);
    float metallic = metallicAndRoughness.r;
    float roughness = metallicAndRoughness.g;
    float4 albedo = GetAlbedo(pin);
    float ao = GetAmbientOcclusion(pin);
    float3 N = GetNormal(pin);
    float3 V = normalize(gCbPrePass.cameraPos - pin.position);

    MaterialData materialData = CalcMaterialData(albedo.rgb, roughness, metallic);
    float3 finalColor = ComputeDirectionLight(gCbLighting.directionalLight, materialData, N, V);
    finalColor += ComputeAmbientLight(gCbLighting.ambientLight, materialData, ao);
    finalColor += GetEmission(pin);
    return float4(finalColor, albedo.a);
}

struct GBufferOut {
	float4 gBuffer0 : SV_TARGET0;           // .rgb: albedo .a: metallic
    float4 gBuffer1 : SV_TARGET1;           // NRD_FrontEnd_PackNormalAndRoughness result     
    float3 gBuffer2 : SV_TARGET2;           // emission
    float2 gBuffer3 : SV_TARGET3;           // motion vector (rg16f+)
    float  gBuffer4 : SV_TARGET4;           // view-depth
};
GBufferOut GBufferPSMain(VertexOut pin) {
	GBufferOut pout = (GBufferOut)0;
    float2 metallicAndRoughness = GetMetallicAndRoughness(pin);
    float4 albedo = GetAlbedo(pin);
    float3 N = GetNormal(pin);
    float3 emission = GetEmission(pin);

    pout.gBuffer0.xyz = albedo.rgb;
    pout.gBuffer0.w = metallicAndRoughness.x;
	pout.gBuffer1 = NRD_FrontEnd_PackNormalAndRoughness(N, metallicAndRoughness.y, 0);
    pout.gBuffer2.rgb = emission;
    #if GENERATE_MOTION_VECTOR
		pout.gBuffer3 = CalcMotionVector(pin.currentClipPos, pin.previousClipPos);
	#endif
    pout.gBuffer4 = ViewSpaceDepth(pin.SVPosition.z, gCbPrePass.zBufferParams);
    return pout;
}