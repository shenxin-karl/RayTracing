#pragma once

float ViewSpaceDepth(float zNdc, float4 zBufferParams) {
	return 1.0 / (zBufferParams.z * zNdc + zBufferParams.w);
}

float Linear01Depth(float zNdc, float4 zBufferParams) {
    return 1.0 / (zBufferParams.x * zNdc + zBufferParams.y);
}

// from https://www.gamedev.net/forums/topic/703933-world-position-from-depth/5413262/

float3 WorldPositionFromDepth(float2 uv, float zNdc, float4x4 matInvViewProj) {
    float4 pos = float4(uv * 2.0 - 1.0, zNdc, 1.0);
    pos.y *= -1.0;
    float4 worldPos = mul(matInvViewProj, pos);
    worldPos.xyz /= worldPos.w;
    return worldPos.xyz;
}