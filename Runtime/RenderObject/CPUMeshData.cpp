#include "CPUMeshData.h"

CPUMeshData::CPUMeshData(): _semanticMask(SemanticMask::eNothing), _vertexCount(0), _indexCount(0) {
}

CPUMeshData::~CPUMeshData() {
}

auto CPUMeshData::GetSemanticBegin(SemanticIndex index) -> StrideIterator {
	size_t offset = GetSemanticOffset(_semanticMask, index);
	size_t stride = GetSemanticStride(_semanticMask);
	size_t dataSize = GetSemanticInfo(index).dataSize;
	return StrideIterator(_pVertices.get() + offset, stride, dataSize);
}

auto CPUMeshData::GetSemanticEnd(SemanticIndex index) -> StrideIterator {
	return GetSemanticBegin(index) + _vertexCount;
}

void CPUMeshData::Resize(SemanticMask mask, size_t vertexCount, size_t indexCount) {
	if (_semanticMask != mask || _vertexCount != vertexCount) {
		_semanticMask = mask;
		_vertexCount = vertexCount;
		size_t vertexStride = GetSemanticStride(mask);
		_pVertices = std::make_unique<int8_t[]>(vertexStride * vertexCount);
		std::memset(_pVertices.get(), 0, vertexStride * vertexCount);
	}
	if (_indexCount != indexCount) {
		_pIndices = std::make_unique<int16_t[]>(indexCount);
		std::memset(_pVertices.get(), 0, sizeof(int16_t) * indexCount);
	}
}
