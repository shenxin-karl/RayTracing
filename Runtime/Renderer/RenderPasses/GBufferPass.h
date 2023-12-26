#pragma once
#include "D3d12/DescriptorHandle.h"
#include "D3d12/Texture.h"
#include "Foundation/NonCopyable.h"

struct RenderObject;
namespace cbuffer {
struct CbLighting;
struct CbPrePass;
}    // namespace cbuffer

class GBufferPass : NonCopyable {
public:
    GBufferPass();
    void OnCreate();
    void OnDestroy();
    void OnResize(size_t width, size_t height);
    auto GetGBufferSRV(size_t index) const -> D3D12_CPU_DESCRIPTOR_HANDLE;

    struct DrawArgs {
        dx::GraphicsContext *pGfxCtx = nullptr;
        const cbuffer::CbPrePass *pCbPrePass = nullptr;
        const cbuffer::CbLighting *pCbLighting = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS cbPrePassCBuffer = 0;
        D3D12_GPU_VIRTUAL_ADDRESS cbLightBuffer = 0;
        ID3D12Resource             *pDepthBufferResource = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE depthBufferDSV = {};
    };

    void PreDraw(const DrawArgs &args);
    void DrawBatch(const std::vector<RenderObject *> &batchList, const DrawArgs &args);
    void PostDraw(const DrawArgs &args);
private:
    dx::Texture _gBuffer0;    // R8G8B8A8,      float3 albedo,	float ao
    dx::Texture _gBuffer1;    // R16G16B16A16   float2 normal,	float metallic, float roughness
    dx::Texture _gBuffer2;    // R11G11B10      float3 emission,
    dx::RTV     _gBufferRTV;   
    dx::SRV     _gBufferSRV;
    size_t      _width;
    size_t      _height;
};