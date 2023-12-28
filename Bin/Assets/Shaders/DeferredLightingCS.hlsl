#include "CbLighting.hlsli"
#include "CbPrePass.hlsli"
#include "CookTorrance.hlsli"
#include "NormalUtil.hlsli"
#include "DepthUtil.hlsli"

Texture2D<float4>			gBuffer0	: register(t0);
Texture2D<float4>			gBuffer1	: register(t1);
Texture2D<float4>			gBuffer2	: register(t2);
Texture2D<float>            gDepthTex   : register(t3);
RWTexture2D<float4>         gOutput     : register(u0);

ConstantBuffer<CbLighting>  gCbLighting : register(b0);
ConstantBuffer<CBPrePass>   gCbPrePass  : register(b1);

struct ComputeIn {
    uint3 GroupID           : SV_GroupID;           
    uint3 GroupThreadID     : SV_GroupThreadID;     
    uint3 DispatchThreadID  : SV_DispatchThreadID;  
    uint  GroupIndex        : SV_GroupIndex;        
};

void UnpackGBuffer0(ComputeIn cin, out float3 albedo, out float ao) {
	float4 sampleColor = gBuffer0[cin.DispatchThreadID.xy];
    albedo = sampleColor.rgb;
    ao = sampleColor.a;
}

void UnpackGBuffer1(ComputeIn cin, out float3 normal, out float metallic, out float roughness) {
	float4 sampleColor = gBuffer1[cin.DispatchThreadID.xy];
	normal = NormalDecode(sampleColor.xy);
    metallic = sampleColor.z;
    roughness = sampleColor.w;
}

void UnpackGBuffer2(ComputeIn cin, out float3 emission) {
	emission = gBuffer2[cin.DispatchThreadID.xy].rgb;
}

float3 GetWorldPosition(ComputeIn cin) {
	float zNdc = gDepthTex[cin.DispatchThreadID.xy];
    float2 uv = (cin.DispatchThreadID.xy + 0.5f) * gCbPrePass.gInvRenderTargetSize;
    return WorldPositionFromDepth(uv, zNdc, gCbPrePass.gMatInvViewProj);
}

float3 Test0(ComputeIn cin) {
	float zNdc = gDepthTex[cin.DispatchThreadID.xy];
    return Linear01Depth(zNdc, gCbPrePass.gNearClip, gCbPrePass.gFarClip);
}

[numthreads(16, 32, 1)]
void CSMain(ComputeIn cin) {
	float3 albedo;
    float ao;
    UnpackGBuffer0(cin, albedo, ao);

    float3 N;
    float metallic;
    float roughness;
	UnpackGBuffer1(cin, N, metallic, roughness);

    float3 worldPosition = GetWorldPosition(cin);
    float3 V = normalize(gCbPrePass.gCameraPos - worldPosition);

    float3 emission;
    UnpackGBuffer2(cin, emission);

    MaterialData materialData = CalcMaterialData(albedo.rgb, roughness, metallic);
    float3 finalColor = ComputeDirectionLight(gCbLighting.directionalLight, materialData, N, V);
    finalColor += ComputeAmbientLight(gCbLighting.ambientLight, materialData, ao);
    finalColor += emission;
    gOutput[cin.DispatchThreadID.xy] = float4(finalColor, 1.f);


    // gOutput[cin.DispatchThreadID.xy] = float4(N * 0.5 + 0.5, 1.0);
    // gOutput[cin.DispatchThreadID.xy] = float4(roughness, 0.f, 0.f, 1.f);
    // gOutput[cin.DispatchThreadID.xy] = Test0(cin);



    // float3 V = normalize(gCbPrePass.gCameraPos - pin.position);

    // MaterialData materialData = CalcMaterialData(albedo, roughness, metallic);
    // float3 finalColor = ComputeDirectionLight(gCbLighting.directionalLight, materialData, N, V);
    // finalColor += ComputeAmbientLight(gCbLighting.ambientLight, materialData, ao);
    // finalColor += GetEmission(pin);
    // return float4(finalColor, albedo.a);
}
