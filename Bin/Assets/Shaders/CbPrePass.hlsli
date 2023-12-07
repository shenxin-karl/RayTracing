#ifndef __CB_PRE_PASS__
#define __CB_PRE_PASS__

struct CBPrePass {
    // camera
	float4x4 gMatView;
	float4x4 gMatInvView;
	float4x4 gMatProj;
	float4x4 gMatInvProj;
	float4x4 gMatViewProj; 
	float4x4 gMatInvViewProj;
	float3   gCameraPos;
	float	 gNearClip;
    float3   gCameraLookUp;
	float	 gFarClip;
	float3   gCameraLookAt;
    float	 padding0;

    // RenderTarget
	float2   gRenderTargetSize;
	float2   gInvRenderTargetSize;

    /// time
	float	gTotalTime;
	float	gDeltaTime;
};

#endif