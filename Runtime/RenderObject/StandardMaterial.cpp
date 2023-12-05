#include "StandardMaterial.h"
#include "Foundation/ColorUtil.hpp"

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

}

StandardMaterial::StandardMaterial()
    : _renderMode(eOpaque),
      _albedo(Colors::White),
      _emission(Colors::Black),
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
