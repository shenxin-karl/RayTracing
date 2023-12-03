// clang-format off
#pragma once
#include <dxgiformat.h>
#include <string_view>
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
	eInvalid			=  0x8000,
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
	eMaskAll			=  1 << (static_cast<int>(SemanticIndex::eMaxNum) - 1) 
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
        {SemanticMask::eTexCoord0, DXGI_FORMAT_R32G32_FLOAT, 2, sizeof(float) * 2, "TEXCOORD0"},
        {SemanticMask::eTexCoord1, DXGI_FORMAT_R32G32_FLOAT, 2, sizeof(float) * 2, "TEXCOORD1"},
        {SemanticMask::eTexCoord2, DXGI_FORMAT_R32G32B32_FLOAT, 3, sizeof(float) * 3, "TEXCOORD2"},
        {SemanticMask::eTexCoord3, DXGI_FORMAT_R32G32B32_FLOAT, 3, sizeof(float) * 3, "TEXCOORD3"},
        {SemanticMask::eTexCoord4, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "TEXCOORD4"},
        {SemanticMask::eTexCoord5, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "TEXCOORD5"},
        {SemanticMask::eTexCoord6, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "TEXCOORD6"},
        {SemanticMask::eTexCoord7, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, sizeof(float) * 4, "TEXCOORD7"},
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
		if (HasFlag(mask, SemanticMaskCast(index))) {
			offset += GetSemanticInfo(index).dataSize;
		}
	}
	return offset;
}

// clang-format on