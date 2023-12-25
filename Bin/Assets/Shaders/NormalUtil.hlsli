#pragma once

float2 NormalEncode(float3 n) {
    float2 enc = normalize(n.xy) * (sqrt(-n.z*0.5+0.5));
    enc = enc*0.5+0.5;
    return enc;
}

float3 NormalDecode(float4 enc) {
    float4 nn = enc*float4(2,2,0,0) + float4(-1,-1,1,-1);
    half l = dot(nn.xyz,-nn.xyw);
    nn.z = l;
    nn.xy *= sqrt(l);
    return nn.xyz * 2 + float3(0,0,-1);
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