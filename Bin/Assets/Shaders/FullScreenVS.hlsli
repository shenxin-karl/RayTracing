#pragma once

static const float4 FullScreenVertsPos[3] = { float4(-1, 1, 1, 1), float4(3, 1, 1, 1), float4(-1, -3, 1, 1) };
static const float2 FullScreenVertsUVs[3] = { float2(0, 0), float2(2, 0), float2(0, 2) };

struct VertexOut {
	float4 SVPosition	: SV_POSITION;
	float2 uv			: TEXCOORD0;
};

VertexOut VSMain(uint vertexID : SV_VertexID) {
	VertexOut vout;
	vout.SVPosition = FullScreenVertsPos[vertexID];
	vout.uv = FullScreenVertsUVs[vertexID];
	return vout;
}