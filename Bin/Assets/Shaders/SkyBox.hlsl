
cbuffer CBSetting : register(b0) {
	float4x4 gViewProj;
	float	 reversedZ;
};

TextureCube<float3>		 		gCubeMap       	: register(t0);
SamplerState			 		gSamLinearWrap 	: register(s0);

struct VertexIn {
	float3 position		: POSITION;
};

struct VertexOut {
	float4 SVPosition	: SV_Position;
	float3 uv			: TEXCOORD;
};

VertexOut VSMain(VertexIn vin) {
	VertexOut vout;
	vout.SVPosition = mul(gViewProj, float4(vin.position, 1.0)).xyww;
	if (reversedZ > 0.1f) {
		vout.SVPosition.z = 0.f;
	}

	vout.uv = vin.position;
	return vout;
}

float4 PSMain(VertexOut pin) : SV_Target {
	float3 texColor = gCubeMap.Sample(gSamLinearWrap, pin.uv);
	return float4(texColor, 1.0);
}