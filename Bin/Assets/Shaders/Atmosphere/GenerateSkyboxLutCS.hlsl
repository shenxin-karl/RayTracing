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

ConstantBuffer<CBSetting>           gSetting              : register(b0);
ConstantBuffer<AtmosphereParameter> gAtmosphereParameter  : register(b1);
RWTexture2DArray<float3>            gOutputSkyBoxLut      : register(u0);

const static float3x3 sRotateCubeFace[6] = {
    float3x3(float3(+0,  +0, +1), float3(+0, -1, +0), float3(-1, +0, +0) ),   // +X
    float3x3(float3(+0,  +0, -1), float3(+0, -1, +0), float3(+1, +0, +0) ),   // -X
    float3x3(float3(+1,  +0, +0), float3(+0, +0, +1), float3(+0, +1, +0) ),   // +Y
    float3x3(float3(+1,  +0, +0), float3(+0, +0, -1), float3(+0, -1, +0) ),   // -Y
    float3x3(float3(+1,  +0, +0), float3(+0, -1, +0), float3(+0, +0, +1) ),   // +Z
    float3x3(float3(-1,  +0, +0), float3(+0, -1, +0), float3(+0, +0, -1) )    // -Z
};

float3 CalcDirection(ComputeIn cin) {
	float3 dims;
    gOutputSkyBoxLut.GetDimensions(dims.x, dims.y, dims.z);

	uint index = cin.DispatchThreadID.z;
	float2 uv = (cin.DispatchThreadID.xy + 0.5f) / dims;
    float3 direction = normalize(mul(sRotateCubeFace[index], float3(uv, 0.5)));       // w
    return direction;
}

[numthreads(8, 8, 1)]
void CSMain(ComputeIn cin) {
    float3 direction = CalcDirection(cin);
    float3 cameraWorldPos = float3(0.f, gSetting.cameraPosY, 0.f);
	gOutputSkyBoxLut[cin.DispatchThreadID] = IntegrateSignalScatting(gAtmosphereParameter, 
    cameraWorldPos, 
    gSetting.lightDir, 
    direction);
}