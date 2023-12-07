#include "StandardMaterial.h"

#include "VertexSemantic.hpp"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "Foundation/ColorUtil.hpp"
#include "Renderer/GfxDevice.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Utils/GlobalCallbacks.h"

namespace ShaderFeatures {

static const char *sEnableAlphaTest = "ENABLE_ALPHA_TEST";
static const char *sEnableEmission = "ENABLE_EMISSION";
static const char *sEnableNormalScale = "ENABLE_NORMAL_SCALE";

static const char *sTextureKeyword[] = {
    "ENABLE_ALBEDO_TEXTURE",
    "ENABLE_AMBIENT_OCCLUSION_TEXTURE",
    "ENABLE_EMISSION_TEXTURE",
    "ENABLE_METAL_ROUGHNESS_TEXTURE",
    "ENABLE_NORMAL_TEX",
};

}    // namespace ShaderFeatures

class StandardMaterialManager {
public:
    StandardMaterialManager() {
        _createCallbackHandle = GlobalCallbacks::Get().onCreate.Register(this, &StandardMaterialManager::OnCreate);
        _destroyCallbackHandle = GlobalCallbacks::Get().onDestroy.Register(this, &StandardMaterialManager::OnDestroy);
    }
    void OnCreate() {
        _pRootSignature = std::make_unique<dx::RootSignature>();
        _pRootSignature->OnCreate(5, 6);
        _pRootSignature->At(0).InitAsBufferCBV(0);    // gCbPrePass;
        _pRootSignature->At(1).InitAsBufferCBV(0);    // gCbPreObject;
        _pRootSignature->At(2).InitAsBufferCBV(0);    // gCbLighting;
        _pRootSignature->At(3).InitAsBufferCBV(0);    // gCbLighting;

        auto range = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            -1,
            0,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
        _pRootSignature->At(4).InitAsDescriptorTable({// gTextureList
            range});

        //_pRootSignature->Generate(GfxDevice::GetInstance()->GetDevice(), D3D12_ROOT_SIGNATURE_FLAG_)
    }
    void OnDestroy() {
        _pRootSignature = nullptr;
        _pipelineStateMap.clear();
    }

    auto GetRootSignature() -> std::shared_ptr<dx::RootSignature> {
        return _pRootSignature;
    }
    auto GetPipelineState(StandardMaterial *pMaterial) -> dx::WRL::ComPtr<ID3D12PipelineState> {

        size_t hash = std::hash<std::string>{}(pMaterial->_defineList.ToString());
        auto iter = _pipelineStateMap.find(hash);
        if (iter != _pipelineStateMap.end()) {
            return iter->second;
        }

        ShaderLoadInfo shaderLoadInfo;
        shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/StandardMaterial.hlsli");
        shaderLoadInfo.entryPoint = "VSMain";
        shaderLoadInfo.shaderType = dx::ShaderType::eVS;
        shaderLoadInfo.pDefineList = &pMaterial->_defineList;
        D3D12_SHADER_BYTECODE vsByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);
        Assert(vsByteCode.pShaderBytecode != nullptr);

        shaderLoadInfo.entryPoint = "PSMain";
        shaderLoadInfo.shaderType = dx::ShaderType::ePS;
        D3D12_SHADER_BYTECODE psByteCode = ShaderManager::GetInstance()->LoadShaderByteCode(shaderLoadInfo);

        struct PipelineStateStream {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendDesc;
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

        auto inputLayouts = SemanticMaskToVertexInputElements(pMaterial->_semanticMask);
        pipelineDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC{
            inputLayouts.data(),
            static_cast<UINT>(inputLayouts.size()),
        };

        if (pMaterial->_renderMode == StandardMaterial::eTransparent) {
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

        D3D12_RT_FORMAT_ARRAY rtvFormats;
        rtvFormats.RTFormats[0] = pGfxDevice->GetRenderTargetFormat();
        rtvFormats.NumRenderTargets = 1;
        pipelineDesc.RTVFormats = rtvFormats;

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
            sizeof(PipelineStateStream),
            &pipelineDesc,
        };

        dx::WRL::ComPtr<ID3D12PipelineState> pPipelineState;
        dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
        dx::ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pPipelineState)));

        _pipelineStateMap[hash] = pPipelineState;
        return pPipelineState;
    }
private:
    using PipelineStateMap = std::unordered_map<size_t, dx::WRL::ComPtr<ID3D12PipelineState>>;
    // clang-format off
    CallbackHandle                      _createCallbackHandle;
    CallbackHandle                      _destroyCallbackHandle;
    PipelineStateMap                    _pipelineStateMap;
    std::shared_ptr<dx::RootSignature>  _pRootSignature;
    // clang-format on
};

static StandardMaterialManager gMaterialManager = {};

StandardMaterial::StandardMaterial()
    : _semanticMask(SemanticMask::eVertex | SemanticMask::eNormal | SemanticMask::eColor),
      _renderMode(eOpaque),
      _albedo(Colors::White),
      _emission(Colors::Black),
      _tilingAndOffset(1.f),
      _cutoff(0.f),
      _roughness(0.5f),
      _metallic(0.5f),
      _normalScale(1.f) {
}

StandardMaterial::~StandardMaterial() {
}

void StandardMaterial::SetRenderMode(RenderMode renderMode) {
    _renderMode = renderMode;
    _defineList.Set(ShaderFeatures::sEnableAlphaTest, (renderMode == eAlphaTest));
    _pipeStateDirty = true;
}

void StandardMaterial::SetRenderFeatures(RenderFeatures features, bool enable) {
    switch (features) {
    case eEnableEmission:
        _defineList.Set(ShaderFeatures::sEnableEmission, enable);
        _pipeStateDirty = true;
        break;
    case eEnableNormalScale:
        _defineList.Set(ShaderFeatures::sEnableNormalScale, enable);
        _pipeStateDirty = true;
        break;
    }
}

void StandardMaterial::SetTextures(TextureType textureType, std::shared_ptr<dx::Texture> pTexture) {
    _textures[textureType] = std::move(pTexture);
    _defineList.Set(ShaderFeatures::sTextureKeyword[textureType], _textures[textureType] != nullptr);
    _pipeStateDirty = true;

    // todo create texture view
    // todo update SemanticMask
}

void StandardMaterial::SetAlbedo(const glm::vec4 &albedo) {
    _albedo = albedo;
}

void StandardMaterial::SetEmission(const glm::vec4 &emission) {
    _emission = emission;
}

void StandardMaterial::SetTillingAndOffset(const glm::vec4 &tilingAndOffset) {
    _tilingAndOffset = tilingAndOffset;
}

void StandardMaterial::SetCutoff(float cutoff) {
    _cutoff = cutoff;
}

void StandardMaterial::SetRoughness(float roughness) {
    _roughness = roughness;
}

void StandardMaterial::SetMetallic(float metallic) {
    _metallic = metallic;
}

void StandardMaterial::SetNormalScale(float normalScale) {
    _normalScale = normalScale;
}
