#include "CbLighting.hlsli"
#include "CbPrePass.hlsli"
#include "CookTorrance.hlsli"
#include "NormalUtil.hlsli"
#include "DepthUtil.hlsli"
#include "NRDEncoding.hlsli"
#include "NRD.hlsli"

Texture2D<float4>			gBuffer0	   : register(t0);
Texture2D<float4>			gBuffer1	   : register(t1);
Texture2D<float4>			gBuffer2	   : register(t2);
Texture2D<float>            gDepthTex      : register(t3);
Texture2D<float>            gShadowMask    : register(t4);
RWTexture2D<float4>         gOutput        : register(u0);

ConstantBuffer<CbLighting>  gCbLighting : register(b0);
ConstantBuffer<CBPrePass>   gCbPrePass  : register(b1);

struct ComputeIn {
    uint3 GroupID           : SV_GroupID;           
    uint3 GroupThreadID     : SV_GroupThreadID;     
    uint3 DispatchThreadID  : SV_DispatchThreadID;  
    uint  GroupIndex        : SV_GroupIndex;        
};

void UnpackGBuffer0(ComputeIn cin, out float3 albedo, out float metallic) {
	float4 sampleColor = gBuffer0[cin.DispatchThreadID.xy];
    albedo = sampleColor.rgb;
    metallic = sampleColor.a;
}

void UnpackGBuffer1(ComputeIn cin, out float3 normal, out float roughness) {
	float4 sampleColor = gBuffer1[cin.DispatchThreadID.xy];
	float4 unpackData = NRD_FrontEnd_UnpackNormalAndRoughness(sampleColor);
    normal = unpackData.xyz;
    roughness = unpackData.w;
}

void UnpackGBuffer2(ComputeIn cin, out float3 emission) {
	emission = gBuffer2[cin.DispatchThreadID.xy].rgb;
}

float3 GetWorldPosition(ComputeIn cin) {
	float zNdc = gDepthTex[cin.DispatchThreadID.xy];
    float2 uv = (cin.DispatchThreadID.xy + 0.5f) * gCbPrePass.invRenderSize;
    return WorldPositionFromDepth(uv, zNdc, gCbPrePass.matInvJitteredViewProj);
}

float GetShadow(ComputeIn cin) {
    float shadowData = gShadowMask[cin.DispatchThreadID.xy];
    float shadow = SIGMA_BackEnd_UnpackShadow(shadowData);
    return shadow;
}

[numthreads(THREAD_WRAP_SIZE, 16, 1)]
void CSMain(ComputeIn cin) {
    float3 albedo;
    float metallic;
    UnpackGBuffer0(cin, albedo, metallic);

    float3 N;
    float roughness;
	UnpackGBuffer1(cin, N, roughness);

    float3 worldPosition = GetWorldPosition(cin);
    float3 V = normalize(gCbPrePass.cameraPos - worldPosition);

    float3 emission;
    UnpackGBuffer2(cin, emission);

    float shadow = GetShadow(cin);
    MaterialData materialData = CalcMaterialData(albedo.rgb, roughness, metallic);
    float3 finalColor = shadow * ComputeDirectionLight(gCbLighting.directionalLight, materialData, N, V);
    finalColor += ComputeAmbientLight(gCbLighting.ambientLight, materialData, 1.f);
    finalColor += emission;
    gOutput[cin.DispatchThreadID.xy] = float4(finalColor, 1.0);
}
