#include "RayTracingShadowPass.h"

#include "Components/MeshRenderer.h"
#include "D3d12/BindlessCollection.hpp"
#include "D3d12/Context.h"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "D3d12/ShaderCompiler.h"
#include "D3d12/Texture.h"
#include "Foundation/GameTimer.h"
#include "Renderer/GfxDevice.h"
#include "Renderer/Denoiser/Denoiser.h"
#include "Renderer/RenderUtils/UserMarker.h"
#include "RenderObject/GPUMeshData.h"
#include "RenderObject/Material.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/RenderGroup.hpp"
#include "RenderObject/VertexSemantic.hpp"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Renderer/GUI/ImguiHelper.h"
#include "Renderer/RenderUtils/RenderSetting.h"
#include "Renderer/RenderUtils/RenderView.h"
#include "Utils/BuildInResource.h"

static const wchar_t *sShadowRayGenShaderName = L"ShadowRaygenShader";
static const wchar_t *sShadowMissShaderName = L"ShadowMissShader";

static const wchar_t *sOpaqueHitGroupName = L"ShadowOpaqueHitGroup";
static const wchar_t *sOpaqueClosestHitShaderName = L"ShadowOpaqueClosestHitShader";

static const wchar_t *sAlphaTestHitGroupName = L"ShadowAlphaTestHitGroup";
static const wchar_t *sAlphaClosestHitShader = L"ShadowAlphaTestClosestHitShader";

void RayTracingShadowPass::OnCreate() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
#if ENABLE_RAY_TRACING
    _pGlobalRootSignature = dx::RootSignature::Create(3, 6);
    _pGlobalRootSignature->At(eScene).InitAsBufferSRV(0);
    _pGlobalRootSignature->At(eRayGenCb).InitAsBufferCBV(0);
    _pGlobalRootSignature->At(eTable0).InitAsDescriptorTable({
        CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1),
        CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0),
    });
    _pGlobalRootSignature->SetStaticSamplers(dx::GetStaticSamplerArray());
    _pGlobalRootSignature->Generate(pGfxDevice->GetDevice());

    _pAlphaTestLocalRootSignature = dx::RootSignature::Create(5);
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
    CreatePipelineState();

    _buildRenderSettingUiHandle = GlobalCallbacks::Get().OnBuildRenderSettingGUI.Register(this,
        &RayTracingShadowPass::BuildRenderSettingUI);
#endif
}

void RayTracingShadowPass::OnDestroy() {
#if ENABLE_RAY_TRACING
    RenderPass::OnDestroy();
    _pShadowDataTex = nullptr;
    _pShadowDataTex = nullptr;
    _pGlobalRootSignature = nullptr;
    _pAlphaTestLocalRootSignature = nullptr;
    _pRayTracingPSO = nullptr;
#endif
}

void RayTracingShadowPass::GenerateShadowMap(const DrawArgs &args) {
    UserMarker userMarker(args.pComputeContext, "GenerateShadowMaskMap");
#if ENABLE_RAY_TRACING
    GenerateShadowData(args);
    ShadowDenoise(args);
#else
    args.pComputeContext->Transition(_pShadowMaskTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    args.pComputeContext->FlushResourceBarriers();
    args.pComputeContext->ClearUnorderedAccessViewFloat(_pShadowMaskTex->GetResource(),
        _shadowMaskUAV.GetCpuHandle(),
        glm::vec4(1.f));
    args.pComputeContext->Transition(_pShadowMaskTex->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#endif
}

auto RayTracingShadowPass::GetShadowMaskSRV() const -> D3D12_CPU_DESCRIPTOR_HANDLE {
    return _shadowMaskSRV.GetCpuHandle();
}

void RayTracingShadowPass::OnResize(const ResolutionInfo &resolution) {
    _pShadowMaskTex = nullptr;

    size_t textureWidth = resolution.renderWidth;
    size_t textureHeight = resolution.renderHeight;
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, textureWidth, textureHeight);
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    _pShadowMaskTex = dx::Texture::Create(pGfxDevice->GetDevice(), texDesc, D3D12_RESOURCE_STATE_COMMON);

    if (_shadowMaskSRV.IsNull()) {
        _shadowMaskSRV = pGfxDevice->GetDevice()->AllocDescriptor<dx::SRV>(1);
    }

    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
    device->CreateShaderResourceView(_pShadowMaskTex->GetResource(), nullptr, _shadowMaskSRV.GetCpuHandle());

    if (_shadowMaskUAV.IsNull()) {
        _shadowMaskUAV = pGfxDevice->GetDevice()->AllocDescriptor<dx::UAV>(1);
    }
    device->CreateUnorderedAccessView(_pShadowMaskTex->GetResource(), nullptr, nullptr, _shadowMaskUAV.GetCpuHandle());

    _pShadowDataTex = nullptr;
    texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16_FLOAT, textureWidth, textureHeight);
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    _pShadowDataTex = dx::Texture::Create(pGfxDevice->GetDevice(), texDesc, D3D12_RESOURCE_STATE_COMMON);
    if (_shadowDataUAV.IsNull()) {
        _shadowDataUAV = pGfxDevice->GetDevice()->AllocDescriptor<dx::UAV>(1);
    }
    device->CreateUnorderedAccessView(_pShadowDataTex->GetResource(), nullptr, nullptr, _shadowDataUAV.GetCpuHandle());
}

