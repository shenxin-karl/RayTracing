#pragma once
#include "Renderer.h"
#include "D3d12/AccelerationStructure.h"
#include "D3d12/DescriptorHandle.h"
#include "D3d12/RootSignature.h"
#include "D3d12/Texture.h"
#include "D3d12/ASBuilder.h"
#include "Foundation/Camera.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct CubeConstantBuffer {
    glm::vec4 albedo;
};

struct SceneConstantBuffer {
    glm::mat4x4 projectToWorld;    // inverse(proj * view)
    glm::vec4 cameraPosition;
    glm::vec4 lightPosition;
    glm::vec4 lightAmbientColor;
    glm::vec4 lightDiffuseColor;
};

class SimpleLighting : public Renderer {
public:
    void OnCreate(uint32_t numBackBuffer, HWND hwnd) override;
    void OnDestroy() override;
    void OnUpdate(GameTimer &timer) override;
    void OnPreRender(GameTimer &timer) override;
    void OnRender(GameTimer &timer) override;
    void OnResize(uint32_t width, uint32_t height) override;
public:
    void SetupCamera();
    void BuildGeometry();
    void CreateRayTracingOutput();
    void CreateRootSignature();
    void CreateRayTracingPipeline();
    void BuildAccelerationStructure();
    void LoadCubeMap();
private:
    // clang-format off
	dx::Texture								_rayTracingOutput;
	dx::UAV									_rayTracingOutputHandle;

	dx::RootSignature						_globalRootSignature;
	dx::RootSignature						_closestLocalRootSignature;
    dx::WRL::ComPtr<ID3D12StateObject>		_pRayTracingPSO;

	std::shared_ptr<dx::StaticBuffer>		_pMeshBuffer;
	D3D12_VERTEX_BUFFER_VIEW				_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW					_indexBufferView  = {};

	dx::BottomLevelAS						_bottomLevelAs;
	dx::TopLevelAS							_topLevelAs;
	std::unique_ptr<dx::ASBuilder>			_pASBuilder;

    std::unique_ptr<Camera>                 _pCamera;
    SceneConstantBuffer                     _sceneConstantBuffer = {};
    // clang-format on
};
