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

float3 ApplyToneMapping(float3 input, int type) {
	switch (type) {
	case 0: return AMDTonemapper(input);
	case 1: return DX11DSK(input);
	case 2: return Reinhard(input);
	case 3: return Uncharted2Tonemap(input);
	case 4: return ACESFilm(input);
	default:
		return input;
	}
}

float3 ApplyGammaCorrection(float3 input) {
	static const float gamma = 2.2;
	return pow(input, 1.f / gamma);
}

[numthreads(8, 8, 1)]
void CSMain(ComputeIn cin) {
	float4 input = gInput[cin.DispatchThreadID.xy];
	input.rgb *= exposure;
	input.rgb = ApplyToneMapping(input.rgb, toneMapperType);
	input.rgb = ApplyGammaCorrection(input.rgb);
	gOutput[cin.DispatchThreadID.xy] = input;
}