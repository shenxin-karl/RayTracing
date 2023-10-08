
struct Viewport {
    float left;
    float top;
    float right;
    float bottom;
};

struct RayGenConstantBuffer {
    Viewport viewport;
    Viewport stencil;
};

RaytracingAccelerationStructure         gScene          : register(t0);
RWTexture2D<float4>                     gRenderTarget   : register(u0);
ConstantBuffer<RayGenConstantBuffer>    gRayGenCB       : register(b0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct RayPayload {
    float4 color;
};

bool IsInsideViewport(float2 p, Viewport viewport) {
    return (p.x >= viewport.left && p.x <= viewport.right)
        && (p.y >= viewport.top && p.y <= viewport.bottom);
}

[shader("raygeneration")]
void MyRayGenShader() {
    float2 lerpValues = (float)DispatchRaysIndex() / (float2)DispatchRaysDimensions();
    float3 rayDir = float3(0, 0, 1);
    float3 origin = float3(
        lerp(gRayGenCB.viewport.left, gRayGenCB.viewport.right, lerpValues.x),
        lerp(gRayGenCB.viewport.top, gRayGenCB.viewport.bottom, lerpValues.y),
        0.0f);

    if (IsInsideViewport(origin.xy, gRayGenCB.stencil)) {
        RayDesc ray;
        ray.Origin = origin;
        ray.Direction = rayDir;
        // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
        // TMin should be kept small to prevent missing geometry at close contact areas.
        ray.TMin = 0.001;
        ray.TMax = 10000.0;
        RayPayload payload = { float4(0, 0, 0, 0) };
        TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);
        gRenderTarget[DispatchRaysIndex().xy] = payload.color;
    } else {
        gRenderTarget[DispatchRaysIndex().xy] = float4(lerpValues, 0, 1);
    }
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr) {
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    payload.color = float4(barycentrics, 1);
}

[shader("miss")]
void MyMissShader(inout RayPayload payload) {
    payload.color = float4(0, 0, 0, 1);
}