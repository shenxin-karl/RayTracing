#pragma once
#include "Components/Camera.h"
#include "D3d12/Context.h"
#include "Foundation/GlmStd.hpp"
#include "D3d12/D3dStd.h"

class SceneLightManager;
class Transform;
class GameObject;

namespace cbuffer {

constexpr size_t kAlignment = sizeof(glm::vec4);

using float4x4 = glm::mat4x4;
using float4 = glm::vec4;
using float3 = glm::vec3;
using float2 = glm::vec2;

// clang-format off
struct alignas(kAlignment) CbPreObject {
    float4x4 matWorld;
    float4x4 matInvWorld;
    float4x4 matNormal;
    float4x4 matWorldPrev;     
};

struct alignas(kAlignment) CbPrePass {
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
	float4x4 matJitterViewProj;
	float4x4 matInvJitterViewProj;

	float3   cameraPos;
	float	 nearClip;
    float3   cameraLookUp;
	float	 farClip;
	float3   cameraLookAt;
    float	 mipBias;

    // RenderTarget
	float2   renderTargetSize;
	float2   invRenderTargetSize;

    /// time
	float	totalTime;
	float	deltaTime;
	float2	padding1;

	float4  zBufferParams;
};


struct alignas(kAlignment) AmbientLight {
	glm::vec3	color;
    float		intensity;
};

struct alignas(kAlignment) DirectionalLight {
    glm::vec3	color;
    float		intensity;
    glm::vec3	direction;
    float		padding0;
};

struct alignas(kAlignment) SpotLight {
    glm::vec3	strength;
    float		falloffStart;
    glm::vec3	direction;
    float		falloffEnd;
    glm::vec3	position;
    float		spotPower;
};

struct alignas(kAlignment) PointLight {
    glm::vec3	color;
    float		intensity;
    glm::vec3	position;
    float		range;
};

struct CbLighting {
	AmbientLight	 ambientLight;
	DirectionalLight directionalLight;
};

// clang-format on

auto MakeCbPrePass(const CameraState *pCurrentCameraState, const CameraState *pPreviousCameraState = nullptr) -> CbPrePass;
auto MakeCbLighting(const SceneLightManager *pSceneLightManager) -> CbLighting;

}    // namespace cbuffer
