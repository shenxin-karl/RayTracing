#include "GBufferPass.h"
#include "D3d12/BindlessCollection.hpp"
#include "D3d12/Context.h"
#include "D3d12/D3dStd.h"
#include "D3d12/Device.h"
#include "Foundation/ColorUtil.hpp"
#include "Renderer/GfxDevice.h"
#include "Renderer/RenderUtils/RenderSetting.h"
#include "Renderer/RenderUtils/UserMarker.h"
#include "Renderer/RenderUtils/ConstantBufferHelper.h"
#include "RenderObject/GPUMeshData.h"
#include "RenderObject/Material.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/RenderGroup.hpp"
#include "RenderObject/RenderObject.h"
#include "RenderObject/VertexSemantic.hpp"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"

GBufferPass::GBufferPass() : _width(0), _height(0) {
}

void GBufferPass::OnCreate() {
    GfxDevice *pDevice = GfxDevice::GetInstance();
    _gBufferSRV = pDevice->GetDevice()->AllocDescriptor<dx::SRV>(5);
    _gBufferRTV = pDevice->GetDevice()->AllocDescriptor<dx::RTV>(5);

    _pRootSignature = std::make_unique<dx::RootSignature>();
    _pRootSignature->OnCreate(eMaxNumRootParam, 6);
    _pRootSignature->At(eCbPrePass).InitAsBufferCBV(0);
    _pRootSignature->At(eCbPreObject).InitAsBufferCBV(1);
    _pRootSignature->At(eCbMaterial).InitAsBufferCBV(2);

    // eTextureList enable bindless
    CD3DX12_DESCRIPTOR_RANGE1 range = {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        static_cast<UINT>(-1),
        0,
        0,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
    };
    _pRootSignature->At(eTextureList).InitAsDescriptorTable({range});
    _pRootSignature->SetStaticSamplers(dx::GetStaticSamplerArray());
    _pRootSignature->Generate(GfxDevice::GetInstance()->GetDevice());
    _pRootSignature->SetName("GBufferPass::RootSignature");
}

void GBufferPass::OnDestroy() {
    _gBufferTextures.clear();
    _gBufferRTV.Release();
    _gBufferSRV.Release();
    _pRootSignature->OnDestroy();
    _pipelineStateMap.clear();
}

void GBufferPass::OnResize(const ResolutionInfo &resolutio) {
    _width = resolutio.renderWidth;
    _height = resolutio.renderHeight;

    GfxDevice *pDevice = GfxDevice::GetInstance();

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Color[0] = 0.f;
    clearValue.Color[1] = 0.f;
    clearValue.Color[2] = 0.f;
    clearValue.Color[3] = 1.f;

    DXGI_FORMAT gBufferFormats[] = {
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R11G11B10_FLOAT,
        DXGI_FORMAT_R16G16_FLOAT,
        DXGI_FORMAT_R16_FLOAT,
    };

    _gBufferTextures.clear();
    _gBufferTextures.resize(5);

    for (size_t i = 0; i < 5; ++i) {
	    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(gBufferFormats[i], _width, _height);
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		clearValue.Format = desc.Format;
        _gBufferTextures[i] = std::make_unique<dx::Texture>();
        _gBufferTextures[i]->OnCreate(pDevice->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON, &clearValue);
		_gBufferTextures[i]->SetName(fmt::format("GBufferPass::GBuffer{}", i));
        dx::NativeDevice *device = pDevice->GetDevice()->GetNativeDevice();
        device->CreateRenderTargetView(_gBufferTextures[i]->GetResource(), nullptr, _gBufferRTV[i]);
        device->CreateShaderResourceView(_gBufferTextures[i]->GetResource(), nullptr, _gBufferSRV[i]);
    }
}

auto GBufferPass::GetGBufferSRV(size_t index) const -> D3D12_CPU_DESCRIPTOR_HANDLE {
    return _gBufferSRV[index];
}

