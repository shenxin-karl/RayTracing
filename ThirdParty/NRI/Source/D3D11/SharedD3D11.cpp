// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

using namespace nri;

constexpr std::array<D3D11_LOGIC_OP, (size_t)LogicFunc::MAX_NUM> LOGIC_FUNC_TABLE =
{
    D3D11_LOGIC_OP_CLEAR,                               // NONE,
    D3D11_LOGIC_OP_CLEAR,                               // CLEAR,
    D3D11_LOGIC_OP_AND,                                 // AND,
    D3D11_LOGIC_OP_AND_REVERSE,                         // AND_REVERSE,
    D3D11_LOGIC_OP_COPY,                                // COPY,
    D3D11_LOGIC_OP_AND_INVERTED,                        // AND_INVERTED,
    D3D11_LOGIC_OP_XOR,                                 // XOR,
    D3D11_LOGIC_OP_OR,                                  // OR,
    D3D11_LOGIC_OP_NOR,                                 // NOR,
    D3D11_LOGIC_OP_EQUIV,                               // EQUIVALENT,
    D3D11_LOGIC_OP_INVERT,                              // INVERT,
    D3D11_LOGIC_OP_OR_REVERSE,                          // OR_REVERSE,
    D3D11_LOGIC_OP_COPY_INVERTED,                       // COPY_INVERTED,
    D3D11_LOGIC_OP_OR_INVERTED,                         // OR_INVERTED,
    D3D11_LOGIC_OP_NAND,                                // NAND,
    D3D11_LOGIC_OP_SET                                  // SET
};

D3D11_LOGIC_OP nri::GetD3D11LogicOpFromLogicFunc(LogicFunc logicalFunc)
{
    return LOGIC_FUNC_TABLE[(size_t)logicalFunc];
}

constexpr std::array<D3D11_BLEND_OP, (size_t)BlendFunc::MAX_NUM> BLEND_FUNC_TABLE =
{
    D3D11_BLEND_OP_ADD,                                 // ADD,
    D3D11_BLEND_OP_SUBTRACT,                            // SUBTRACT,
    D3D11_BLEND_OP_REV_SUBTRACT,                        // REVERSE_SUBTRACT,
    D3D11_BLEND_OP_MIN,                                 // MIN,
    D3D11_BLEND_OP_MAX                                  // MAX
};

D3D11_BLEND_OP nri::GetD3D11BlendOpFromBlendFunc(BlendFunc blendFunc)
{
    return BLEND_FUNC_TABLE[(size_t)blendFunc];
}

constexpr std::array<D3D11_BLEND, (size_t)BlendFactor::MAX_NUM> BLEND_FACTOR_TABLE =
{
    D3D11_BLEND_ZERO,                                   // ZERO,
    D3D11_BLEND_ONE,                                    // ONE,
    D3D11_BLEND_SRC_COLOR,                              // SRC_COLOR,
    D3D11_BLEND_INV_SRC_COLOR,                          // ONE_MINUS_SRC_COLOR,
    D3D11_BLEND_DEST_COLOR,                             // DST_COLOR,
    D3D11_BLEND_INV_DEST_COLOR,                         // ONE_MINUS_DST_COLOR,
    D3D11_BLEND_SRC_ALPHA,                              // SRC_ALPHA,
    D3D11_BLEND_INV_SRC_ALPHA,                          // ONE_MINUS_SRC_ALPHA,
    D3D11_BLEND_DEST_ALPHA,                             // DST_ALPHA,
    D3D11_BLEND_INV_DEST_ALPHA,                         // ONE_MINUS_DST_ALPHA,
    D3D11_BLEND_BLEND_FACTOR,                           // CONSTANT_COLOR,
    D3D11_BLEND_INV_BLEND_FACTOR,                       // ONE_MINUS_CONSTANT_COLOR,
    D3D11_BLEND_BLEND_FACTOR,                           // CONSTANT_ALPHA,
    D3D11_BLEND_INV_BLEND_FACTOR,                       // ONE_MINUS_CONSTANT_ALPHA,
    D3D11_BLEND_SRC_ALPHA_SAT,                          // SRC_ALPHA_SATURATE,
    D3D11_BLEND_SRC1_COLOR,                             // SRC1_COLOR,
    D3D11_BLEND_INV_SRC1_COLOR,                         // ONE_MINUS_SRC1_COLOR,
    D3D11_BLEND_SRC1_ALPHA,                             // SRC1_ALPHA,
    D3D11_BLEND_INV_SRC1_ALPHA                          // ONE_MINUS_SRC1_ALPHA,
};

