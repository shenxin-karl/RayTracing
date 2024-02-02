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

}    // namespace cbuffer
