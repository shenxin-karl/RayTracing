#include "Atmosphere.hlsli"

struct ComputeIn {
	uint3 GroupID           : SV_GroupID;           
    uint3 GroupThreadID     : SV_GroupThreadID;     
    uint3 DispatchThreadID  : SV_DispatchThreadID;  
    uint  GroupIndex        : SV_GroupIndex;   
};

struct CBSetting {
    float3 lightDir;
    float  cameraPosY;
};


RWTexture2DArray<float3>                        gOutputSkyBoxLut      : register(u0);
ConstantBuffer<CBSetting>                       gSetting              : register(b0);
ConstantBuffer<Atmosphere::AtmosphereParameter> gAtmosphereParameter  : register(b1);

[numthreads(8, 8, 1)]
void CSMain(ComputeIn cin) {
	float3 dimes;
    gOutputSkyBoxLut.GetDimensions(dimes.x, dimes.y, dimes.z);

    float2 uv = (cin.DispatchThreadID.xy + 0.5f) / dimes.xy;
    float3 viewDir = Atmosphere::UVToViewDir(uv);

    float3 cameraWorldPos = float3(0.f, gSetting.cameraPosY, 0.f);
    float3 skyColor = Atmosphere::IntegrateSignalScatting(gAtmosphereParameter, 
		cameraWorldPos, 
		gSetting.lightDir, 
		viewDir);
	gOutputSkyBoxLut[cin.DispatchThreadID] = skyColor;
}