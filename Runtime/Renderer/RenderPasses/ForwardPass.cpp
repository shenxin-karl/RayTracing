#include "ForwardPass.h"
#include "D3d12/Device.h"
#include "Foundation/Logger.h"
#include "Renderer/GfxDevice.h"
#include "RenderObject/GPUMeshData.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/RenderGroup.hpp"
#include "RenderObject/RenderObject.h"
#include "RenderObject/VertexSemantic.hpp"
#include "RenderObject/Material.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "D3d12/BindlessCollection.hpp"
#include "Renderer/RenderUtils/RenderSetting.h"
#include "Renderer/RenderUtils/UserMarker.h"

void ForwardPass::OnCreate() {
    _pRootSignature = dx::RootSignature::Create(5, 6);
    _pRootSignature->At(ePrePass).InitAsBufferCBV(0);    // gCbPrePass;
    _pRootSignature->At(ePreObject).InitAsBufferCBV(1);    // gCbPreObject;
    _pRootSignature->At(eLighting).InitAsBufferCBV(3);    // gCbLighting;
    _pRootSignature->At(eMaterial).InitAsBufferCBV(2);    // gCbMaterial;

    CD3DX12_DESCRIPTOR_RANGE1 range = {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        static_cast<UINT>(-1),
        0,
        0,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
    };
    _pRootSignature->At(eTextureList).InitAsDescriptorTable({range});    // gTextureList
    _pRootSignature->SetStaticSamplers(dx::GetStaticSamplerArray());
    _pRootSignature->Generate(GfxDevice::GetInstance()->GetDevice());
    _pRootSignature->SetName("ForwardPass:RootSignature");
}

void ForwardPass::OnDestroy() {
    _pRootSignature = nullptr;
}

void ForwardPass::DrawBatch(const std::vector<RenderObject *> &batchList, const DrawArgs &drawArgs) {
    if (batchList.empty()) {
	    return;
    }
    UserMarker userMarker(drawArgs.pGfxCtx, "ForwardPass::DrawBatch");
    DrawBatchList(batchList, [&](std::span<RenderObject *const> batch) { DrawBatchInternal(batch, drawArgs); });
}

void ForwardPass::DrawBatchInternal(std::span<RenderObject *const> batch, const DrawArgs &drawArgs) {
    dx::GraphicsContext *pGfxCtx = drawArgs.pGfxCtx;

    // bind pipeline state object
    ID3D12PipelineState *pPipelineState = GetPipelineState(batch.front());
    pGfxCtx->SetGraphicsRootSignature(_pRootSignature.Get());
    pGfxCtx->SetPipelineState(pPipelineState);
    pGfxCtx->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pGfxCtx->SetGraphicsRootConstantBufferView(ePrePass, drawArgs.cbPrePassCBuffer);
    pGfxCtx->SetGraphicsRootConstantBufferView(eLighting, drawArgs.cbLightBuffer);

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
            pGfxCtx->SetGraphicsRootDynamicConstantBuffer(ePreObject, batch[index]->cbPreObject);

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
            pGfxCtx->SetGraphicsRootDynamicConstantBuffer(eMaterial, cbMaterial);

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

auto ForwardPass::GetPipelineState(RenderObject *pRenderObject) -> ID3D12PipelineState * {
    const Material *pMaterial = pRenderObject->pMaterial;
    auto iter = _pipelineStateMap.find(pMaterial->GetPipelineID());
    if (iter != _pipelineStateMap.end()) {
        return iter->second.Get();
    }

    ShaderLoadInfo shaderLoadInfo;
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/Material.hlsl");
    shaderLoadInfo.entryPoint = "VSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::eVS;
    shaderLoadInfo.pDefineList = &pMaterial->_defineList;
    D3D12_SHADER_BYTECODE vsByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);
    Assert(vsByteCode.pShaderBytecode != nullptr);

    shaderLoadInfo.entryPoint = "ForwardPSMain";
    shaderLoadInfo.shaderType = dx::ShaderType::ePS;
    D3D12_SHADER_BYTECODE psByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);

    struct PipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendDesc;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
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

    if (RenderGroup::IsTransparent(pMaterial->_renderGroup)) {
        CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
        D3D12_RENDER_TARGET_BLEND_DESC rt0BlendDesc = {};
        rt0BlendDesc.BlendEnable = true;
        rt0BlendDesc.LogicOpEnable = false;
        rt0BlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt0BlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        rt0BlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
        rt0BlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt0BlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
        rt0BlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0] = rt0BlendDesc;
        pipelineDesc.BlendDesc = blendDesc;
    }

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.RTFormats[0] = pGfxDevice->GetRenderTargetFormat();
    rtvFormats.NumRenderTargets = 1;
    pipelineDesc.RTVFormats = rtvFormats;

    CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
    if (RenderGroup::IsAlphaTest(pMaterial->GetRenderGroup())) {
	    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    }
    pipelineDesc.Rasterizer = rasterizer;

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