void RayTracingShadowPass::GenerateShadowData(const DrawArgs &args) {
    dx::ComputeContext *pComputeContext = args.pComputeContext;
    const RegionTopLevelAS *pRegionTopLevelAs = args.pRegionTopLevelAs;
    UserMarker userMarker(pComputeContext, "RayTracingShadowData");

    pComputeContext->SetComputeRootSignature(_pGlobalRootSignature.Get());
    pComputeContext->SetRayTracingPipelineState(_pRayTracingPSO.Get());
    pComputeContext->SetComputeRootShaderResourceView(eScene,
        pRegionTopLevelAs->GetTopLevelAS()->GetGPUVirtualAddress());

    // clang-format off
    struct RayGenCB {
        glm::mat4x4 matInvViewProj;
        glm::vec3   lightDirection;
        uint        enableSoftShadow;
        glm::vec4   zBufferParams;
		float       cosSunAngularRadius;
		float       tanSunAngularRadius;
        uint        frameCount;
        float       maxT;
        float       minT;
		uint        maxRecursiveDepth;
        float       backgroundNDCDepth;
    };
    // clang-format on

    const ShadowConfig &shadowConfig = RenderSetting::Get().GetShadowConfig();

    float angularDiameter = glm::radians(shadowConfig.sunAngularDiameter * 0.5f);

    RayGenCB rayGenCb;
    rayGenCb.matInvViewProj = args.pRenderView->GetCBPrePass().matInvJitteredViewProj;
    rayGenCb.lightDirection = args.pRenderView->GetCBLighting().directionalLight.direction;
    rayGenCb.enableSoftShadow = angularDiameter != 0.f;
    rayGenCb.zBufferParams = args.pRenderView->GetCBPrePass().zBufferParams;
    rayGenCb.cosSunAngularRadius = std::cos(angularDiameter);
    rayGenCb.tanSunAngularRadius = std::tan(angularDiameter);
    rayGenCb.frameCount = GameTimer::Get().GetFrameCount();
    rayGenCb.maxT = shadowConfig.rayTMax;
    rayGenCb.minT = shadowConfig.rayTMin;
    rayGenCb.maxRecursiveDepth = shadowConfig.rayTraceMaxRecursiveDepth;
    rayGenCb.backgroundNDCDepth = RenderSetting::Get().GetDepthClearValue();
    pComputeContext->SetComputeRootDynamicConstantBuffer(eRayGenCb, rayGenCb);

    D3D12_CPU_DESCRIPTOR_HANDLE table0[2];
    table0[eDepthTex] = args.depthTexSRV;
    table0[eOutputShadowData] = _shadowDataUAV.GetCpuHandle();
    pComputeContext->SetDynamicViews(eTable0, table0);
    pComputeContext->Transition(_pShadowDataTex->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    dx::DispatchRaysDesc dispatchRaysDesc = {};
    BuildShaderRecode(pRegionTopLevelAs->GetGeometries(), pComputeContext, dispatchRaysDesc);
    dispatchRaysDesc.width = _pShadowDataTex->GetWidth();
    dispatchRaysDesc.height = _pShadowDataTex->GetHeight();
    dispatchRaysDesc.depth = 1;
    pComputeContext->DispatchRays(dispatchRaysDesc);
}

void RayTracingShadowPass::ShadowDenoise(const DrawArgs &args) {
    dx::ComputeContext *pComputeContext = args.pComputeContext;
    UserMarker userMarker(pComputeContext, "ShadowDenoise");
    Denoiser *pDenoiser = args.pDenoiser;
    ShadowDenoiseDesc denoiseDesc;

    DenoiserCommonSettings commonSettingRestore = pDenoiser->GetCommonSetting();
    DenoiserCommonSettings shadowCommonSetting = commonSettingRestore;
    shadowCommonSetting.rectSize[0] = _pShadowMaskTex->GetWidth();
    shadowCommonSetting.rectSize[1] = _pShadowMaskTex->GetHeight();
    shadowCommonSetting.rectSizePrev[0] = _pShadowMaskTex->GetWidth();
    shadowCommonSetting.rectSizePrev[1] = _pShadowMaskTex->GetHeight();
    pDenoiser->SetCommonSetting(shadowCommonSetting);

    const ShadowConfig &shadowConfig = RenderSetting::Get().GetShadowConfig();
    denoiseDesc.settings.blurRadiusScale = shadowConfig.denoiseBlurRadiusScale;
    denoiseDesc.settings.planeDistanceSensitivity = shadowConfig.planeDistanceSensitivity;

    denoiseDesc.pComputeContext = pComputeContext;
    denoiseDesc.pShadowDataTex = _pShadowDataTex.Get();
    denoiseDesc.pOutputShadowMaskTex = _pShadowMaskTex.Get();
    pDenoiser->ShadowDenoise(denoiseDesc);
    pDenoiser->SetCommonSetting(commonSettingRestore);
}

void RayTracingShadowPass::CreatePipelineState() {
    ShaderLoadInfo shaderLoadInfo = {};
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/RayTracingShadow.hlsl");
    shaderLoadInfo.shaderType = dx::ShaderType::eLib;

    D3D12_SHADER_BYTECODE byteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);
    Assert(byteCode.pShaderBytecode != nullptr);

    CD3DX12_STATE_OBJECT_DESC rayTracingPipeline{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};
    CD3DX12_DXIL_LIBRARY_SUBOBJECT *pLib = rayTracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    pLib->SetDXILLibrary(&byteCode);
    pLib->DefineExport(sShadowRayGenShaderName);
    pLib->DefineExport(sOpaqueClosestHitShaderName);
    pLib->DefineExport(sAlphaClosestHitShader);
    pLib->DefineExport(sShadowMissShaderName);

    CD3DX12_HIT_GROUP_SUBOBJECT *pOpaqueHitGroup = rayTracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    pOpaqueHitGroup->SetHitGroupExport(sOpaqueHitGroupName);
    pOpaqueHitGroup->SetClosestHitShaderImport(sOpaqueClosestHitShaderName);
    pOpaqueHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    CD3DX12_HIT_GROUP_SUBOBJECT *pAlphaTestHitGroup = rayTracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    pAlphaTestHitGroup->SetHitGroupExport(sAlphaTestHitGroupName);
    pAlphaTestHitGroup->SetClosestHitShaderImport(sAlphaClosestHitShader);
    pAlphaTestHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    auto *pShaderConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    size_t payloadSize = sizeof(float) + sizeof(uint);
    size_t attributeSize = 2 * sizeof(float);
    pShaderConfig->Config(payloadSize, attributeSize);

    CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT
    *pLocalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    pLocalRootSignature->SetRootSignature(_pAlphaTestLocalRootSignature->GetRootSignature());
    CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT *rootSignatureAssociation = rayTracingPipeline.CreateSubobject<
        CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    rootSignatureAssociation->SetSubobjectToAssociate(*pLocalRootSignature);
    rootSignatureAssociation->AddExport(sAlphaTestHitGroupName);

    pLocalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    pLocalRootSignature->SetRootSignature(BuildInResource::Get().GetEmptyLocalRootSignature()->GetRootSignature());
    rootSignatureAssociation = rayTracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    rootSignatureAssociation->SetSubobjectToAssociate(*pLocalRootSignature);
    rootSignatureAssociation->AddExport(sOpaqueHitGroupName);

    auto *pGlobalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    pGlobalRootSignature->SetRootSignature(_pGlobalRootSignature->GetRootSignature());

    auto *pPipelineConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pPipelineConfig->Config(RenderSetting::Get().GetShadowConfig().rayTraceMaxRecursiveDepth);

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
#if ENABLE_RAY_TRACING
    dx::ThrowIfFailed(device->CreateStateObject(rayTracingPipeline, IID_PPV_ARGS(&_pRayTracingPSO)));
#endif
}

