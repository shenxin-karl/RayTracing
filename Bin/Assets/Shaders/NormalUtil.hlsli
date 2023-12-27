#pragma once

float2 NormalEncode(float3 N) {
	float2 encode = normalize(N.xy) * sqrt(N.z * 0.5f + 0.5f);
	return encode;
}

float3 NormalDecode(float2 encode) {
	float3 N;
	N.z = length(encode.xy) * 2.f - 1.f;
	N.xy = normalize(encode.xy) * sqrt(1.f - N.z * N.z);
	return N;
}

float3 NormalBlendUDK(float3 n1, float3 n2) {
	return normalize(float3(n1.xy + n2.xy, n1.z));
}

float3 NormalBlendReoriented(float3 n1, float3 n2) {
	float3x3 nBasis = float3x3(
		float3(n1.z, n1.y, -n1.x), 
		float3(n1.x, n1.z, -n1.y), 
		float3(n1.x, n1.y,  n1.z));
	return normalize(n2.x * nBasis[0] + n2.y * nBasis[1] + n2.z * nBasis[2]);
}