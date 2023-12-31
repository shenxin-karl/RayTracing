#include "RayTracingShadowPass.h"
#include "D3d12/BindlessCollection.hpp"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "D3d12/ShaderCompiler.h"
#include "D3d12/Texture.h"
#include "Foundation/GameTimer.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/RenderSetting.h"
#include "Renderer/RenderUtils/UserMarker.h"
#include "RenderObject/GPUMeshData.h"
#include "RenderObject/Material.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/RenderGroup.hpp"
#include "RenderObject/VertexSemantic.hpp"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Renderer/RenderUtils/ImguiHelper.h"

static const wchar_t *sShadowRayGenShaderName = L"ShadowRaygenShader";

static const wchar_t *sOpaqueHitGroupName = L"ShadowOpaqueHitGroup1";
static const wchar_t *sOpaqueAnyHitShaderName = L"ShadowOpaqueAnyHitShader";

static const wchar_t *sAlphaTestHitGroupName = L"ShadowAlphaTestHitGroup2";
static const wchar_t *sAlphaTestAnyHitShaderName = L"ShadowAlphaTestAnyHitShader";
static const wchar_t *sShadowMissShaderName = L"ShadowMissShader";

void RayTracingShadowPass::OnCreate() {
    RenderPass::OnCreate();
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    _pShadowMaskTex = std::make_unique<dx::Texture>();
    _pGlobalRootSignature = std::make_unique<dx::RootSignature>();
    _pGlobalRootSignature->OnCreate(3, 6);
    _pGlobalRootSignature->At(eScene).InitAsBufferSRV(0);
    _pGlobalRootSignature->At(eRayGenCb).InitAsBufferCBV(0);
    _pGlobalRootSignature->At(eTable0).InitAsDescriptorTable({
        CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1),
        CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0),
    });
    _pGlobalRootSignature->SetStaticSamplers(dx::GetStaticSamplerArray());
    _pGlobalRootSignature->Generate(pGfxDevice->GetDevice());

    _pAlphaTestLocalRootSignature = std::make_unique<dx::RootSignature>();
    _pAlphaTestLocalRootSignature->OnCreate(5);
    _pAlphaTestLocalRootSignature->At(eMaterialIndex).InitAsConstants(1, 0, 1);
    _pAlphaTestLocalRootSignature->At(eVertexBuffer).InitAsBufferSRV(0, 1);
    _pAlphaTestLocalRootSignature->At(eIndexBuffer).InitAsBufferSRV(1, 1);
    _pAlphaTestLocalRootSignature->At(eInstanceMaterial).InitAsBufferSRV(2, 1);

    CD3DX12_DESCRIPTOR_RANGE1 range = {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        static_cast<UINT>(-1),
        3,
        1,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
    };
    _pAlphaTestLocalRootSignature->At(eAlbedoTextureList).InitAsDescriptorTable({range});
    _pAlphaTestLocalRootSignature->Generate(pGfxDevice->GetDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

    ShaderLoadInfo shaderLoadInfo = {};
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/RayTracingShadow.hlsl");
    shaderLoadInfo.shaderType = dx::ShaderType::eLib;

    D3D12_SHADER_BYTECODE byteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);
    Assert(byteCode.pShaderBytecode != nullptr);

    CD3DX12_STATE_OBJECT_DESC rayTracingPipeline{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};
    CD3DX12_DXIL_LIBRARY_SUBOBJECT *pLib = rayTracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    pLib->SetDXILLibrary(&byteCode);
    pLib->DefineExport(sShadowRayGenShaderName);
    pLib->DefineExport(sOpaqueAnyHitShaderName);
    pLib->DefineExport(sAlphaTestAnyHitShaderName);
    pLib->DefineExport(sShadowMissShaderName);

    CD3DX12_HIT_GROUP_SUBOBJECT *pOpaqueHitGroup = rayTracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    pOpaqueHitGroup->SetHitGroupExport(sOpaqueHitGroupName);
    pOpaqueHitGroup->SetAnyHitShaderImport(sOpaqueAnyHitShaderName);
    pOpaqueHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    CD3DX12_HIT_GROUP_SUBOBJECT *pAlphaTestHitGroup = rayTracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    pAlphaTestHitGroup->SetHitGroupExport(sAlphaTestHitGroupName);
    pAlphaTestHitGroup->SetAnyHitShaderImport(sAlphaTestAnyHitShaderName);
    pAlphaTestHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    auto *pShaderConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    size_t payloadSize = sizeof(float);    // float
    size_t attributeSize = 2 * sizeof(float);
    pShaderConfig->Config(payloadSize, attributeSize);

    CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT
    *pLocalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    pLocalRootSignature->SetRootSignature(_pAlphaTestLocalRootSignature->GetRootSignature());
    CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT *rootSignatureAssociation = rayTracingPipeline.CreateSubobject<
        CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    rootSignatureAssociation->SetSubobjectToAssociate(*pLocalRootSignature);
    rootSignatureAssociation->AddExport(sAlphaTestHitGroupName);

    auto *pGlobalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    pGlobalRootSignature->SetRootSignature(_pGlobalRootSignature->GetRootSignature());

    auto *pPipelineConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pPipelineConfig->Config(1);

    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
