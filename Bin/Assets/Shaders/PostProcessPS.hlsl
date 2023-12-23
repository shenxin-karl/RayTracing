#include "FullScreenVS.hlsli"
#include "Tonemappers.hlsli"

Texture2D<float4> gInput        : register(t0);
SamplerState      gLinearClamp  : register(s0);

cbuffer CbSetting : register(b0) {
	float exposure;
	int	  toneMapperType;
};

float4 PSMain(VertexOut pin) : SV_TARGET0 {
	float4 input = gInput.SampleLevel(gLinearClamp, pin.uv, 0);
	input *= exposure;
	input.rgb = ApplyToneMapping(input.rgb, toneMapperType);
	input.rgb = ApplyGammaCorrection(input.rgb);
	return input;
}