D3D11_BLEND nri::GetD3D11BlendFromBlendFactor(BlendFactor blendFactor)
{
    return BLEND_FACTOR_TABLE[(size_t)blendFactor];
}

constexpr std::array<D3D11_STENCIL_OP, (size_t)StencilFunc::MAX_NUM> STENCIL_FUNC_TABLE =
{
    D3D11_STENCIL_OP_KEEP,                              // KEEP,
    D3D11_STENCIL_OP_ZERO,                              // ZERO,
    D3D11_STENCIL_OP_REPLACE,                           // REPLACE,
    D3D11_STENCIL_OP_INCR_SAT,                          // INCREMENT_AND_CLAMP,
    D3D11_STENCIL_OP_DECR_SAT,                          // DECREMENT_AND_CLAMP,
    D3D11_STENCIL_OP_INVERT,                            // INVERT,
    D3D11_STENCIL_OP_INCR,                              // INCREMENT_AND_WRAP,
    D3D11_STENCIL_OP_DECR                               // DECREMENT_AND_WRAP
};

D3D11_STENCIL_OP nri::GetD3D11StencilOpFromStencilFunc(StencilFunc stencilFunc)
{
    return STENCIL_FUNC_TABLE[(size_t)stencilFunc];
}

constexpr std::array<D3D11_COMPARISON_FUNC, (size_t)CompareFunc::MAX_NUM> COMPARE_FUNC_TABLE =
{
    D3D11_COMPARISON_ALWAYS,                            // NONE,
    D3D11_COMPARISON_ALWAYS,                            // ALWAYS,
    D3D11_COMPARISON_NEVER,                             // NEVER,
    D3D11_COMPARISON_LESS,                              // LESS,
    D3D11_COMPARISON_LESS_EQUAL,                        // LESS_EQUAL,
    D3D11_COMPARISON_EQUAL,                             // EQUAL,
    D3D11_COMPARISON_GREATER_EQUAL,                     // GREATER_EQUAL,
    D3D11_COMPARISON_GREATER                            // GREATER
};

D3D11_COMPARISON_FUNC nri::GetD3D11ComparisonFuncFromCompareFunc(CompareFunc compareFunc)
{
    return COMPARE_FUNC_TABLE[(size_t)compareFunc];
}

constexpr std::array<D3D11_CULL_MODE, (size_t)CullMode::MAX_NUM> CULL_MODE_TABLE =
{
    D3D11_CULL_NONE,                                    // NONE,
    D3D11_CULL_FRONT,                                   // FRONT,
    D3D11_CULL_BACK                                     // BACK
};

D3D11_CULL_MODE nri::GetD3D11CullModeFromCullMode(CullMode cullMode)
{
    return CULL_MODE_TABLE[(size_t)cullMode];
}

constexpr std::array<uint32_t, (size_t)Topology::MAX_NUM> TOPOLOGY_TABLE =
{
    D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,                  // POINT_LIST,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST,                   // LINE_LIST,
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,                  // LINE_STRIP,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,               // TRIANGLE_LIST,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,              // TRIANGLE_STRIP,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,               // LINE_LIST_WITH_ADJACENCY,
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,              // LINE_STRIP_WITH_ADJACENCY,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ,           // TRIANGLE_LIST_WITH_ADJACENCY,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ,          // TRIANGLE_STRIP_WITH_ADJACENCY,
    D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST   // PATCH_LIST
};

D3D11_PRIMITIVE_TOPOLOGY nri::GetD3D11TopologyFromTopology(Topology topology, uint32_t patchPoints)
{
    uint32_t res = TOPOLOGY_TABLE[(size_t)topology];

    if (res == D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST)
        res += patchPoints - 1;

    return (D3D11_PRIMITIVE_TOPOLOGY)res;
}