#if ENABLE_RAY_TRACING
    dx::ThrowIfFailed(device->CreateStateObject(rayTracingPipeline, IID_PPV_ARGS(&_pRayTracingPSO)));
#endif

    _buildRenderSettingUiHandle = GlobalCallbacks::Get().OnBuildRenderSettingGUI.Register(this,
        &RayTracingShadowPass::BuildRenderSettingUI);
}

void RayTracingShadowPass::OnDestroy() {
    RenderPass::OnDestroy();
    _pShadowMaskTex = nullptr;
    _pGlobalRootSignature = nullptr;
    _pAlphaTestLocalRootSignature = nullptr;
    _pRayTracingPSO = nullptr;
}

void RayTracingShadowPass::GenerateShadowMap(const DrawArgs &args) {
    dx::ComputeContext *pComputeContext = args.pComputeContext;
    UserMarker userMarker(pComputeContext, "RayTracingShadowPass");

    pComputeContext->SetComputeRootSignature(_pGlobalRootSignature.get());
    pComputeContext->SetRayTracingPipelineState(_pRayTracingPSO.Get());
    pComputeContext->SetComputeRootShaderResourceView(eScene, args.sceneTopLevelAS);

    struct RayGenCB {
        glm::mat4x4 matInvViewProj;
        glm::vec3 lightDirection;
        float maxCosineTheta;
        uint enableSoftShadow;
        uint frameCount;
        float maxT;
        float minT;
    };


    float cosineTheta = RenderSetting::Get().GetShadowRayMaxCosineTheta();
    RayGenCB rayGenCb;
    rayGenCb.matInvViewProj = args.matInvViewProj;
    rayGenCb.lightDirection = args.lightDirection;
    rayGenCb.maxCosineTheta = std::cos(glm::radians(cosineTheta));
    rayGenCb.enableSoftShadow = cosineTheta != 0.f;
    rayGenCb.frameCount = GameTimer::Get().GetFrameCount();
    rayGenCb.maxT = RenderSetting::Get().GetShadowRayTMax();
    rayGenCb.minT = RenderSetting::Get().GetShadowRayTMin();
    pComputeContext->SetComputeRootDynamicConstantBuffer(eRayGenCb, rayGenCb);

    D3D12_CPU_DESCRIPTOR_HANDLE table0[2];
    table0[eDepthTex] = args.depthTexSRV;
    table0[eOutputTex] = _shadowMaskUAV.GetCpuHandle();
    pComputeContext->SetDynamicViews(eTable0, table0);

    pComputeContext->Transition(_pShadowMaskTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    dx::DispatchRaysDesc dispatchRaysDesc = {};
    BuildShaderRecode(args.geometries, pComputeContext, dispatchRaysDesc);
    dispatchRaysDesc.width = _pShadowMaskTex->GetWidth();
    dispatchRaysDesc.height = _pShadowMaskTex->GetHeight();
    dispatchRaysDesc.depth = 1;
    pComputeContext->DispatchRays(dispatchRaysDesc);
    pComputeContext->Transition(_pShadowMaskTex->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

auto RayTracingShadowPass::GetShadowMaskSRV() const -> D3D12_CPU_DESCRIPTOR_HANDLE {
    return _shadowMaskSRV.GetCpuHandle();
}

void RayTracingShadowPass::OnResize(size_t width, size_t height) {
    _pShadowMaskTex->OnDestroy();

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, width, height);
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    _pShadowMaskTex->OnCreate(pGfxDevice->GetDevice(), texDesc, D3D12_RESOURCE_STATE_COMMON);

    if (_shadowMaskSRV.IsNull()) {
        _shadowMaskSRV = pGfxDevice->GetDevice()->AllocDescriptor<dx::SRV>(1);
    }

    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Format = _pShadowMaskTex->GetFormat();
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Texture2D.MipLevels = 1;
    srv.Texture2D.MostDetailedMip = 0;
    srv.Texture2D.ResourceMinLODClamp = 0.f;
    srv.Texture2D.PlaneSlice = 0;
    device->CreateShaderResourceView(_pShadowMaskTex->GetResource(), &srv, _shadowMaskSRV.GetCpuHandle());

    if (_shadowMaskUAV.IsNull()) {
        _shadowMaskUAV = pGfxDevice->GetDevice()->AllocDescriptor<dx::UAV>(1);
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
    uav.Format = _pShadowMaskTex->GetFormat();
    uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uav.Texture2D.MipSlice = 0;
    uav.Texture2D.PlaneSlice = 0;
    device->CreateUnorderedAccessView(_pShadowMaskTex->GetResource(), nullptr, &uav, _shadowMaskUAV.GetCpuHandle());
}

void RayTracingShadowPass::BuildShaderRecode(ReadonlyArraySpan<RayTracingGeometry> geometries,
    dx::ComputeContext *pComputeContext,
    dx::DispatchRaysDesc &dispatchRaysDesc) const {

    dx::BindlessCollection bindlessCollection;
    size_t shadowMaterialCount = 0;
    for (const RayTracingGeometry &geometry : geometries) {
        const Material *pMaterial = geometry.pMaterial;
        if (!RenderGroup::IsAlphaTest(pMaterial->GetRenderGroup())) {
            continue;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE handle = pMaterial->GetTextureHandle(Material::eAlbedoTex);
        bindlessCollection.AddHandle(handle);
        ++shadowMaterialCount;
    }

    struct ShadowMaterial {
        float alpha;
        float cutoff;
        int albedoTextureIndex;
        uint vertexStride;
        uint uv0Offset;
        uint sampleStateIndex;
        uint skipGeometry;
    };
    D3D12_GPU_VIRTUAL_ADDRESS shadowMaterialBuffer = 0;
    std::span<ShadowMaterial> shadowMaterials;
    if (shadowMaterialCount > 0) {
        size_t bufferSize = shadowMaterialCount * sizeof(ShadowMaterial);
        auto allocInfo = pComputeContext->AllocBuffer(bufferSize, sizeof(ShadowMaterial));
        shadowMaterialBuffer = allocInfo.virtualAddress;
        shadowMaterials = std::span(reinterpret_cast<ShadowMaterial *>(allocInfo.pBuffer), shadowMaterialCount);
    }

    dx::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
    dx::ThrowIfFailed(_pRayTracingPSO.As(&stateObjectProperties));
    void *pRayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sShadowRayGenShaderName);
    void *pMissShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sShadowMissShaderName);
    void *pOpaqueHitGroupIdentifier = stateObjectProperties->GetShaderIdentifier(sOpaqueHitGroupName);
    void *pAlphaTestHitGroupIdentifier = stateObjectProperties->GetShaderIdentifier(sAlphaTestHitGroupName);

    dispatchRaysDesc.rayGenerationShaderRecode = dx::ShaderRecode(pRayGenShaderIdentifier);
    dispatchRaysDesc.missShaderTable.push_back(dx::ShaderRecode(pMissShaderIdentifier));

    uint materialIndex = 0;
    for (const RayTracingGeometry &geometry : geometries) {
        const Material *pMaterial = geometry.pMaterial;
        if (RenderGroup::IsOpaque(pMaterial->GetRenderGroup())) {
            dispatchRaysDesc.hitGroupTable.push_back(dx::ShaderRecode(pOpaqueHitGroupIdentifier));
            continue;
        }

        uint currentMaterialIndex = materialIndex++;
        auto &shadowMaterial = shadowMaterials[currentMaterialIndex];
        shadowMaterial.alpha = pMaterial->GetAlbedo().a;
        shadowMaterial.cutoff = pMaterial->GetCutoff();
        shadowMaterial.sampleStateIndex = pMaterial->GetSamplerStateIndex();
        shadowMaterial.albedoTextureIndex = bindlessCollection.GetHandleIndex(
            pMaterial->GetTextureHandle(Material::eAlbedoTex));
        shadowMaterial.vertexStride = GetSemanticStride(geometry.pMesh->GetSemanticMask());
        shadowMaterial.uv0Offset = GetSemanticOffset(geometry.pMesh->GetSemanticMask(), SemanticIndex::eTexCoord0);

        // the geometry is not visible
        shadowMaterial.skipGeometry = shadowMaterial.albedoTextureIndex == -1 &&
                                      shadowMaterial.alpha < shadowMaterial.cutoff;

        const GPUMeshData *pGpuMeshData = geometry.pMesh->GetGPUMeshData();
        dx::ShaderRecode shaderRecode(pAlphaTestHitGroupIdentifier, _pAlphaTestLocalRootSignature.get());
        dx::LocalRootParameterData &localRootParameterData = shaderRecode.GetLocalRootParameterData();
        localRootParameterData.SetConstants(eMaterialIndex, dx::DWParam(currentMaterialIndex));
        localRootParameterData.SetView(eVertexBuffer, pGpuMeshData->GetVertexBufferView().BufferLocation);
        localRootParameterData.SetView(eIndexBuffer, pGpuMeshData->GetIndexBufferView().BufferLocation);
        localRootParameterData.SetView(eInstanceMaterial, shadowMaterialBuffer);
        localRootParameterData.SetDescriptorTable(eAlbedoTextureList, bindlessCollection.GetHandleArrayPtr());
        dispatchRaysDesc.hitGroupTable.push_back(std::move(shaderRecode));
    }
}

void RayTracingShadowPass::BuildRenderSettingUI() {
    if (!ImGui::TreeNode("RayTracingShadowPass")) {
	    return;
    }
    RenderSetting &renderSetting = RenderSetting::Get();
    float tMin = renderSetting.GetShadowRayTMin();
    if (ImGui::DragFloat("RayTMin", &tMin, 0.1f, 0.f, 1.f)) {
	    renderSetting.SetShadowRayTMin(tMin);
    }

    float tMax = renderSetting.GetShadowRayTMax();
    if (ImGui::InputFloat("RayTMax", &tMax)) {
	    renderSetting.SetShadowRayTMax(tMax);
    }

    float maxCosineTheta = renderSetting.GetShadowRayMaxCosineTheta();
    if (ImGui::DragFloat("maxCosineTheta(degrees)", &maxCosineTheta, 0.1f, 0.f, 50.f)) {
        renderSetting.SetShadowRayMaxCosineTheta(maxCosineTheta);
    }

    ImGui::TreePop();
}