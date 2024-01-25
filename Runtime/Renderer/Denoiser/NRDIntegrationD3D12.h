#pragma once
#include <vector>
#include "D3d12/D3dStd.h"
#include "NRD.h"
#include "NRDDescs.h"
#include "D3d12/DescriptorHandle.h"

using NrdUserPoolD3D12 = std::array<dx::Texture *, static_cast<size_t>(nrd::ResourceType::MAX_NUM) - 2>;

class NrdIntegrationD3D12 {
public:
    NrdIntegrationD3D12(uint32_t bufferedFramesNum, bool enableDescriptorCaching, const char *persistentName = "")
        : m_Name(persistentName),
          m_IsDescriptorCachingEnabled(enableDescriptorCaching) {
    }

    bool OnCreate(const nrd::InstanceCreationDesc &instanceCreationDesc);
    void OnDestroy();

    bool SetCommonSettings(const nrd::CommonSettings& commonSettings);
    bool SetDenoiserSettings(nrd::Identifier denoiser, const void* denoiserSettings);

    void Denoise(const nrd::Identifier *denoisers,
        uint32_t denoisersNum,
        dx::ComputeContext *pComputeContext,
        const NrdUserPoolD3D12 &userPool);
    void Resize(size_t width, size_t height);
private:
    void CreatePipelines();
    void CreateSampler();
    void Dispatch(dx::ComputeContext *pComputeContext, const nrd::DispatchDesc& dispatchDesc, const NrdUserPoolD3D12& userPool);
private:
    std::vector<std::unique_ptr<dx::Texture>> _texturePool;
    std::vector<std::unique_ptr<dx::RootSignature>> _rootSignatures;
    std::vector<dx::WRL::ComPtr<ID3D12PipelineState>> _pipelines;
    dx::SAMPLER _samplers;
    nrd::Instance *m_Instance = nullptr;
    const char *m_Name = nullptr;
    bool m_IsDescriptorCachingEnabled = false;
};

#define NRD_INTEGRATION_ABORT_ON_FAILURE(result)                                                                       \
    if ((result) != nri::Result::SUCCESS)                                                                              \
    NRD_INTEGRATION_ASSERT(false, "Abort on failure!")
