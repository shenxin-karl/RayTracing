#include "DepthUtil.hlsli"
#include "RayTracingUtils.hlsli"

struct RayGenCB {
    float4x4    matInvViewProj;
    float3      lightDirection;
	float       maxCosineTheta;
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

// local root signature paramaters
// alpha test usage only

struct CBMaterialIndex {
    uint index;
};

struct Material {
    float   alpha;
    float   cutoff;
    int     albedoTextureIndex;
    int     isAlphaTest;
    uint    vertexStride;
    uint    uv0Offset;
    uint    sampleStateIndex;
};
ConstantBuffer<CBMaterialIndex>     gMaterialIndex          : register(b0, space1);
ByteAddressBuffer                   gVertexBuffer           : register(t0, space1);
ByteAddressBuffer                   gIndexBuffer            : register(t1, space1);
Texture2D<float4>                   gAlbedoTextureList[]    : register(t2, space1);
StructuredBuffer<Material>          gAllInstanceMaterial    : register(t3, space1);
SamplerState                        gStaticSamplerState[]   : register(s0, space1);

struct HitGeometryParam {
    MyAttributes    attr;
    uint            primitiveIndex;
    bool            hitGeometry;     
};

void HitGeometryTest(inout HitGeometryParam param) {
    Material mat = gAllInstanceMaterial[gMaterialIndex.index];
    if (!mat.isAlphaTest) {
        param.hitGeometry = true;
        return;
    }

    // primitiveIndex * triangleSize * sizeof(uint16);
    uint offset = param.primitiveIndex * 3 * 2;
    uint3 indices = Load3x16BitIndices(gIndexBuffer, offset);

    float2 uv0 = gVertexBuffer.Load<float2>(indices[0] * mat.vertexStride + mat.uv0Offset);
    float2 uv1 = gVertexBuffer.Load<float2>(indices[1] * mat.vertexStride + mat.uv0Offset);
    float2 uv2 = gVertexBuffer.Load<float2>(indices[2] * mat.vertexStride + mat.uv0Offset);
    float2 uv = BarycentricBlend(uv0, uv1, uv2, param.attr.barycentrics);
    float alpha = mat.alpha;

    SamplerState samplerState = gStaticSamplerState[mat.sampleStateIndex];
    Texture2D<float4> albedoTexture = gAlbedoTextureList[mat.albedoTextureIndex];
    alpha *= albedoTexture.SampleLevel(samplerState, uv, 0).a;
    param.hitGeometry = alpha >= mat.cutoff;
}


struct ShadowRayPayload {
	float visFactor;  // Will be 1.0 for fully lit, 0.0 for fully shadowed
};

RayDesc GenerateRay() {
    uint2 index = DispatchRaysIndex().xy;
    float2 uv = (index + 0.5f) / DispatchRaysDimensions().xy;
    float zNdc = gDepthTex[index];

    RayDesc rayDesc;
    rayDesc.Origin = WorldPositionFromDepth(uv, zNdc, gRayGenCB.matInvViewProj);
    rayDesc.Direction = gRayGenCB.lightDirection;
    rayDesc.TMin = gRayGenCB.minT;
    rayDesc.TMax = gRayGenCB.maxT;
    return rayDesc;
}

[shader("raygeneration")]
void ShadowRaygenShader() {
    ShadowRayPayload payload = { 1.f };
    RayDesc rayDesc = GenerateRay();
    TraceRay(gScene, 
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_CULL_NON_OPAQUE,
        0xff,
        0,          // RayContributionToHitGroupIndex
        0,          // MultiplierForGeometryContributionToShaderIndex
        0,          // MissShaderIndex
        rayDesc,
        payload
    );
    gOutputTexture[DispatchRaysIndex().xy] = payload.visFactor;
}

[shader("anyhit")]
void ShadowAnyHitShader(inout ShadowRayPayload payload, in MyAttributes attr) {
    HitGeometryParam param;
    param.attr = attr;
    param.primitiveIndex = PrimitiveIndex();
    param.hitGeometry = false;
    CallShader(InstanceIndex(), param);        // call HitGeometryTest

    // 光线击中不透明物体, 或者 AlphaTest 物体但是 Test 失败的情况
    // 停止搜索光线, 直接标记为处于阴影中
    if (param.hitGeometry) {     
        payload.visFactor = 0.f;
        AcceptHitAndEndSearch();
    }
}

[shader("miss")]
void ShadowMissShader(inout ShadowRayPayload payload) {
    payload.visFactor = 1.0;
}