auto GBufferPass::GetGBufferTexture(size_t index) const -> dx::Texture * {
    return _gBufferTextures[index].get();
}

void GBufferPass::PreDraw(const DrawArgs &args) {
	UserMarker marker(args.pGfxCtx, "GBufferPreDrawPass");
    for (size_t i = 0; i < _gBufferTextures.size(); ++i) {
	    args.pGfxCtx->Transition(_gBufferTextures[i]->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        args.pGfxCtx->ClearRenderTargetView(_gBufferRTV[i], Colors::Black);
    }

    args.pGfxCtx->Transition(args.pDepthBufferResource, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    args.pGfxCtx->ClearDepthStencilView(args.depthBufferDSV,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        RenderSetting::Get().GetDepthClearValue(),
        0);

    args.pGfxCtx->SetRenderTargets(_gBufferRTV.GetCpuHandle(), 5, args.depthBufferDSV);
}

void GBufferPass::DrawBatch(const std::vector<RenderObject *> &batchList, const DrawArgs &args) {
    if (batchList.empty()) {
	    return;
    }

	UserMarker marker(args.pGfxCtx, "GBufferDrawBatchPass");
    DrawBatchList(batchList, [&](std::span<RenderObject *const> batch) { DrawBatchInternal(batch, args); });
}

void GBufferPass::PostDraw(const DrawArgs &args) {
	UserMarker marker(args.pGfxCtx, "GBufferPostDrawPass");
    for (TexturePtr &texture : _gBufferTextures) {
		args.pGfxCtx->Transition(texture->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    args.pGfxCtx->Transition(args.pDepthBufferResource, D3D12_RESOURCE_STATE_DEPTH_READ);
    
}

void GBufferPass::DrawBatchInternal(std::span<RenderObject *const> batch, const DrawArgs &args) {
   dx::GraphicsContext *pGfxCtx = args.pGfxCtx;

    // bind pipeline state object
    ID3D12PipelineState *pPipelineState = GetPipelineState(batch.front());
    pGfxCtx->SetGraphicsRootSignature(_pRootSignature.get());
    pGfxCtx->SetPipelineState(pPipelineState);
    pGfxCtx->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pGfxCtx->SetGraphicsRootConstantBufferView(eCbPrePass, args.cbPrePassCBuffer);
    using TextureType = Material::TextureType;

    size_t index = 0;
    while (index < batch.size()) {
        dx::BindlessCollection bindlessCollection;
        size_t batchIdx = index;
        while (batchIdx < batch.size() && bindlessCollection.EnsureCapacity(TextureType::eMaxNum)) {
            const Material *pMaterial = batch[batchIdx]->pMaterial;
            for (const dx::SRV &srv : pMaterial->_textureHandles) {
                bindlessCollection.AddHandle(srv.GetCpuHandle());
            }
            ++batchIdx;
        }

        // texture list bindless
        pGfxCtx->SetDynamicViews(eTextureList, bindlessCollection.GetHandles());
        for (size_t i = index; i != batchIdx; ++i) {
            pGfxCtx->SetGraphicsRootDynamicConstantBuffer(eCbPreObject, batch[i]->cbPreObject);

            const Material *pMaterial = batch[i]->pMaterial;
            Material::CbPreMaterial cbMaterial = pMaterial->_cbPreMaterial;
            cbMaterial.albedoTexIndex = bindlessCollection.GetHandleIndex(
                pMaterial->_textureHandles[TextureType::eAlbedoTex].GetCpuHandle());
            cbMaterial.ambientOcclusionTexIndex = bindlessCollection.GetHandleIndex(
                pMaterial->_textureHandles[TextureType::eAmbientOcclusionTex].GetCpuHandle());
            cbMaterial.emissionTexIndex = bindlessCollection.GetHandleIndex(
                pMaterial->_textureHandles[TextureType::eEmissionTex].GetCpuHandle());
            cbMaterial.metalRoughnessTexIndex = bindlessCollection.GetHandleIndex(
                pMaterial->_textureHandles[TextureType::eMetalRoughnessTex].GetCpuHandle());
            cbMaterial.normalTexIndex = bindlessCollection.GetHandleIndex(
                pMaterial->_textureHandles[TextureType::eNormalTex].GetCpuHandle());
            pGfxCtx->SetGraphicsRootDynamicConstantBuffer(eCbMaterial, cbMaterial);

            const Mesh *pMesh = batch[i]->pMesh;
            const GPUMeshData *pGpuMeshData = pMesh->GetGPUMeshData();
            pGfxCtx->SetVertexBuffers(0, pGpuMeshData->GetVertexBufferView());
            if (pMesh->GetIndexCount() > 0) {
                pGfxCtx->SetIndexBuffer(pGpuMeshData->GetIndexBufferView());
            }

            for (const SubMesh &subMesh : pMesh->GetSubMeshes()) {
                if (subMesh.indexCount > 0) {
                    pGfxCtx->DrawIndexedInstanced(subMesh.indexCount,
                        1,
                        subMesh.baseIndexLocation,
                        subMesh.baseVertexLocation,
                        0);
                } else {
                    pGfxCtx->DrawInstanced(subMesh.vertexCount, 1, subMesh.baseVertexLocation, 0);
                }
            }
        }
        index = batchIdx;
    }
}

auto GBufferPass::GetPipelineState(RenderObject *pRenderObject) -> ID3D12PipelineState * {
    const Material *pMaterial = pRenderObject->pMaterial;
    auto iter = _pipelineStateMap.find(pMaterial->GetPipelineID());
    if (iter != _pipelineStateMap.end()) {
        return iter->second.Get();
    }

    dx::DefineList defineList = pMaterial->_defineList.Clone();
    defineList.Set("GENERATE_MOTION_VECTOR");

    ShaderLoadInfo shaderLoadInfo;
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/Material.hlsl");
    shaderLoadInfo.entryPoint = "VSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::eVS;
    shaderLoadInfo.pDefineList = &defineList;
    D3D12_SHADER_BYTECODE vsByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);
    Assert(vsByteCode.pShaderBytecode != nullptr);

    shaderLoadInfo.entryPoint = "GBufferPSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::ePS;
    D3D12_SHADER_BYTECODE psByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);

    struct PipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    };

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();

    PipelineStateStream pipelineDesc = {};
    pipelineDesc.pRootSignature = _pRootSignature->GetRootSignature();
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.VS = vsByteCode;
    pipelineDesc.PS = psByteCode;
    pipelineDesc.DSVFormat = pGfxDevice->GetDepthStencilFormat();

    CD3DX12_DEPTH_STENCIL_DESC depthStencil(D3D12_DEFAULT);
    depthStencil.DepthFunc = RenderSetting::Get().GetDepthFunc();
    pipelineDesc.DepthStencil = depthStencil;

    SemanticMask meshSemanticMask = pRenderObject->pMesh->GetSemanticMask();
    SemanticMask pipelineSemanticMask = pMaterial->_pipelineSemanticMask;
    auto inputLayouts = SemanticMaskToVertexInputElements(meshSemanticMask, pipelineSemanticMask);
    pipelineDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC{
        inputLayouts.data(),
        static_cast<UINT>(inputLayouts.size()),
    };

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = _gBufferTextures.size();
    for (size_t i = 0; i < _gBufferTextures.size(); ++i) {
	    rtvFormats.RTFormats[i] = _gBufferTextures[i]->GetFormat();
    }
    pipelineDesc.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream),
        &pipelineDesc,
    };

    dx::WRL::ComPtr<ID3D12PipelineState> pPipelineState;
    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
    dx::ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pPipelineState)));

    _pipelineStateMap[pMaterial->GetPipelineID()] = pPipelineState;
    return pPipelineState.Get();
}
