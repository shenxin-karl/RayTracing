#pragma once
#include <memory>
#include "Foundation/NonCopyable.h"
#include "D3d12/DescriptorHandle.h"
#include "D3d12/ShaderCompiler.h"
#include "D3d12/D3dUtils.h"

enum class SemanticMask;

namespace dx {
class Texture;
}

class StandardMaterial : NonCopyable {
public:
    enum TextureType { eAlbedoTex, eAmbientOcclusionTex, eEmissionTex, eMetalRoughnessTex, eNormalTex, eMaxNum };
    enum RenderMode { eOpaque, eAlphaTest, eTransparent };
    enum RenderFeatures {
        eEnableEmission,
        eEnableNormalScale
    };
public:
    StandardMaterial();
    ~StandardMaterial();
    void SetRenderMode(RenderMode renderMode);
    void SetRenderFeatures(RenderFeatures features, bool enable);
    void SetTextures(TextureType textureType, std::shared_ptr<dx::Texture> pTexture);
    void SetAlbedo(const glm::vec4 &albedo);
    void SetEmission(const glm::vec4 &emission);
    void SetTillingAndOffset(const glm::vec4 &tilingAndOffset);
    void SetCutoff(float cutoff);
    void SetRoughness(float roughness);
    void SetMetallic(float metallic);
    void SetNormalScale(float normalScale);
private:
    friend class StandardMaterialManager;
    // clang-format off
    SemanticMask                 _semanticMask;
	RenderMode                   _renderMode;
    dx::DefineList               _defineList;
    dx::SRV                      _textureHandles[eMaxNum];
    std::shared_ptr<dx::Texture> _textures[eMaxNum];

    glm::vec4                    _albedo;
    glm::vec4                    _emission;
    glm::vec4                    _tilingAndOffset;
    float                        _cutoff;
    float                        _roughness;
    float                        _metallic;
    float                        _normalScale;

    bool                                 _pipeStateDirty = false;
    std::shared_ptr<dx::RootSignature>   _pRootSignature;
    dx::WRL::ComPtr<ID3D12PipelineState> _pPipelineState;
    // clang-format on
};