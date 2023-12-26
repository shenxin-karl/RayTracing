#include "Material.h"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "D3d12/Texture.h"
#include "Foundation/ColorUtil.hpp"
#include "ShaderLoader/ShaderManager.h"
#include "Foundation/HashUtil.hpp"
#include "Renderer/RenderPasses/ForwardPass.h"
#include "RenderObject/RenderGroup.hpp"
#include "RenderObject/VertexSemantic.hpp"

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

Material::Material()
    : _renderGroup(RenderGroup::eOpaque), _pipelineIDDirty(false), _pipelineSemanticMask(), _pipelineID(0) {

    _cbPreMaterial.albedo = Colors::White;
    _cbPreMaterial.emission = Colors::Black;
    _cbPreMaterial.tilingAndOffset = glm::vec4(1.f, 1.f, 0.f, 0.f);
    _cbPreMaterial.cutoff = 0.f;
    _cbPreMaterial.roughness = 0.5f;
    _cbPreMaterial.metallic = 0.5f;
    _cbPreMaterial.normalScale = 1.f;
    _cbPreMaterial.samplerStateIndex = eLinearWrap;
    _cbPreMaterial.albedoTexIndex = 0;
    _cbPreMaterial.ambientOcclusionTexIndex = 0;
    _cbPreMaterial.emissionTexIndex = 0;
    _cbPreMaterial.metalRoughnessTexIndex = 0;
    _cbPreMaterial.normalTexIndex = 0;
    _cbPreMaterial.padding0 = 0;
    _cbPreMaterial.padding1 = 0;
}

Material::~Material() {
}

void Material::SetRenderGroup(uint16_t renderGroup) {
    _renderGroup = renderGroup;
    _defineList.Set(ShaderFeatures::sEnableAlphaTest, (RenderGroup::IsAlphaTest(renderGroup)));
    _pipelineIDDirty = true;
}

void Material::SetTextures(TextureType textureType, std::shared_ptr<dx::Texture> pTexture, dx::SRV srv) {
    _textures[textureType] = std::move(pTexture);
    _defineList.Set(ShaderFeatures::sTextureKeyword[textureType], _textures[textureType] != nullptr);
    _pipelineIDDirty = true;

    _textureHandles[textureType] = dx::SRV{};
    if (_textures[textureType] != nullptr) {
        _textureHandles[textureType] = std::move(srv);
    }
}

void Material::SetAlbedo(const glm::vec4 &albedo) {
    _cbPreMaterial.albedo = albedo;
}

void Material::SetEmission(const glm::vec4 &emission) {
    _cbPreMaterial.emission = emission;
}

void Material::SetTillingAndOffset(const glm::vec4 &tilingAndOffset) {
    _cbPreMaterial.tilingAndOffset = tilingAndOffset;
}

void Material::SetCutoff(float cutoff) {
    _cbPreMaterial.cutoff = cutoff;
}

void Material::SetRoughness(float roughness) {
    _cbPreMaterial.roughness = roughness;
}

void Material::SetMetallic(float metallic) {
    _cbPreMaterial.metallic = metallic;
}

void Material::SetNormalScale(float normalScale) {
    _cbPreMaterial.normalScale = normalScale;
}

void Material::SetSamplerAddressMode(SamplerAddressMode mode) {
    _cbPreMaterial.samplerStateIndex = mode;
}

static std::vector<size_t> sPipelineIDList = {};
size_t GetPipelineIDByHash(size_t hash) {
    for (size_t i = 0; i < sPipelineIDList.size(); ++i) {
        if (sPipelineIDList[i] == hash) {
            return i;
        }
    }
    sPipelineIDList.push_back(hash);
    return sPipelineIDList.size() - 1;
}

bool Material::UpdatePipelineID(SemanticMask meshSemanticMask) {
    _pipelineSemanticMask = SemanticMask::eNormal | SemanticMask::eVertex;
    if (_textures[eNormalTex] != nullptr) {
        _pipelineSemanticMask = SetFlags(_pipelineSemanticMask, SemanticMask::eTangent);
    }
    for (auto &texture : _textures) {
        if (texture != nullptr) {
            _pipelineSemanticMask = SetFlags(_pipelineSemanticMask, SemanticMask::eTexCoord0);
            break;
        }
    }
    if (!HasAllFlags(meshSemanticMask, _pipelineSemanticMask)) {
        return false;
    }

    if (HasFlag(meshSemanticMask, SemanticMask::eColor)) {
        _pipelineSemanticMask = SetFlags(_pipelineSemanticMask, SemanticMask::eColor);
        _defineList.Set(ShaderFeatures::sEnableVertexColor, 1);
    } else {
        _defineList.Set(ShaderFeatures::sEnableVertexColor, 0);
    }

    size_t hash = hash_value(_defineList.ToString());
    hash = combine_and_hash_value(hash, _renderGroup);
    hash = combine_and_hash_value(hash, static_cast<size_t>(meshSemanticMask));
    hash = combine_and_hash_value(hash, static_cast<size_t>(_pipelineSemanticMask));
    _pipelineID = GetPipelineIDByHash(hash);

    _pipelineIDDirty = false;
    return true;
}

bool Material::PipelineIDDirty() const {
    return _pipelineIDDirty;
}

auto Material::GetRenderGroup() const -> uint16_t {
    return _renderGroup;
}

auto Material::GetPipelineID() const -> uint16_t {
    return _pipelineID;
}
