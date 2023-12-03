#include "Mesh.h"
#include "GPUMeshData.h"
#include "CPUMeshData.h"

Mesh::Mesh() : _gpuMeshDataDirty(false) {
    _pCpuMeshData = std::make_unique<CPUMeshData>();
    _pGpuMeshData = std::make_unique<GPUMeshData>();
}

Mesh::~Mesh() {
}

auto Mesh::GetVertexCount() const -> size_t {
    return _pCpuMeshData->GetVertexCount();
}

auto Mesh::GetIndexCount() const -> size_t {
    return _pCpuMeshData->GetIndexCount();
}

auto Mesh::GetSemanticMask() const -> SemanticMask {
    return _pCpuMeshData->GetSemanticMask();
}

void Mesh::GetVertices(std::vector<glm::vec3> &vertices) const {
    auto iter = _pCpuMeshData->GetSemanticBegin(SemanticIndex::eVertex);
    auto end = _pCpuMeshData->GetSemanticEnd(SemanticIndex::eVertex);
    while (iter != end) {
        vertices.push_back(iter.Get<glm::vec3>());
        ++iter;
    }
}

void Mesh::Resize(SemanticMask mask, size_t vertexCount, size_t indexCount) {
    _pCpuMeshData->Resize(mask, vertexCount, indexCount);
    _gpuMeshDataDirty = true;
}

template<typename T>
static void fill(CPUMeshData::StrideIterator begin, CPUMeshData::StrideIterator end, ReadonlyArraySpan<T> data) {
    size_t index = 0;
    while (begin != end) {
        begin.Set<T>(data[index]);
        ++index;
        ++begin;
    }
}

void Mesh::SetVertices(ReadonlyArraySpan<glm::vec3> vertices) {
    SetDataCheck(vertices.Count(), SemanticIndex::eVertex);
    auto begin = _pCpuMeshData->GetSemanticBegin(SemanticIndex::eVertex);
    auto end = _pCpuMeshData->GetSemanticEnd(SemanticIndex::eVertex);
    fill(begin, end, vertices);
    _gpuMeshDataDirty = true;
}

void Mesh::SetNormals(ReadonlyArraySpan<glm::vec3> normals) {
    SetDataCheck(normals.Count(), SemanticIndex::eNormal);
    auto begin = _pCpuMeshData->GetSemanticBegin(SemanticIndex::eNormal);
    auto end = _pCpuMeshData->GetSemanticEnd(SemanticIndex::eNormal);
    fill(begin, end, normals);
    _gpuMeshDataDirty = true;
}

void Mesh::SetTangents(ReadonlyArraySpan<glm::vec4> tangents) {
    SetDataCheck(tangents.Count(), SemanticIndex::eTangent);
    auto begin = _pCpuMeshData->GetSemanticBegin(SemanticIndex::eTangent);
    auto end = _pCpuMeshData->GetSemanticEnd(SemanticIndex::eTangent);
    fill(begin, end, tangents);
    _gpuMeshDataDirty = true;
}

void Mesh::SetColors(ReadonlyArraySpan<glm::vec4> colors) {
    SetDataCheck(colors.Count(), SemanticIndex::eColor);
    auto begin = _pCpuMeshData->GetSemanticBegin(SemanticIndex::eColor);
    auto end = _pCpuMeshData->GetSemanticEnd(SemanticIndex::eColor);
    fill(begin, end, colors);
    _gpuMeshDataDirty = true;
}

void Mesh::SetUV0(ReadonlyArraySpan<glm::vec2> uvs) {
    SetDataCheck(uvs.Count(), SemanticIndex::eTexCoord0);
    auto begin = _pCpuMeshData->GetSemanticBegin(SemanticIndex::eTexCoord0);
    auto end = _pCpuMeshData->GetSemanticEnd(SemanticIndex::eTexCoord0);
    fill(begin, end, uvs);
    _gpuMeshDataDirty = true;
}

void Mesh::UploadMeshData() {
    if (!_gpuMeshDataDirty) {
        return;
    }
    _pGpuMeshData->UploadGpuMemory(_pCpuMeshData.get());
    _gpuMeshDataDirty = false;
}

void Mesh::SetDataCheck(size_t vertexCount, SemanticIndex index) const {
    if (vertexCount != _pCpuMeshData->GetVertexCount()) {
        Exception::Throw("The number of vertices does not match");
    }
    if (!HasFlag(_pCpuMeshData->GetSemanticMask(), SemanticMaskCast(index))) {
        VertexSemantic semanticInfo = GetSemanticInfo(index);
        Exception::Throw("This semantic channel '{}' does not exist", semanticInfo.semantic);
    }
}
