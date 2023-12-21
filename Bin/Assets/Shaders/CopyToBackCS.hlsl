#pragma once

struct ComputeIn {
    uint3 GroupID           : SV_GroupID;           
    uint3 GroupThreadID     : SV_GroupThreadID;     
    uint3 DispatchThreadID  : SV_DispatchThreadID;  
    uint  GroupIndex        : SV_GroupIndex;        
};

Texture2D<float4>   gSrcTex : register(t0);
RWTexture2D<float4> gDstTex : register(u0);

[numthreads(16, 16, 1)]
void CSMain(ComputeIn cin) {
	gDstTex[cin.DispatchThreadID.xy] = gSrcTex[cin.DispatchThreadID.xy];
}