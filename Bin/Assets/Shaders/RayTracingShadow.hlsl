#include "DepthUtil.hlsli"
#include "RayTracingUtils.hlsli"

struct RayGenCB {
    float4x4    matInvViewProj;
    float3      lightDirection;
	float       maxCosineTheta;
    uint        enableSoftShadow;
	uint        frameCount;     // Frame counter, used to perturb random seed each frame
    float       maxT;           // Max distance to start a ray to avoid self-occlusion
	float       minT;           // Min distance to start a ray to avoid self-occlusion
};

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

// global root signature parameters
RaytracingAccelerationStructure     gScene                  : register(t0);
Texture2D<float>                    gDepthTex               : register(t1);
ConstantBuffer<RayGenCB>            gRayGenCB               : register(b0);
RWTexture2D<float>                  gOutputTexture          : register(u0);
SamplerState                        gStaticSamplerState[]   : register(s0);

// local root signature paramaters
// alpha test usage only

struct CBMaterialIndex {
    uint index;
};

struct ShadowRayPayload {
	float visFactor;  // Will be 1.0 for fully lit, 0.0 for fully shadowed
};


// Generates a seed for a random number generator from 2 inputs plus a backoff
uint InitRand(uint val0, uint val1, uint backoff = 16) {
	uint v0 = val0, v1 = val1, s0 = 0;
	[unroll]
	for (uint n = 0; n < backoff; n++) {
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float NextRand(inout uint s) {
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

float3 GetPerpendicularVector(float3 u) {
	float3 a = abs(u);
	uint xm = ((a.x - a.y)<0 && (a.x - a.z)<0) ? 1 : 0;
	uint ym = (a.y - a.z)<0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	return cross(u, float3(xm, ym, zm));
}

float3 GetConeSample(inout uint randSeed, float3 hitNorm, float cosThetaMax) {
	// Get 2 random numbers to select our sample with
	float2 randVal = float2(NextRand(randSeed), NextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
	float3 bitangent = GetPerpendicularVector(hitNorm);
	float3 tangent = cross(bitangent, hitNorm);

	float cosTheta = (1.0 - randVal.x) + randVal.x * cosThetaMax;
	float r = sqrt(1.0 - cosTheta * cosTheta);
	float phi = randVal.y * 2.0 * 3.14159265f;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi)) + bitangent * (r * sin(phi)) + hitNorm.xyz * cosTheta;
}


RayDesc GenerateRay() {
    uint2 index = DispatchRaysIndex().xy;
    float2 uv = (index + 0.5f) / DispatchRaysDimensions().xy;
    SamplerState linearClamp = gStaticSamplerState[3];
    float zNdc = gDepthTex.SampleLevel(linearClamp, uv, 0);

    float3 direction = gRayGenCB.lightDirection;
    if (gRayGenCB.enableSoftShadow != 0) {
	    uint seed = InitRand(index.x + index.y * DispatchRaysDimensions().x, gRayGenCB.frameCount);
	    direction = GetConeSample(seed, gRayGenCB.lightDirection, gRayGenCB.maxCosineTheta);
    }

    RayDesc rayDesc;
    rayDesc.Origin = WorldPositionFromDepth(uv, zNdc, gRayGenCB.matInvViewProj);
    rayDesc.Direction = direction;
    rayDesc.TMin = gRayGenCB.minT;
    rayDesc.TMax = gRayGenCB.maxT;
    return rayDesc;
}

[shader("raygeneration")]
void ShadowRaygenShader() {
    ShadowRayPayload payload = { 1.f };
    RayDesc rayDesc = GenerateRay();

    TraceRay(gScene, 
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_NON_OPAQUE ,
        ~0,
        0,          // RayContributionToHitGroupIndex
        1,          // MultiplierForGeometryContributionToShaderIndex
        0,          // MissShaderIndex
        rayDesc,
        payload
    );
    gOutputTexture[DispatchRaysIndex().xy] = payload.visFactor;
}

struct ShadowMaterial {
    float   alpha;
    float   cutoff;
    int     albedoTextureIndex;
    uint    vertexStride;
    uint    uv0Offset;
    uint    sampleStateIndex;
    uint    skipGeometry;
};

ConstantBuffer<CBMaterialIndex>     lMaterialIndex          : register(b0, space1);
ByteAddressBuffer                   lVertexBuffer           : register(t0, space1);
ByteAddressBuffer                   lIndexBuffer            : register(t1, space1);
StructuredBuffer<ShadowMaterial>    lAllInstanceMaterial    : register(t2, space1);
Texture2D<float4>                   lAlbedoTextureList[]    : register(t3, space1);

[shader("anyhit")]
void ShadowAlphaTestAnyHitShader(inout ShadowRayPayload payload, in MyAttributes attr) {
    ShadowMaterial mat = lAllInstanceMaterial[lMaterialIndex.index];
    if (mat.skipGeometry != 0) {
        IgnoreHit();
    }

    // primitiveIndex * triangleSize * sizeof(uint16);
    uint offset = PrimitiveIndex() * 3 * 2;
    uint3 indices = Load3x16BitIndices(lIndexBuffer, offset);

    float2 uv0 = lVertexBuffer.Load<float2>(indices[0] * mat.vertexStride + mat.uv0Offset);
    float2 uv1 = lVertexBuffer.Load<float2>(indices[1] * mat.vertexStride + mat.uv0Offset);
    float2 uv2 = lVertexBuffer.Load<float2>(indices[2] * mat.vertexStride + mat.uv0Offset);
    float2 uv = BarycentricBlend(uv0, uv1, uv2, attr.barycentrics);
    float alpha = mat.alpha;

    SamplerState samplerState = gStaticSamplerState[mat.sampleStateIndex];
    Texture2D<float4> albedoTexture = lAlbedoTextureList[mat.albedoTextureIndex];
    alpha *= albedoTexture.SampleLevel(samplerState, uv, 0).a;
    if (alpha > mat.cutoff) {     
        payload.visFactor = 0.f;
        AcceptHitAndEndSearch();
    }
}

[shader("anyhit")]
void ShadowOpaqueAnyHitShader(inout ShadowRayPayload payload, in MyAttributes attr) {
    payload.visFactor = 0.f;
    AcceptHitAndEndSearch();
}

[shader("miss")]
void ShadowMissShader(inout ShadowRayPayload payload) {
    payload.visFactor = 1.0;
}