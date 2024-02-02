#ifndef __CB_PRE_PASS__
#define __CB_PRE_PASS__

struct CBPrePass {
	float4x4 matView;
	float4x4 matInvView;
	float4x4 matProj;
	float4x4 matInvProj;
	float4x4 matViewProj;
	float4x4 matInvViewProj;

	// previous frame matrix
	float4x4 matViewProjPrev;
	float4x4 matJitterViewProjPrev;

	// current frame jitter matrix
	float4x4 matJitteredProj;
	float4x4 matInvJitteredProj;

	float4x4 matJitteredViewProj;
	float4x4 matInvJitteredViewProj;

	float2   viewportJitter;
	float2   viewportJitterPrev;

	float2   cameraJitter;
	float2   cameraJitterPrev;

	float3   cameraPos;
	float	 nearClip;
    float3   cameraLookUp;
	float	 farClip;
	float3   cameraLookAt;
    float	 mipBias;

    // time
	float	totalTime;
	float	deltaTime;

	float	radianFov;
	float	degreeFov;

	float4  zBufferParams;

	float2  renderSize;
	float2  displaySize;
	float2  invRenderSize;
	float2  invDisplaySize;
};

#endif