void RayTracingShadowPass::BuildShaderRecode(ReadonlyArraySpan<RayTracingGeometry> geometries,
    dx::ComputeContext *pComputeContext,
    dx::DispatchRaysDesc &dispatchRaysDesc) const {

    dx::BindlessCollection bindlessCollection;
    size_t shadowMaterialCount = 0;
    for (const RayTracingGeometry &geometry : geometries) {
        const MeshRenderer *pMeshRender = geometry.GetMeshRenderer();
        const Material *pMaterial = pMeshRender->GetMaterial().get();
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

    auto pEmptyLocalRootSignature = BuildInResource::Get().GetEmptyLocalRootSignature();

    uint materialIndex = 0;
    for (const RayTracingGeometry &geometry : geometries) {
        const MeshRenderer *pMeshRender = geometry.GetMeshRenderer();
        const Material *pMaterial = pMeshRender->GetMaterial().get();
        if (RenderGroup::IsOpaque(pMaterial->GetRenderGroup())) {
            dispatchRaysDesc.hitGroupTable.push_back(
                dx::ShaderRecode(pOpaqueHitGroupIdentifier, pEmptyLocalRootSignature.Get()));
            continue;
        }

        const Mesh *pMesh = pMeshRender->GetMesh().get();
        uint currentMaterialIndex = materialIndex++;
        auto &shadowMaterial = shadowMaterials[currentMaterialIndex];
        shadowMaterial.alpha = pMaterial->GetAlbedo().a;
        shadowMaterial.cutoff = pMaterial->GetCutoff();
        shadowMaterial.sampleStateIndex = pMaterial->GetSamplerStateIndex();
        shadowMaterial.albedoTextureIndex = bindlessCollection.GetHandleIndex(
            pMaterial->GetTextureHandle(Material::eAlbedoTex));
        shadowMaterial.vertexStride = GetSemanticStride(pMesh->GetSemanticMask());
        shadowMaterial.uv0Offset = GetSemanticOffset(pMesh->GetSemanticMask(), SemanticIndex::eTexCoord0);

        // the geometry is not visible
        shadowMaterial.skipGeometry = shadowMaterial.albedoTextureIndex == -1 &&
                                      shadowMaterial.alpha < shadowMaterial.cutoff;

        const GPUMeshData *pGpuMeshData = pMesh->GetGPUMeshData();
        dx::ShaderRecode shaderRecode(pAlphaTestHitGroupIdentifier, _pAlphaTestLocalRootSignature.Get());
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
    ShadowConfig &shadowConfig = renderSetting.GetShadowConfig();

    ImGui::DragFloat("RayTMin", &shadowConfig.rayTMin, 0.001f, 0.f, 5.f);
    ImGui::InputFloat("RayTMax", &shadowConfig.rayTMax);

    ImGui::SliderFloat("SunAngularDiameter", &shadowConfig.sunAngularDiameter, 0.f, 5.f);
    if (ImGui::SliderInt("RayTraceMaxRecursiveDepth", &shadowConfig.rayTraceMaxRecursiveDepth, 1, 5)) {
        GfxDevice::GetInstance()->GetDevice()->WaitForGPUFlush();
        _pRayTracingPSO = nullptr;
        CreatePipelineState();
    }
    ImGui::DragFloat("BlurRadiusScale", &shadowConfig.denoiseBlurRadiusScale, 0.01f, 1.f, 3.f);
    ImGui::SliderFloat("PlaneDistanceSensitivity", &shadowConfig.planeDistanceSensitivity, 0.f, 0.5f);

    ImGui::TreePop();
}