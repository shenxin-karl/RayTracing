#include "NRDIntegrationD3D12.h"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "D3d12/Texture.h"
#include "Renderer/GfxDevice.h"
#include "Foundation/Formatter.hpp"
#include "Renderer/RenderUtils/UserMarker.h"
#include "D3d12/Context.h"

static constexpr DXGI_FORMAT kNrdFormat2DXGIFormat[] = {
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R8_SINT,

    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_R8G8_SINT,

    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    DXGI_FORMAT_R8G8B8A8_UINT,
    DXGI_FORMAT_R8G8B8A8_SINT,

    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_SNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16_SINT,
    DXGI_FORMAT_R16_FLOAT,

    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_R16G16_SNORM,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_R16G16_SINT,
    DXGI_FORMAT_R16G16_FLOAT,

    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_SNORM,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R16G16B16A16_SINT,
    DXGI_FORMAT_R16G16B16A16_FLOAT,

    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R32_FLOAT,

    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R32G32_FLOAT,

    DXGI_FORMAT_R32G32B32A32_UINT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    DXGI_FORMAT_R32G32B32A32_FLOAT,

    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R10G10B10A2_UINT,
    DXGI_FORMAT_R11G11B10_FLOAT,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
};

bool NrdIntegrationD3D12::OnCreate(const nrd::InstanceCreationDesc &instanceCreationDesc) {
    if (nrd::CreateInstance(instanceCreationDesc, m_Instance) != nrd::Result::SUCCESS) {
        return false;
    }
    CreateSampler();
    CreatePipelines();
    return true;
}

void NrdIntegrationD3D12::OnDestroy() {
    _rootSignatures.clear();
    _pipelines.clear();
    _samplers.Release();
    _texturePool.clear();

    nrd::DestroyInstance(*m_Instance);
    m_Instance = nullptr;
    m_Name = nullptr;
    m_IsDescriptorCachingEnabled = false;
}

bool NrdIntegrationD3D12::SetCommonSettings(const nrd::CommonSettings &commonSettings) {
    Exception::CondThrow(m_Instance, "Uninitialized! Did you forget to call 'Initialize'?");
    nrd::Result result = nrd::SetCommonSettings(*m_Instance, commonSettings);
    Exception::CondThrow(result == nrd::Result::SUCCESS, "nrd::SetCommonSettings(): failed!");
    return result == nrd::Result::SUCCESS;
}

bool NrdIntegrationD3D12::SetDenoiserSettings(nrd::Identifier denoiser, const void *denoiserSettings) {
    Exception::CondThrow(m_Instance, "Uninitialized! Did you forget to call 'Initialize'?");
    nrd::Result result = nrd::SetDenoiserSettings(*m_Instance, denoiser, denoiserSettings);
    Exception::CondThrow(result == nrd::Result::SUCCESS, "nrd::SetDenoiserSettings(): failed!");
    return result == nrd::Result::SUCCESS;
}

void NrdIntegrationD3D12::Denoise(const nrd::Identifier *denoisers,
    uint32_t denoisersNum,
    dx::ComputeContext *pComputeContext,
    const NrdUserPoolD3D12 &userPool) {
    Exception::CondThrow(m_Instance, "Uninitialized! Did you forget to call 'Initialize'?");

    const nrd::DispatchDesc *dispatchDescs = nullptr;
    uint32_t dispatchDescsNum = 0;
    nrd::GetComputeDispatches(*m_Instance, denoisers, denoisersNum, dispatchDescs, dispatchDescsNum);

    for (uint32_t i = 0; i < dispatchDescsNum; i++) {
        const nrd::DispatchDesc &dispatchDesc = dispatchDescs[i];
        UserMarker userMarker(pComputeContext, dispatchDesc.name);
        Dispatch(pComputeContext, dispatchDesc, userPool);
    }
}

