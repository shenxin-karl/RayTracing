#include "StandardMaterial.h"
#include "VertexSemantic.hpp"
#include "D3d12/Device.h"
#include "D3d12/RootSignature.h"
#include "D3d12/Texture.h"
#include "Foundation/ColorUtil.hpp"
#include "Renderer/GfxDevice.h"
#include "ShaderLoader/ShaderManager.h"
#include "Utils/AssetProjectSetting.h"
#include "Utils/GlobalCallbacks.h"

namespace ShaderFeatures {

static const char *sEnableAlphaTest = "ENABLE_ALPHA_TEST";

static const char *sTextureKeyword[] = {
    "ENABLE_ALBEDO_TEXTURE",
    "ENABLE_AMBIENT_OCCLUSION_TEXTURE",
    "ENABLE_EMISSION_TEXTURE",
    "ENABLE_METAL_ROUGHNESS_TEXTURE",
    "ENABLE_NORMAL_TEX",
};

}    // namespace ShaderFeatures

class StandardMaterialDataManager {
public:
    StandardMaterialDataManager() {
        _createCallbackHandle = GlobalCallbacks::Get().onCreate.Register(this, &StandardMaterialDataManager::OnCreate);
        _destroyCallbackHandle = GlobalCallbacks::Get().onDestroy.Register(this, &StandardMaterialDataManager::OnDestroy);
    }
    void OnCreate() {
        _pRootSignature = std::make_unique<dx::RootSignature>();
        _pRootSignature->OnCreate(5, 6);
        _pRootSignature->At(0).InitAsBufferCBV(0);    // gCbPrePass;
        _pRootSignature->At(1).InitAsBufferCBV(1);    // gCbPreObject;
        _pRootSignature->At(2).InitAsBufferCBV(3);    // gCbLighting;
        _pRootSignature->At(3).InitAsBufferCBV(2);    // gCbMaterial;

        CD3DX12_DESCRIPTOR_RANGE1 range = {
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            static_cast<UINT>(-1),
            0,
            0,
        	D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
        };
        
        _pRootSignature->At(4).InitAsDescriptorTable({range});  // gTextureList

        D3D12_STATIC_SAMPLER_DESC samplers[6] = {
	        dx::GetPointWrapStaticSampler(0),
            dx::GetPointClampStaticSampler(1),
            dx::GetLinearWrapStaticSampler(2),
            dx::GetLinearClampStaticSampler(3),
            dx::GetAnisotropicWrapStaticSampler(4),
            dx::GetAnisotropicClampStaticSampler(5)
        };
        _pRootSignature->SetStaticSamplers(samplers);
        _pRootSignature->Generate(GfxDevice::GetInstance()->GetDevice());
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

    auto GetTextureSRV(dx::Texture *pTexture) -> dx::SRV {
	    if (auto it = _textureSRVMap.find(pTexture); it != _textureSRVMap.end()) {
		    return it->second;
	    }

        GfxDevice *pGfxDevice = GfxDevice::GetInstance();
        dx::Device *pDevice = pGfxDevice->GetDevice();

        dx::SRV srv = pDevice->AllocDescriptor<dx::SRV>(1);
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = pTexture->GetFormat();
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MostDetailedMip = 0;
        desc.Texture2D.MipLevels = pTexture->GetMipCount();
        desc.Texture2D.PlaneSlice = 0;
        desc.Texture2D.ResourceMinLODClamp = 0.f;
        pDevice->GetNativeDevice()->CreateShaderResourceView(pTexture->GetResource(), &desc, srv.GetCpuHandle());
        _textureSRVMap[pTexture] = srv;
        return srv;
    }
private:
    using PipelineStateMap = std::unordered_map<size_t, dx::WRL::ComPtr<ID3D12PipelineState>>;
    using TextureSRVMap = std::unordered_map<dx::Texture *, dx::SRV>;
    // clang-format off
    CallbackHandle                      _createCallbackHandle;
    CallbackHandle                      _destroyCallbackHandle;
    PipelineStateMap                    _pipelineStateMap;
    TextureSRVMap                       _textureSRVMap;
    std::shared_ptr<dx::RootSignature>  _pRootSignature;
    // clang-format on
};

static StandardMaterialDataManager gMaterialManager = {};

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

void StandardMaterial::SetTextures(TextureType textureType, std::shared_ptr<dx::Texture> pTexture) {
    _textures[textureType] = std::move(pTexture);
    _defineList.Set(ShaderFeatures::sTextureKeyword[textureType], _textures[textureType] != nullptr);
    _pipeStateDirty = true;

    _textureHandles[textureType] = dx::SRV{};
    if (_textures[textureType] != nullptr) {
	    _textureHandles[textureType] = gMaterialManager.GetTextureSRV(_textures[textureType].get());
    } 
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
