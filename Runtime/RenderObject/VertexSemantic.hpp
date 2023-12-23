// clang-format off
#pragma once
#include <d3d12.h>
#include <dxgiformat.h>
#include <string_view>

#include "D3d12/d3dx12.h"
#include "Foundation/Exception.h"
#include "Foundation/PreprocessorDirectives.h"

enum class SemanticIndex {
    eVertex			= 0,	// Vertex			(float3)
	eNormal			= 1,	// Normal			(float3)
	eTangent		= 2,	// Tangent			(float4)
	eColor			= 3,	// Vertex color		(float4)
	eTexCoord0		= 4,	// Texcoord 0		(float2)
	eTexCoord1		= 5,	// Texcoord 1		(float2)
	eTexCoord2		= 6,	// Texcoord 2		(float3)
	eTexCoord3		= 7,	// Texcoord 3		(float3)
	eTexCoord4		= 8,	// Texcoord 4		(float4)
	eTexCoord5		= 9,	// Texcoord 5		(float4)
	eTexCoord6		= 10,	// Texcoord 6		(float4)
	eTexCoord7		= 11,	// Texcoord 7		(float4)
	eBlendWeights	= 12,	// Blend weights	(float4)
	eBlendIndices	= 13,	// Blend indices	(int8_4)
	eMaxNum
};
ENUM_INCREMENT(SemanticIndex);

enum class SemanticMask {
	eNothing			=  0,
	eVertex				=  1 << static_cast<int>(SemanticIndex::eVertex),
	eNormal				=  1 << static_cast<int>(SemanticIndex::eNormal),
	eTangent			=  1 << static_cast<int>(SemanticIndex::eTangent),
	eColor				=  1 << static_cast<int>(SemanticIndex::eColor),
	eTexCoord0			=  1 << static_cast<int>(SemanticIndex::eTexCoord0),
	eTexCoord1			=  1 << static_cast<int>(SemanticIndex::eTexCoord1),
	eTexCoord2			=  1 << static_cast<int>(SemanticIndex::eTexCoord2),
	eTexCoord3			=  1 << static_cast<int>(SemanticIndex::eTexCoord3),
	eTexCoord4			=  1 << static_cast<int>(SemanticIndex::eTexCoord4),
	eTexCoord5			=  1 << static_cast<int>(SemanticIndex::eTexCoord5),
	eTexCoord6			=  1 << static_cast<int>(SemanticIndex::eTexCoord6),
	eTexCoord7			=  1 << static_cast<int>(SemanticIndex::eTexCoord7),
	eBlendWeights		=  1 << static_cast<int>(SemanticIndex::eBlendWeights),
	eBlendIndices		=  1 << static_cast<int>(SemanticIndex::eBlendIndices),
};
ENUM_FLAGS(SemanticMask);

constexpr SemanticMask SemanticMaskCast(SemanticIndex index) {
	return static_cast<SemanticMask>(1 << static_cast<size_t>(index));
}

struct VertexSemantic {
public:
    constexpr VertexSemantic(SemanticMask mask, DXGI_FORMAT format, size_t fieldCount, size_t dataSize, std::string_view semantic)
        : mask(mask), format(format), fieldCount(fieldCount), dataSize(dataSize), semantic(semantic) {
    }
    constexpr VertexSemantic(const VertexSemantic &) = default;
	constexpr VertexSemantic &operator=(const VertexSemantic &) = default;
	constexpr friend std::strong_ordering operator<=>(const VertexSemantic &, const VertexSemantic &) = default;
public:
    SemanticMask      mask;
	DXGI_FORMAT		  format;
    int8_t            fieldCount;
    int8_t            dataSize;
    std::string_view  semantic;
};