void NrdIntegrationD3D12::Resize(size_t width, size_t height) {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    const nrd::InstanceDesc &instanceDesc = nrd::GetInstanceDesc(*m_Instance);
    const uint32_t poolSize = instanceDesc.permanentPoolSize + instanceDesc.transientPoolSize;

    _texturePool.clear();
    _texturePool.resize(poolSize);

    // Texture pool
    for (uint32_t i = 0; i < poolSize; i++) {
        const nrd::TextureDesc &nrdTextureDesc = (i < instanceDesc.permanentPoolSize)
                                                     ? instanceDesc.permanentPool[i]
                                                     : instanceDesc.transientPool[i - instanceDesc.permanentPoolSize];
        const DXGI_FORMAT format = kNrdFormat2DXGIFormat[static_cast<size_t>(nrdTextureDesc.format)];

        uint16_t w = dx::DivideRoundingUp(width, nrdTextureDesc.downsampleFactor);
        uint16_t h = dx::DivideRoundingUp(height, nrdTextureDesc.downsampleFactor);

        D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, w, h, 1);
        textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        _texturePool[i] = std::make_unique<dx::Texture>();
        _texturePool[i]->OnCreate(pGfxDevice->GetDevice(), textureDesc, D3D12_RESOURCE_STATE_COMMON);

        std::string name;
        if (i < instanceDesc.permanentPoolSize) {
            name = fmt::format("{}::PermamentPool_{}", m_Name, i);
        } else {
            name = fmt::format("{}::TransientPool_{}", m_Name, i);
        }
        _texturePool[i]->SetName(name);
    }
}

void NrdIntegrationD3D12::CreatePipelines() {
    _pipelines.clear();
    _rootSignatures.clear();

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    const nrd::InstanceDesc &instanceDesc = nrd::GetInstanceDesc(*m_Instance);
    for (uint32_t i = 0; i < instanceDesc.pipelinesNum; i++) {
        std::unique_ptr<dx::RootSignature> pRootSignature = std::make_unique<dx::RootSignature>();
        const nrd::PipelineDesc &nrdPipelineDesc = instanceDesc.pipelines[i];
        size_t numRootParam = nrdPipelineDesc.hasConstantData ? 3 : 2;
        pRootSignature->OnCreate(numRootParam);
        size_t idx = 0;
        if (nrdPipelineDesc.hasConstantData) {
            pRootSignature->At(0).InitAsBufferCBV(instanceDesc.constantBufferRegisterIndex,
                instanceDesc.constantBufferSpaceIndex);
            ++idx;
        }
        pRootSignature->At(idx++).InitAsDescriptorTable({CD3DX12_DESCRIPTOR_RANGE1{
            D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
            instanceDesc.samplersNum,
            instanceDesc.samplersBaseRegisterIndex,
            instanceDesc.samplersBaseRegisterIndex,
        }});
        pRootSignature->At(idx).InitAsDescriptorTable(nrdPipelineDesc.resourceRangesNum);

        // Resources
        for (uint32_t j = 0; j < nrdPipelineDesc.resourceRangesNum; j++) {
            const nrd::ResourceRangeDesc &nrdResourceRange = nrdPipelineDesc.resourceRanges[j];
            D3D12_DESCRIPTOR_RANGE_TYPE type = nrdResourceRange.descriptorType == nrd::DescriptorType::TEXTURE
                                                   ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV
                                                   : D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            pRootSignature->At(idx).SetTableRange(j,
                type,
                nrdResourceRange.baseRegisterIndex,
                nrdResourceRange.descriptorsNum);
        }
        pRootSignature->Generate(pGfxDevice->GetDevice());

        const nrd::ComputeShaderDesc &nrdComputeShader = nrdPipelineDesc.computeShaderDXIL;

        struct PipelineDesc {
            CD3DX12_PIPELINE_STATE_STREAM_CS CS;
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
        };
        PipelineDesc pipelineDesc;
        pipelineDesc.RootSignature = pRootSignature->GetRootSignature();
        pipelineDesc.CS = D3D12_SHADER_BYTECODE(nrdComputeShader.bytecode, nrdComputeShader.size);

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
            sizeof(pipelineDesc),
            &pipelineDesc,
        };

        dx::WRL::ComPtr<ID3D12PipelineState> pPipelineState;
        dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
        dx::ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pPipelineState)));

        _rootSignatures.push_back(std::move(pRootSignature));
        _pipelines.push_back(std::move(pPipelineState));
    }
}

