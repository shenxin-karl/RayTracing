#pragma once
#include "Renderer.h"
#include "D3d12/AccelerationStructure.h"
#include "D3d12/DescriptorHandle.h"
#include "D3d12/RootSignature.h"
#include "D3d12/Texture.h"
#include "D3d12/ASBuilder.h"
#include "Foundation/Memory/SharedPtr.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct CubeConstantBuffer {
    glm::vec4 albedo;
    float noiseTile;
};

struct SceneConstantBuffer {
    glm::mat4x4 projectToWorld;    // inverse(proj * view)
    glm::vec4 cameraPosition;
    glm::vec4 lightPosition;
    glm::vec4 lightAmbientColor;
    glm::vec4 lightDiffuseColor;
    float time;
};

class Scene;
class GameObject;

class SimpleLighting : public Renderer {
public:
    SimpleLighting();
    ~SimpleLighting() override;
    void OnCreate() override;
    void OnDestroy() override;
    void OnUpdate(GameTimer &timer) override;
    void OnPreRender(GameTimer &timer) override;
    void OnRender(GameTimer &timer) override;
    void OnResize(uint32_t width, uint32_t height) override;
public:
    void BuildGeometry();
    void CreateRayTracingOutput();
    void CreateRootSignature();
    void CreateRayTracingPipeline();
    void BuildAccelerationStructure();
    void LoadCubeMap();
    void InitScene();
private:
    // clang-format off
	SharedPtr<dx::Texture>                  _rayTracingOutput;
	dx::UAV									_rayTracingOutputHandle;

    SharedPtr<dx::Texture>                  _pCubeMap;
    dx::SRV                                 _cubeMapHandle;

	SharedPtr<dx::RootSignature>			_pGlobalRootSignature;
	SharedPtr<dx::RootSignature>			_pClosestLocalRootSignature;
    dx::WRL::ComPtr<ID3D12StateObject>		_pRayTracingPSO;

	SharedPtr<dx::Buffer>       		    _pMeshBuffer;
	D3D12_VERTEX_BUFFER_VIEW				_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW					_indexBufferView  = {};

	SharedPtr<dx::BottomLevelAS>  	        _pBottomLevelAs;
	SharedPtr<dx::TopLevelAS>		        _pTopLevelAs;

    SceneConstantBuffer                     _sceneConstantBuffer = {};
    Scene                                  *_pScene = nullptr;
    SharedPtr<GameObject>                   _pGameObject = nullptr;
    // clang-format on
};
