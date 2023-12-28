#pragma once

float2 _OctWrap(float2 v) {
	return (1.0 - abs(v.yx)) * (v.xy >= 0.0 ? 1.0 : -1.0);
}
 
float2 NormalEncode(float3 n) {
	n /= (abs(n.x) + abs(n.y) + abs(n.z));
	n.xy = n.z >= 0.0 ? n.xy : _OctWrap(n.xy);
	n.xy = n.xy * 0.5 + 0.5;
	return n.xy;
}
 
float3 NormalDecode(float2 f)
{
	f = f * 2.0 - 1.0;
	// https://twitter.com/Stubbesaurus/status/937994790553227264
	float3 n = float3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
	float t = saturate(-n.z);
	n.xy += n.xy >= 0.0 ? -t : t;
	return normalize(n);
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