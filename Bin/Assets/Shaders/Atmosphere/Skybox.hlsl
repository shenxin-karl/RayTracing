#include "Atmosphere.hlsli"

struct VertexIn {
    float3 position  : POSITION;
};

struct VertexOut {
	float4 SVPosition : SV_POSITION;
	float3 uv		  : TEXCOORD0;
};

struct CBSetting {
	float4x4 matViewProj;
	float	 reversedZ;
};

ConstantBuffer<CBSetting> gCBSetting			: register(b0);
Texture2D<float3>		  gSkyBoxLutTex			: register(t0);
SamplerState			  gLinearClamp			: register(s0);

VertexOut VSMain(VertexIn vin) {
	VertexOut vout = (VertexOut)0;
	vout.SVPosition = mul(gCBSetting.matViewProj, float4(vin.position, 1.f));
	if (gCBSetting.reversedZ > 0.1f) {
		vout.SVPosition.z = 0.f;
	}
	vout.uv = vin.position;
	return vout;
}

float4 PSMain(VertexOut pin) : SV_Target {
	float3 viewDir = normalize(pin.uv);
	float2 uv = Atmosphere::ViewDirToUV(viewDir);
	float3 skyColor = gSkyBoxLutTex.SampleLevel(gLinearClamp, uv, 0);
	return float4(skyColor, 1.f);
}