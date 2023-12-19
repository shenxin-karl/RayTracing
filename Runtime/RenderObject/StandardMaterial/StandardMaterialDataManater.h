#pragma once
#include <d3d12.h>
#include "Foundation/NonCopyable.h"
#include "RenderObject/VertexSemantic.hpp"
#include "Utils/GlobalCallbacks.h"
#include "D3d12/D3dUtils.h"
#include "D3d12/DescriptorHandle.h"

class StandardMaterial;
class StandardMaterialDataManager : private NonCopyable {
public:
    enum RootParam {
	    ePrePass    = 0,
        ePreObject  = 1,
        eLighting   = 3,
        eMaterial   = 2,
        eTextureList = 4,
    };

    struct alignas(sizeof(glm::vec4)) CbPreMaterial {
        glm::vec4 albedo;
        glm::vec4 emission;
        glm::vec4 tilingAndOffset;
        float     cutoff;
        float     roughness;
        float     metallic;
        float     normalScale;
        int       samplerStateIndex;
        int       albedoTexIndex;
        int       ambientOcclusionTexIndex;
        int       emissionTexIndex;
        int       metalRoughnessTexIndex;
        int       normalTexIndex;
    };

    StandardMaterialDataManager();
    void OnCreate();
    void OnDestroy();
    auto GetPipelineState(StandardMaterial *pMaterial, SemanticMask meshSemanticMask, SemanticMask pipelineSemanticMask)
        -> ID3D12PipelineState *;
    auto GetPipelineID(ID3D12PipelineState *pPipelineStateObject) -> uint16_t;
    auto GetTextureSRV(dx::Texture *pTexture) -> dx::SRV;

    auto GetRootSignature() const -> dx::RootSignature * {
        return _pRootSignature.get();
    }
    auto GetMaterialID() const -> uint16_t {
        return _materialID;
    }

    static auto Get() -> StandardMaterialDataManager &;
private:
    using PipelineStateMap = std::unordered_map<size_t, dx::WRL::ComPtr<ID3D12PipelineState>>;
    using PipelineIDMap = std::unordered_map<ID3D12PipelineState *, uint16_t>;
    using TextureSRVMap = std::unordered_map<dx::Texture *, dx::SRV>;

    // clang-format off
    uint16_t                            _materialID;
    CallbackHandle                      _createCallbackHandle;
    CallbackHandle                      _destroyCallbackHandle;
    PipelineStateMap                    _pipelineStateMap;
    PipelineIDMap                       _pipelineIdMap;

    TextureSRVMap                       _textureSRVMap;
    std::shared_ptr<dx::RootSignature>  _pRootSignature;
    // clang-format on
};