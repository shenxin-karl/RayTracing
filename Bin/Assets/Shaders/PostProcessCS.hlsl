#include "Tonemappers.hlsli"

Texture2D<float4>   gInput  : register(t0);
RWTexture2D<float4> gOutput : register(u0);

cbuffer CbSetting : register(b0) {
	float exposure;
	int	  toneMapperType;
};

struct ComputeIn {
    uint3 DispatchThreadID  : SV_DispatchThreadID;  
};

[numthreads(8, 8, 1)]
void CSMain(ComputeIn cin) {
	float4 input = gInput[cin.DispatchThreadID.xy];
	// input.rgb *= exposure;
	// input.rgb = ApplyToneMapping(input.rgb, toneMapperType);
	// input.rgb = ApplyGammaCorrection(input.rgb);
	gOutput[cin.DispatchThreadID.xy] = input;
}