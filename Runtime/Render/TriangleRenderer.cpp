#include "TriangleRenderer.h"
#include "D3d12/Context.h"
#include "D3d12/FrameResource.h"
#include "D3d12/FrameResourceRing.h"
#include "D3d12/ShaderCompiler.h"
#include "D3d12/StaticBuffer.h"
#include "D3d12/SwapChain.h"
#include "Foundation/GameTimer.h"
#include "Foundation/Logger.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"

enum RootIndex {
    AccelerationStructureSlot = 0,
    OutputRenderTarget = 1,
    ConstantBuffer = 2,
};

static std::wstring_view RayGenShaderName = L"MyRaygenShader";
static std::wstring_view ClosestHitShaderName = L"MyClosestHitShader";
static std::wstring_view MissShaderName = L"MyMissShader";
static std::wstring_view HitGroupName = L"MyHitGroup";

void TriangleRenderer::OnCreate(uint32_t numBackBuffer, HWND hwnd) {
    Renderer::OnCreate(numBackBuffer, hwnd);
    CreateGeometry();
    CreateRootSignature();
    CreateRayTracingPipelineStateObject();
}

void TriangleRenderer::OnDestroy() {
    _pTriangleStaticBuffer->OnDestroy();
    Renderer::OnDestroy();
}

void TriangleRenderer::OnRender(GameTimer &timer) {
    Renderer::OnRender(timer);

    static uint64_t frameCount = 0;
    if (static_cast<uint64_t>(timer.GetTotalTime()) > frameCount) {
        Logger::Info("fps {}", timer.GetFPS());
        frameCount = static_cast<uint64_t>(timer.GetTotalTime());
    }

    _pFrameResourceRing->OnBeginFrame();
    dx::FrameResource &frameResource = _pFrameResourceRing->GetCurrentFrameResource();
    std::shared_ptr<dx::GraphicsContext> pGraphicsCtx = frameResource.AllocGraphicsContext();

    CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(_width), static_cast<float>(_height));
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(_width), static_cast<LONG>(_height)};
    pGraphicsCtx->SetViewport(viewport);
    pGraphicsCtx->SetScissor(scissor);
    pGraphicsCtx->SetRenderTargets(_pSwapChain->GetCurrentBackBufferRTV());
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    pGraphicsCtx->FlushResourceBarriers();

    glm::vec4 color = {
        std::sin(timer.GetTotalTime()) * 0.5f + 0.5f,
        std::cos(timer.GetTotalTime()) * 0.5f + 0.5f,
        0.f,
        1.f,
    };

    pGraphicsCtx->ClearRenderTargetView(_pSwapChain->GetCurrentBackBufferRTV(), color);
    pGraphicsCtx->Transition(_pSwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
    frameResource.ExecuteContexts(pGraphicsCtx.get());

    _pFrameResourceRing->OnEndFrame();
    _pSwapChain->Present();
}

void TriangleRenderer::CreateGeometry() {
    _pTriangleStaticBuffer = std::make_shared<dx::StaticBuffer>();
    uint16_t indices[] = {0, 1, 2};

    float depthValue = 1.0;
    float offset = 0.7f;
    glm::vec3 vertices[] = {{0, -offset, depthValue}, {-offset, offset, depthValue}, {offset, offset, depthValue}};

    _pTriangleStaticBuffer->OnCreate(_pDevice.get(), sizeof(indices) + sizeof(vertices));
    dx::StaticBufferUploadHeap uploadHeap(*_pTriangleStaticBuffer, *_pUploadHeap);
    _vertexBufferView = uploadHeap.AllocVertexBuffer(std::size(vertices), sizeof(glm::vec3), vertices).value();
    _indexBufferView = uploadHeap.AllocIndexBuffer(std::size(indices), sizeof(uint16_t), indices).value();
    uploadHeap.DoUpload();
}

void TriangleRenderer::CreateRootSignature() {
    _rootSignature.Reset(3, 0);
    _rootSignature.At(AccelerationStructureSlot).InitAsBufferSRV(0);
    _rootSignature.At(OutputRenderTarget).InitAsBufferUAV(0);
    _rootSignature.At(ConstantBuffer).InitAsBufferCBV(0);
    _rootSignature.Finalize(_pDevice.get());
}

void TriangleRenderer::CreateRayTracingPipelineStateObject() {
    ShaderManager *pShaderManager = ShaderManager::GetInstance();
    ShaderLoadInfo shaderLoadInfo;
    shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/HelloWorldRaytracing.hlsl");
    shaderLoadInfo.shaderType = dx::ShaderType::eLib;
    shaderLoadInfo.pDefineList = nullptr;
    D3D12_SHADER_BYTECODE byteCode = pShaderManager->LoadShaderByteCode(shaderLoadInfo);
    Assert(byteCode.pShaderBytecode != nullptr);

    CD3DX12_STATE_OBJECT_DESC rayTracingPipeline{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};
    CD3DX12_DXIL_LIBRARY_SUBOBJECT *pLib = rayTracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    pLib->SetDXILLibrary(&byteCode);
    pLib->DefineExport(RayGenShaderName.data());
    pLib->DefineExport(ClosestHitShaderName.data());
    pLib->DefineExport(MissShaderName.data());

    CD3DX12_HIT_GROUP_SUBOBJECT *pHitGroup = rayTracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    pHitGroup->SetClosestHitShaderImport(ClosestHitShaderName.data());
    pHitGroup->SetHitGroupExport(HitGroupName.data());
    pHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    auto *pShaderConfig = rayTracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    size_t payloadSize = 4 * sizeof(float);     // float4
    size_t attributeSize = 2 * sizeof(float);
    pShaderConfig->Config(payloadSize, attributeSize);

    auto *pLocalRootSignature = rayTracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    pLocalRootSignature->SetRootSignature(_rootSignature.GetRootSignature());
    
}