void NrdIntegrationD3D12::CreateSampler() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    const nrd::InstanceDesc &instanceDesc = nrd::GetInstanceDesc(*m_Instance);
    dx::SAMPLER sampler = pGfxDevice->GetDevice()->AllocDescriptor<dx::SAMPLER>(instanceDesc.samplersNum);
    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
    for (uint32_t i = 0; i < instanceDesc.samplersNum; i++) {
        nrd::Sampler nrdSampler = instanceDesc.samplers[i];
        D3D12_SAMPLER_DESC desc = {};
        desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        if (nrdSampler == nrd::Sampler::NEAREST_CLAMP) {
            desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        } else {
            desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        }

        device->CreateSampler(&desc, sampler.GetCpuHandle(i));
    }
}

void NrdIntegrationD3D12::Dispatch(dx::ComputeContext *pComputeContext,
    const nrd::DispatchDesc &dispatchDesc,
    const NrdUserPoolD3D12 &userPool) {

    const nrd::InstanceDesc &instanceDesc = nrd::GetInstanceDesc(*m_Instance);
    const nrd::PipelineDesc &pipelineDesc = instanceDesc.pipelines[dispatchDesc.pipelineIndex];

    pComputeContext->SetComputeRootSignature(_rootSignatures[dispatchDesc.pipelineIndex].get());
    pComputeContext->SetPipelineState(_pipelines[dispatchDesc.pipelineIndex].Get());

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;
    for (uint32_t i = 0; i < pipelineDesc.resourceRangesNum; i++) {
        const nrd::ResourceRangeDesc &resourceRange = pipelineDesc.resourceRanges[i];
        const bool isStorage = resourceRange.descriptorType == nrd::DescriptorType::STORAGE_TEXTURE;
        for (uint32_t j = 0; j < resourceRange.descriptorsNum; j++) {
            const nrd::ResourceDesc &nrdResource = dispatchDesc.resources[handles.size()];
            dx::Texture *pTexture = nullptr;
            if (nrdResource.type == nrd::ResourceType::TRANSIENT_POOL) {
                pTexture = _texturePool[nrdResource.indexInPool + instanceDesc.permanentPoolSize].get();
            } else if (nrdResource.type == nrd::ResourceType::PERMANENT_POOL) {
                pTexture = _texturePool[nrdResource.indexInPool].get();
            } else {
                pTexture = userPool[static_cast<uint32_t>(nrdResource.type)];
            }

            D3D12_RESOURCE_STATES state = !isStorage ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                                                     : D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            pComputeContext->Transition(pTexture->GetResource(), state);
            if (!isStorage) {
                // todo get SRV
                handles.push_back({});
            } else {
                // todo get UAV
                handles.push_back({});
            }
        }
    }

    if (pipelineDesc.hasConstantData) {
        D3D12_GPU_VIRTUAL_ADDRESS address = pComputeContext->AllocConstantBuffer(dispatchDesc.constantBufferDataSize,
            dispatchDesc.constantBufferData);
        pComputeContext->SetComputeRootConstantBufferView(0, address);
    }
    pComputeContext->SetDynamicSamples(1, _samplers.GetNumHandle(), _samplers);
    if (handles.size() > 0) {
        pComputeContext->SetDynamicViews(2, handles);
    }
    pComputeContext->Dispatch(dispatchDesc.gridWidth, dispatchDesc.gridHeight, 1);
}
