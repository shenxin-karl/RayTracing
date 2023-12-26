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

class Material : NonCopyable {
public:
    enum TextureType { eAlbedoTex, eAmbientOcclusionTex, eEmissionTex, eMetalRoughnessTex, eNormalTex, eMaxNum };
    enum SamplerAddressMode { ePointWrap, ePointClamp, eLinearWrap, eLinearClamp, eAnisotropicWrap, eAnisotropicClamp };
public:
    Material();
    ~Material();
    void SetRenderGroup(uint16_t renderGroup);
    void SetTextures(TextureType textureType, std::shared_ptr<dx::Texture> pTexture, dx::SRV srv);
    void SetAlbedo(const glm::vec4 &albedo);
    void SetEmission(const glm::vec4 &emission);
    void SetTillingAndOffset(const glm::vec4 &tilingAndOffset);
    void SetCutoff(float cutoff);
    void SetRoughness(float roughness);
    void SetMetallic(float metallic);
    void SetNormalScale(float normalScale);
    void SetSamplerAddressMode(SamplerAddressMode mode);
    bool UpdatePipelineID(SemanticMask meshSemanticMask);
    bool PipelineIDDirty() const;
    auto GetRenderGroup() const -> uint16_t;
    auto GetPipelineID() const -> uint16_t;
private:
    struct alignas(sizeof(glm::vec4)) CbPreMaterial {
        glm::vec4 albedo;
        glm::vec4 emission;
        glm::vec4 tilingAndOffset;
        float cutoff;
        float roughness;
        float metallic;
        float normalScale;
        int samplerStateIndex;
        int albedoTexIndex;
        int ambientOcclusionTexIndex;
        int emissionTexIndex;
        int metalRoughnessTexIndex;
        int normalTexIndex;
        int padding0;
        int padding1;
    };
private:
    friend class ForwardPass;
    // clang-format off
    uint32_t                     _renderGroup;
    dx::DefineList               _defineList;
    dx::SRV                      _textureHandles[eMaxNum];
    std::shared_ptr<dx::Texture> _textures[eMaxNum];
    CbPreMaterial                _cbPreMaterial;

    bool                         _pipelineIDDirty;
    SemanticMask                 _pipelineSemanticMask;
    uint32_t                     _pipelineID;
    // clang-format on
};