bool nri::GetTextureDesc(const TextureD3D11Desc& textureD3D11Desc, TextureDesc& textureDesc)
{
    textureDesc = {};

    ID3D11Resource* resource = textureD3D11Desc.d3d11Resource;
    if (!resource)
        return false;

    D3D11_RESOURCE_DIMENSION type;
    resource->GetType(&type);

    if (type < D3D11_RESOURCE_DIMENSION_TEXTURE1D)
        return false;

    uint32_t bindFlags = 0;
    if (type == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
    {
        ID3D11Texture1D* texture = (ID3D11Texture1D*)resource;
        D3D11_TEXTURE1D_DESC desc = {};
        texture->GetDesc(&desc);

        textureDesc.width = (Dim_t)desc.Width;
        textureDesc.height = 1;
        textureDesc.depth = 1;
        textureDesc.mipNum = (Mip_t)desc.MipLevels;
        textureDesc.arraySize = (Dim_t)desc.ArraySize;
        textureDesc.sampleNum = 1;
        textureDesc.type = TextureType::TEXTURE_1D;
        textureDesc.format = DXGIFormatToNRIFormat(desc.Format);

        bindFlags = desc.BindFlags;
    }
    else if (type == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
        ID3D11Texture2D* texture = (ID3D11Texture2D*)resource;
        D3D11_TEXTURE2D_DESC desc = {};
        texture->GetDesc(&desc);

        textureDesc.width = (Dim_t)desc.Width;
        textureDesc.height = (Dim_t)desc.Height;
        textureDesc.depth = 1;
        textureDesc.mipNum = (Mip_t)desc.MipLevels;
        textureDesc.arraySize = (Dim_t)desc.ArraySize;
        textureDesc.sampleNum = (Sample_t)desc.SampleDesc.Count;
        textureDesc.type = TextureType::TEXTURE_2D;
        textureDesc.format = DXGIFormatToNRIFormat(desc.Format);

        bindFlags = desc.BindFlags;
    }
    else if (type == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
    {
        ID3D11Texture3D* texture = (ID3D11Texture3D*)resource;
        D3D11_TEXTURE3D_DESC desc = {};
        texture->GetDesc(&desc);

        textureDesc.width = (Dim_t)desc.Width;
        textureDesc.height = (Dim_t)desc.Height;
        textureDesc.depth = (Dim_t)desc.Depth;
        textureDesc.mipNum = (Mip_t)desc.MipLevels;
        textureDesc.arraySize = 1;
        textureDesc.sampleNum = 1;
        textureDesc.type = TextureType::TEXTURE_3D;
        textureDesc.format = DXGIFormatToNRIFormat(desc.Format);

        bindFlags = desc.BindFlags;
    }

    if (bindFlags & D3D11_BIND_RENDER_TARGET)
        textureDesc.usageMask |= TextureUsageBits::COLOR_ATTACHMENT;
    if (bindFlags & D3D11_BIND_DEPTH_STENCIL)
        textureDesc.usageMask |= TextureUsageBits::DEPTH_STENCIL_ATTACHMENT;
    if (bindFlags & D3D11_BIND_SHADER_RESOURCE)
        textureDesc.usageMask |= TextureUsageBits::SHADER_RESOURCE;
    if (bindFlags & D3D11_BIND_UNORDERED_ACCESS)
        textureDesc.usageMask |= TextureUsageBits::SHADER_RESOURCE_STORAGE;

    return true;
}

bool nri::GetBufferDesc(const BufferD3D11Desc& bufferD3D11Desc, BufferDesc& bufferDesc)
{
    bufferDesc = {};

    ID3D11Resource* resource = bufferD3D11Desc.d3d11Resource;
    if (!resource)
        return false;

    D3D11_RESOURCE_DIMENSION type;
    resource->GetType(&type);

    if (type != D3D11_RESOURCE_DIMENSION_BUFFER)
        return false;

    ID3D11Buffer* buffer = (ID3D11Buffer*)resource;
    D3D11_BUFFER_DESC desc = {};
    buffer->GetDesc(&desc);

    bufferDesc.size = desc.ByteWidth;
    bufferDesc.structureStride = desc.StructureByteStride;

    if (desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
        bufferDesc.usageMask |= BufferUsageBits::VERTEX_BUFFER;
    if (desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
        bufferDesc.usageMask |= BufferUsageBits::INDEX_BUFFER;
    if (desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
        bufferDesc.usageMask |= BufferUsageBits::CONSTANT_BUFFER;
    if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
        bufferDesc.usageMask |= BufferUsageBits::SHADER_RESOURCE;
    if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
        bufferDesc.usageMask |= BufferUsageBits::SHADER_RESOURCE_STORAGE;
    if (desc.MiscFlags & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
        bufferDesc.usageMask |= BufferUsageBits::ARGUMENT_BUFFER;

    return true;
}
