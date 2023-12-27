#pragma once
#include <unordered_map>
#include "D3d12/DescriptorHandle.h"
#include "D3d12/Texture.h"
#include "RenderPass.h"
#include "Utils/GlobalCallbacks.h"

struct RenderObject;
namespace cbuffer {
struct CbLighting;
struct CbPrePass;
}    // namespace cbuffer

class GBufferPass : public RenderPass {
public:
    GBufferPass();
    void OnCreate() override;
    void OnDestroy() override;
    void OnResize(size_t width, size_t height);
    auto GetGBufferSRV(size_t index) const -> D3D12_CPU_DESCRIPTOR_HANDLE;

    // clang-format off
    struct DrawArgs {
        dx::GraphicsContext         *pGfxCtx                = nullptr;
        const cbuffer::CbPrePass    *pCbPrePass             = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS    cbPrePassCBuffer       = 0;
        ID3D12Resource              *pDepthBufferResource   = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE  depthBufferDSV         = {};
    };
    // clang-format on

    enum RootParam {
        eCbPreObject,
        eCbMaterial,
        eTextureList,
        eCbPrePass,
        eMaxNumRootParam
    };

    void PreDraw(const DrawArgs &args);
    void DrawBatch(const std::vector<RenderObject *> &batchList, const DrawArgs &args);
    void PostDraw(const DrawArgs &args);
private:
    void DrawBatchInternal(std::span<RenderObject *const> batch, const DrawArgs &args);
    auto GetPipelineState(RenderObject *pRenderObject) -> ID3D12PipelineState *;
    using PipelineStateMap = std::unordered_map<size_t, dx::WRL::ComPtr<ID3D12PipelineState>>;
private:
    // clang-format off
    dx::Texture                         _gBuffer0;    // R8G8B8A8,      float3 albedo,	float ao
    dx::Texture                         _gBuffer1;    // R16G16B16A16   float2 normal,	float metallic, float roughness
    dx::Texture                         _gBuffer2;    // R11G11B10      float3 emission,
    dx::RTV                             _gBufferRTV;   
    dx::SRV                             _gBufferSRV;
    size_t                              _width;
    size_t                              _height;
    PipelineStateMap                    _pipelineStateMap;
    std::unique_ptr<dx::RootSignature>  _pRootSignature;
    CallbackHandle                      _recreatePipelineStateCallbackHandle;
    // clang-format on
};