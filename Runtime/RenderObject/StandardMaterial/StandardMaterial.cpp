#include "StandardMaterial.h"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "D3d12/Texture.h"
#include "Foundation/ColorUtil.hpp"
#include "Renderer/GfxDevice.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Utils/GlobalCallbacks.h"
#include "Foundation/HashUtil.hpp"
#include "Renderer/RenderPasses/ForwardPass.h"
#include "RenderObject/RenderGroup.hpp"
#include "StandardMaterialDataManater.h"

namespace ShaderFeatures {

static const char *sEnableAlphaTest = "ENABLE_ALPHA_TEST";
static const char *sEnableVertexColor = "ENABLE_VERTEX_COLOR";

static const char *sTextureKeyword[] = {
    "ENABLE_ALBEDO_TEXTURE",
    "ENABLE_AMBIENT_OCCLUSION_TEXTURE",
    "ENABLE_EMISSION_TEXTURE",
    "ENABLE_METAL_ROUGHNESS_TEXTURE",
    "ENABLE_NORMAL_TEX",
};

}    // namespace ShaderFeatures

StandardMaterial::StandardMaterial()
    : _renderGroup(RenderGroup::eOpaque),
      _pipeStateDirty(false),
      _pRootSignature(nullptr),
      _pPipelineState(nullptr),
      _pipelineID(0) {

    _cbPreMaterial.albedo = Colors::White;
    _cbPreMaterial.emission = Colors::Black;
    _cbPreMaterial.tilingAndOffset = glm::vec4(1.f, 1.f, 0.f, 0.f);
    _cbPreMaterial.cutoff = 0.f;
    _cbPreMaterial.roughness = 0.5f;
    _cbPreMaterial.metallic = 0.5f;
    _cbPreMaterial.normalScale = 1.f;
    _cbPreMaterial.samplerStateIndex = eLinearClamp;
    _cbPreMaterial.albedoTexIndex = 0;
    _cbPreMaterial.ambientOcclusionTexIndex = 0;
    _cbPreMaterial.emissionTexIndex = 0;
    _cbPreMaterial.metalRoughnessTexIndex = 0;
    _cbPreMaterial.normalTexIndex = 0;
    _cbPreMaterial.padding0 = 0;
    _cbPreMaterial.padding1 = 0;
}

StandardMaterial::~StandardMaterial() {
}

void StandardMaterial::SetRenderGroup(uint16_t renderGroup) {
    _renderGroup = renderGroup;
    _defineList.Set(ShaderFeatures::sEnableAlphaTest, (RenderGroup::IsAlphaTest(renderGroup)));
    _pipeStateDirty = true;
}

void StandardMaterial::SetTextures(TextureType textureType, std::shared_ptr<dx::Texture> pTexture) {
    _textures[textureType] = std::move(pTexture);
    _defineList.Set(ShaderFeatures::sTextureKeyword[textureType], _textures[textureType] != nullptr);
    _pipeStateDirty = true;

    _textureHandles[textureType] = dx::SRV{};
    if (_textures[textureType] != nullptr) {
        _textureHandles[textureType] = StandardMaterialDataManager::Get().GetTextureSRV(_textures[textureType].get());
    }
}

void StandardMaterial::SetAlbedo(const glm::vec4 &albedo) {
   _cbPreMaterial.albedo = albedo;
}

void StandardMaterial::SetEmission(const glm::vec4 &emission) {
    _cbPreMaterial.emission = emission;
}

void StandardMaterial::SetTillingAndOffset(const glm::vec4 &tilingAndOffset) {
    _cbPreMaterial.tilingAndOffset = tilingAndOffset;
}

void StandardMaterial::SetCutoff(float cutoff) {
    _cbPreMaterial.cutoff = cutoff;
}

void StandardMaterial::SetRoughness(float roughness) {
    _cbPreMaterial.roughness = roughness;
}

void StandardMaterial::SetMetallic(float metallic) {
    _cbPreMaterial.metallic = metallic;
}

void StandardMaterial::SetNormalScale(float normalScale) {
    _cbPreMaterial.normalScale = normalScale;
}

void StandardMaterial::SetSamplerAddressMode(SamplerAddressMode mode) {
    _cbPreMaterial.samplerStateIndex = mode;
}

bool StandardMaterial::UpdatePipelineState(SemanticMask meshSemanticMask) {
    SemanticMask pipelineSemanticMask = SemanticMask::eNormal | SemanticMask::eVertex;
    if (_textures[eNormalTex] != nullptr) {
        pipelineSemanticMask = SetFlags(pipelineSemanticMask, SemanticMask::eTangent);
    }
    for (auto &texture : _textures) {
        if (texture != nullptr) {
            pipelineSemanticMask = SetFlags(pipelineSemanticMask, SemanticMask::eTexCoord0);
            break;
        }
    }
    if (!HasAllFlags(meshSemanticMask, pipelineSemanticMask)) {
        return false;
    }

    if (HasFlag(meshSemanticMask, SemanticMask::eColor)) {
        pipelineSemanticMask = SetFlags(pipelineSemanticMask, SemanticMask::eColor);
        _defineList.Set(ShaderFeatures::sEnableVertexColor, 1);
    } else {
        _defineList.Set(ShaderFeatures::sEnableVertexColor, 0);
    }

    StandardMaterialDataManager &mgr = StandardMaterialDataManager::Get();
    _pipeStateDirty = false;
    _pPipelineState = mgr.GetPipelineState(this, meshSemanticMask, pipelineSemanticMask);
    _pipelineID = mgr.GetPipelineID(_pPipelineState);
    _pRootSignature = mgr.GetRootSignature();
    return true;
}

bool StandardMaterial::PipelineStateDirty() const {
    return _pipeStateDirty;
}

auto StandardMaterial::GetRenderGroup() const -> uint16_t {
    return _renderGroup;
}

auto StandardMaterial::GetPipelineID() const -> uint16_t {
    return _pipelineID;
}

auto StandardMaterial::GetMaterialID() const -> uint16_t {
    return StandardMaterialDataManager::Get().GetMaterialID();
}
