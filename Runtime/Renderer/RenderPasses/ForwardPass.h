#pragma once
#include "RenderObject/ConstantBufferHelper.h"
#include "RenderPass.h"
#include "Utils/GlobalCallbacks.h"

class ForwardPass : public RenderPass {
public:
    struct DrawArgs {
	    dx::GraphicsContext             *pGfxCtx            = nullptr;
	    const cbuffer::CbPrePass        *pCbPrePass         = nullptr;
	    const cbuffer::CbLighting       *pCbLighting        = nullptr;
	    D3D12_GPU_VIRTUAL_ADDRESS        cbPrePassCBuffer   = 0;
	    D3D12_GPU_VIRTUAL_ADDRESS        cbLightBuffer      = 0;
    };
public:
    void OnCreate() override;
    void OnDestroy() override;
    void DrawBatch(const std::vector<RenderObject *> &batchList, const DrawArgs &globalShaderParam);
private:
    enum RootParam {
        ePreObject,
        eMaterial,
        eTextureList,
        eLighting,
        ePrePass,
    };
    void DrawBatchInternal(std::span<RenderObject *const> batch, const DrawArgs &globalShaderParam);
    auto GetPipelineState(RenderObject *pRenderObject) -> ID3D12PipelineState *;
    using PipelineStateMap = std::unordered_map<size_t, dx::WRL::ComPtr<ID3D12PipelineState>>;
private:
    // clang-format off
    PipelineStateMap                    _pipelineStateMap;
    std::unique_ptr<dx::RootSignature>  _pRootSignature;
    // clang-format on
};
