#pragma once
#include <unordered_map>
#include <vector>
#include "D3d12/D3dStd.h"
#include "NRD.h"
#include "NRDDescs.h"
#include "D3d12/DescriptorHandle.h"

using NrdUserPoolD3D12 = std::array<dx::Texture *, static_cast<size_t>(nrd::ResourceType::MAX_NUM) - 2>;

class NrdIntegrationD3D12 {
public:
    NrdIntegrationD3D12(const char *persistentName = "") : _name(persistentName) {
    }

    bool OnCreate(const nrd::InstanceCreationDesc &instanceCreationDesc);
    void OnDestroy();

    bool SetCommonSettings(const nrd::CommonSettings &commonSettings);
    bool SetDenoiserSettings(nrd::Identifier denoiser, const void *denoiserSettings);

    void Denoise(const nrd::Identifier *denoisers,
        uint32_t denoisersNum,
        dx::ComputeContext *pComputeContext,
        const NrdUserPoolD3D12 &userPool);
    void Resize(size_t width, size_t height);
private:
    void CreatePipelines();
    void CreateSampler();
    void Dispatch(dx::ComputeContext *pComputeContext,
        const nrd::DispatchDesc &dispatchDesc,
        const NrdUserPoolD3D12 &userPool);
	auto GetTextureUAV(const dx::Texture *pTexture) -> D3D12_CPU_DESCRIPTOR_HANDLE;
    auto GetTextureSRV(const dx::Texture *pTexture) -> D3D12_CPU_DESCRIPTOR_HANDLE;
private:
    using TexturePool = std::vector<std::unique_ptr<dx::Texture>>;
    using RootSignaturePool = std::vector<std::unique_ptr<dx::RootSignature>>;
    using PipelineStatePool = std::vector<dx::WRL::ComPtr<ID3D12PipelineState>>;
    using TextureUAVMap = std::unordered_map<ID3D12Resource *, dx::UAV>;
    using TextureSRVMap = std::unordered_map<ID3D12Resource *, dx::SRV>;
private:
    // clang-format off
    std::string         _name;
    nrd::Instance      *_pInstance = nullptr;
    dx::SAMPLER         _samplers;
    TexturePool         _texturePool;
    RootSignaturePool   _rootSignatures;
    PipelineStatePool   _pipelines;
    TextureUAVMap       _textureUAVMap;
    TextureSRVMap       _textureSRVMap;
    // clang-format on
};
