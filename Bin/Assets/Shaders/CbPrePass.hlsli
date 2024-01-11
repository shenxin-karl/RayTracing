#ifndef __CB_PRE_PASS__
#define __CB_PRE_PASS__

struct CBPrePass {
    // camera
	float4x4 matView;
	float4x4 matInvView;
	float4x4 matProj;
	float4x4 matInvProj;
	float4x4 matViewProj; 
	float4x4 matInvViewProj;
	float4x4 matPrevViewProj;
	float3   cameraPos;
	float	 nearClip;
    float3   cameraLookUp;
	float	 farClip;
	float3   cameraLookAt;
    float	 padding0;

    // RenderTarget
	float2   renderTargetSize;
	float2   invRenderTargetSize;

    /// time
	float	totalTime;
	float	deltaTime;

	float4  zBufferParams;
};

#endif