// clang-format on
constexpr VertexSemantic GetSemanticInfo(SemanticIndex index) {
    constexpr VertexSemantic kVertexSemantic[] = {
        {SemanticMask::eVertex, DXGI_FORMAT_R32G32B32_FLOAT, 3, sizeof(float) * 3, "POSITION"},
        {SemanticMask::eNormal, DXGI_FORMAT_R32G32B32_FLOAT, 3, sizeof(float) * 3, "NORMAL"},
        {SemanticMask::eTangent, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "TANGENT"},
        {SemanticMask::eColor, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "COLOR"},
        {SemanticMask::eTexCoord0, DXGI_FORMAT_R32G32_FLOAT, 2, sizeof(float) * 2, "TEXCOORD"},
        {SemanticMask::eTexCoord1, DXGI_FORMAT_R32G32_FLOAT, 2, sizeof(float) * 2, "TEXCOORD"},
        {SemanticMask::eTexCoord2, DXGI_FORMAT_R32G32B32_FLOAT, 3, sizeof(float) * 3, "TEXCOORD"},
        {SemanticMask::eTexCoord3, DXGI_FORMAT_R32G32B32_FLOAT, 3, sizeof(float) * 3, "TEXCOORD"},
        {SemanticMask::eTexCoord4, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "TEXCOORD"},
        {SemanticMask::eTexCoord5, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "TEXCOORD"},
        {SemanticMask::eTexCoord6, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "TEXCOORD"},
        {SemanticMask::eTexCoord7, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "TEXCOORD"},
        {SemanticMask::eBlendWeights, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "BLEND_WEIGHTS"},
        {SemanticMask::eBlendIndices, DXGI_FORMAT_R8G8B8A8_UINT, 4, sizeof(uint8_t) * 4, "BLEND_INDICES"},
    };
    return kVertexSemantic[static_cast<size_t>(index)];
}
// clang-format off

constexpr size_t GetSemanticStride(SemanticMask mask) {
	size_t stride = 0;
	for (SemanticIndex index = SemanticIndex::eVertex; index != SemanticIndex::eMaxNum; ++index) {
		if (HasFlag(mask, SemanticMaskCast(index))) {
			stride += GetSemanticInfo(index).dataSize;
		}
	}
	return stride;
}

constexpr size_t GetSemanticOffset(SemanticMask mask, SemanticIndex index) {
	if (!HasFlag(mask, SemanticMaskCast(index))) {
		return 0;
	}

	size_t offset = 0;
	for (SemanticIndex i = SemanticIndex::eVertex; i != index; ++i) {
		if (HasFlag(mask, SemanticMaskCast(i))) {
			offset += GetSemanticInfo(i).dataSize;
		}
	}
	return offset;
}

inline std::vector<D3D12_INPUT_ELEMENT_DESC> SemanticMaskToVertexInputElements(SemanticMask meshMask, SemanticMask expectMask) {
	Assert(meshMask != SemanticMask::eNothing);
	Assert(HasAllFlags(meshMask, expectMask));

	UINT alignedByteOffset = 0;
	std::vector<D3D12_INPUT_ELEMENT_DESC> descList;
	for (SemanticIndex index = SemanticIndex::eVertex; index != SemanticIndex::eMaxNum; ++index) {
		if (HasFlag(meshMask, SemanticMaskCast(index))) {
			VertexSemantic info = GetSemanticInfo(index);
			if (HasFlag(expectMask, SemanticMaskCast(index))) {
				D3D12_INPUT_ELEMENT_DESC desc = {};
				desc.SemanticName = info.semantic.data();
				desc.Format = info.format;
				desc.InputSlot = 0;
				desc.AlignedByteOffset = alignedByteOffset;
				desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
				desc.InstanceDataStepRate = 0;

				switch (index) {
				case SemanticIndex::eTexCoord0:
				case SemanticIndex::eTexCoord1:
				case SemanticIndex::eTexCoord2:
				case SemanticIndex::eTexCoord3:
				case SemanticIndex::eTexCoord4:
				case SemanticIndex::eTexCoord5:
				case SemanticIndex::eTexCoord6:
				case SemanticIndex::eTexCoord7:
					desc.SemanticIndex = static_cast<size_t>(index) - static_cast<size_t>(SemanticIndex::eTexCoord0);
					break;
				default: 
					desc.SemanticIndex = 0;
				}
				descList.push_back(desc);
			}
			alignedByteOffset += info.dataSize;
		}
	}
	return descList;
}

// clang-format on