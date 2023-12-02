struct SceneConstantBuffer {
    float4x4 projectionToWorld;
    float4 cameraPosition;
    float4 lightPosition;
    float4 lightAmbientColor;
    float4 lightDiffuseColor;
    float  time;
};

struct CubeConstantBuffer {
    float4 albedo;
    float  noiseTile;
};

struct Vertex {
    float3 position;
    float3 normal;
};

struct RayPayload {
    float4 color;
};



RaytracingAccelerationStructure     gScene          : register(t0);
ByteAddressBuffer                   gIndices        : register(t1);
StructuredBuffer<Vertex>            gVertices       : register(t2);
TextureCube<float4>                 gCubeMap        : register(t3);

RWTexture2D<float4>                 gOutput         : register(u0);

ConstantBuffer<SceneConstantBuffer> gSceneCB        : register(b0);
ConstantBuffer<CubeConstantBuffer>  gCubeCB         : register(b1);
SamplerState                        gLinearSampler  : register(s0);       

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

// Load three 16 bit indices from a byte addressed buffer.
uint3 Load3x16BitIndices(uint offsetBytes) {
    uint3 indices;
    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;    
    const uint2 four16BitIndices = gIndices.Load2(dwordAlignedOffset);
 
    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes){
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else {// Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }
    return indices;
}


// Retrieve hit world position.
float3 HitWorldPosition() {
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction) {
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(gSceneCB.projectionToWorld, float4(screenPos, 0, 1));

    world.xyz /= world.w;
    origin = gSceneCB.cameraPosition.xyz;
    direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void MyRaygenShader() {
    float3 rayDir;
    float3 origin;
    
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    // Write the raytraced color to the output texture.
    gOutput[DispatchRaysIndex().xy] = payload.color;
}

// Diffuse lighting calculation.
float4 CalculateDiffuseLighting(float3 hitPosition, float4 albedo, float3 normal) {
    float3 pixelToLight = normalize(gSceneCB.lightPosition.xyz - hitPosition);
    // Diffuse contribution.
    float fNDotL = max(0.0f, dot(pixelToLight, normal));
    return albedo * gSceneCB.lightDiffuseColor * fNDotL;
}

inline float4 CalculateAmbientLighting(float4 albedo) {
    return albedo * gSceneCB.lightAmbientColor;
}

inline float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr) {
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}


#define HASHSCALE3 float3(0.1031, 0.1030, 0.0973)

float2 hash22(float2 p) {
    float3 p3 = frac(p.xyx * HASHSCALE3);
    float tem =  dot(p3, p3.yzx + float3(19.19, 19.19, 19.19));
    p3 = p3 + float3(tem, tem, tem);
    return frac((p3.xx + p3.yz) * p3.zy);
}

float wnoise(float2 p, float time)  {
    float2 n = floor(p);
    float2 f = frac(p);
    float md = 5.0;
    float2 m = float2(0.0 ,0.0);
    for (int i = -1; i <= 1; i++) 
    {
        for (int j = -1; j <= 1; j++) 
        {
            float2 g = float2(i, j);
            float2 o = hash22(n + g);
            o = float2(0.5, 0.5) + 0.5* sin(float2(time, time) + 6.28 * o);
            float2 r = g + o - f;
            float d = dot(r, r);
            if (d < md) 
            {
                md = d;
                m = n + g + o;
            }
        }
    }
    return md;
}


[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr) {
    float3 hitPosition = HitWorldPosition();

    // Get the base index of the triangle's first 16 bit index.
    uint indexSizeInBytes = 2;
    uint indicesPerTriangle = 3;
    uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load up 3 16 bit indices for the triangle.
    const uint3 indices = Load3x16BitIndices(baseIndex);

    // Retrieve corresponding vertex normals for the triangle vertices.
    float3 vertexNormals[3] = { 
        gVertices[indices[0]].normal, 
        gVertices[indices[1]].normal, 
        gVertices[indices[2]].normal 
    };

    // Compute the triangle's normal.
    // This is redundant and done for illustration purposes 
    // as all the per-vertex normals are the same and match triangle's normal in this sample. 
    float3 triangleNormal = HitAttribute(vertexNormals, attr);

    float u = dot(hitPosition, float3(triangleNormal.yzx));
    float v = dot(hitPosition, float3(triangleNormal.zxy));

    float noise = wnoise(float2(u, v) * gCubeCB.noiseTile, gSceneCB.time);
    float4 albedo = saturate(gCubeCB.albedo * (noise + 0.3f));

    float4 diffuseColor = CalculateDiffuseLighting(hitPosition, albedo, triangleNormal);
    float4 ambientColor = CalculateAmbientLighting(albedo);
    float4 color = ambientColor + diffuseColor;
    payload.color = color;
}

[shader("miss")]
void MyMissShader(inout RayPayload payload) {
    float3 direction = WorldRayDirection();
    payload.color = gCubeMap.SampleLevel(gLinearSampler, direction, 0);
}
