#pragma once
#include "RenderObject/ConstantBufferHelper.h"

namespace dx {
class GraphicsContext;
}

// clang-format off

// global shader parameters
struct GlobalShaderParam {
    dx::GraphicsContext             *pGfxCtx                    = nullptr;
    const cbuffer::CbPrePass        *pCbPrePass                 = nullptr;
    const cbuffer::DirectionalLight *pDirectionalLight          = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS        cbPrePassCBuffer           = 0;
    D3D12_GPU_VIRTUAL_ADDRESS        cbDirectionalLightBuffer   = 0;
};

// clang-format on
