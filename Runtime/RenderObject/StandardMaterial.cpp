#include "StandardMaterial.h"
#include "D3d12/RootSignature.h"
#include "Foundation/ColorUtil.hpp"
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

class StandardMaterialData {
public:
    StandardMaterialData() {
        _createCallbackHandle = GlobalCallbacks::Get().onCreate.Register(this, &StandardMaterialData::OnCreate);
        _destroyCallbackHandle = GlobalCallbacks::Get().onDestroy.Register(this, &StandardMaterialData::OnDestroy);
    }
    void OnCreate() {
    }
    void OnDestroy() {
        _pRootSignature = nullptr;
        _pipelineStateMap.clear();
    }

    auto GetRootSignature() -> std::shared_ptr<dx::RootSignature> {
        return _pRootSignature;
    }
    auto GetPipelineState(const dx::DefineList &defineList, StandardMaterial::RenderMode renderMode)
        -> dx::WRL::ComPtr<ID3D12PipelineState> {

        size_t hash = std::hash<std::string>{}(defineList.ToString());
        auto iter = _pipelineStateMap.find(hash);
        if (iter != _pipelineStateMap.end()) {
            return iter->second;
        }

        ShaderLoadInfo shaderLoadInfo;
        shaderLoadInfo.sourcePath = AssetProjectSetting::ToAssetPath("Shaders/StandardMaterial.hlsli");
        shaderLoadInfo.entryPoint = "VSMain";
        shaderLoadInfo.shaderType = dx::ShaderType::eVS;
        shaderLoadInfo.pDefineList = &defineList;
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
	        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
	        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	    };
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

static StandardMaterialData gMaterialData = {};

StandardMaterial::StandardMaterial()
    : _renderMode(eOpaque),
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
