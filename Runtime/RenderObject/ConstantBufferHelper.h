#pragma once
#include "D3d12/Context.h"
#include "Foundation/GlmStd.hpp"
#include "D3d12/D3dStd.h"

class SceneLightManager;
class Transform;
class GameObject;

namespace cbuffer {

constexpr size_t kAlignment = sizeof(glm::vec4);

// clang-format off
struct alignas(kAlignment) CbPreObject {
	glm::mat4x4 matWorld;
	glm::mat4x4 matInvWorld;
	glm::mat4x4 matNormal;
	glm::mat4x4 matPrevWorld;
};

struct alignas(kAlignment) CbPrePass {
	glm::mat4x4 matView;
	glm::mat4x4 matInvView;
	glm::mat4x4 matProj;
	glm::mat4x4 matInvProj;
	glm::mat4x4 matViewProj;
	glm::mat4x4 matInvViewProj;
	glm::mat4x4 matPrevViewProj;
	glm::vec3   cameraPos;
	float		nearClip;
	glm::vec3	cameraLookUp;
	float		farClip;
	glm::vec3	cameraLookAt;
	float		padding0;

	glm::vec2	renderTargetSize;
	glm::vec2	invRenderTargetSize;

	float		totalTime;
	float		deltaTime;
	glm::vec2	padding1;

	glm::vec4   zBufferParams;
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

auto MakeCbPreObject(const Transform *pTransform) -> CbPreObject;
auto MakeCbPrePass(const GameObject *pCameraGO) -> CbPrePass;
auto MakeCbLighting(const SceneLightManager *pSceneLightManager) -> CbLighting;

D3D12_GPU_VIRTUAL_ADDRESS AllocPreObjectCBuffer(dx::Context *pContext, const Transform *pTransform);
D3D12_GPU_VIRTUAL_ADDRESS AllocPrePassCBuffer(dx::Context *pContext, const GameObject *pCameraGO);

}    // namespace cbuffer
