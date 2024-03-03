#include "Mesh.h"
#include "GPUMeshData.h"
#include "CPUMeshData.h"
#include "Foundation/DebugBreak.h"
#include "Foundation/Logger.h"

Mesh::Mesh() : _vertexAttributeDirty(false) {
	_pCpuMeshData = std::make_unique<CPUMeshData>();
	_pGpuMeshData = std::make_unique<GPUMeshData>();
}

Mesh::~Mesh() {
}

auto Mesh::GetName() const -> std::string_view {
    return _name;
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

auto Mesh::GetSubMeshes() const -> const std::vector<SubMesh> & {
    return _subMeshes;
}

auto Mesh::GetGPUMeshData() const -> const GPUMeshData * {
    return _pGpuMeshData.get();
}

void Mesh::SetName(std::string_view name) {
    _name = name;
    _pGpuMeshData->SetName(name);
}

void Mesh::SetIndices(ReadonlyArraySpan<uint32_t> indices) {
    Assert(indices.Count() == _pCpuMeshData->GetIndexCount());
    auto ptr = _pCpuMeshData->GetIndicesBegin();
    std::memcpy(ptr, indices.Data(), sizeof(uint16_t) * indices.Count());
}

void Mesh::SetIndices(ReadonlyArraySpan<int32_t> indices) {
    Assert(indices.Count() == _pCpuMeshData->GetIndexCount());
    auto ptr = _pCpuMeshData->GetIndicesBegin();
    std::memcpy(ptr, indices.Data(), sizeof(uint16_t) * indices.Count());
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
    _vertexAttributeDirty = true;
}

void Mesh::SetNormals(ReadonlyArraySpan<glm::vec3> normals) {
    SetDataCheck(normals.Count(), SemanticIndex::eNormal);
    auto begin = _pCpuMeshData->GetSemanticBegin(SemanticIndex::eNormal);
    auto end = _pCpuMeshData->GetSemanticEnd(SemanticIndex::eNormal);
    fill(begin, end, normals);
    _vertexAttributeDirty = true;
}

void Mesh::SetTangents(ReadonlyArraySpan<glm::vec4> tangents) {
    SetDataCheck(tangents.Count(), SemanticIndex::eTangent);
    auto begin = _pCpuMeshData->GetSemanticBegin(SemanticIndex::eTangent);
    auto end = _pCpuMeshData->GetSemanticEnd(SemanticIndex::eTangent);
    fill(begin, end, tangents);
    _vertexAttributeDirty = true;
}

void Mesh::SetColors(ReadonlyArraySpan<glm::vec4> colors) {
    SetDataCheck(colors.Count(), SemanticIndex::eColor);
    auto begin = _pCpuMeshData->GetSemanticBegin(SemanticIndex::eColor);
    auto end = _pCpuMeshData->GetSemanticEnd(SemanticIndex::eColor);
    fill(begin, end, colors);
    _vertexAttributeDirty = true;
}

void Mesh::SetUV0(ReadonlyArraySpan<glm::vec2> uvs) {
    SetDataCheck(uvs.Count(), SemanticIndex::eTexCoord0);
    auto begin = _pCpuMeshData->GetSemanticBegin(SemanticIndex::eTexCoord0);
    auto end = _pCpuMeshData->GetSemanticEnd(SemanticIndex::eTexCoord0);
    fill(begin, end, uvs);
    _vertexAttributeDirty = true;
}

void Mesh::SetSubMeshes(std::vector<SubMesh> subMeshes) {
    _subMeshes = std::move(subMeshes);
}

void Mesh::Resize(SemanticMask mask, size_t vertexCount, size_t indexCount) {
    _pCpuMeshData->Resize(mask, vertexCount, indexCount);
    _vertexAttributeDirty = true;
    _subMeshes.clear();
}

void Mesh::UploadMeshData() {
    if (_vertexAttributeDirty) {
		_pGpuMeshData->UploadGpuMemory(_pCpuMeshData.get());
		_vertexAttributeDirty = false;
    }
    if (_subMeshes.empty()) {
	    SubMesh subMesh;
        subMesh.vertexCount = _pCpuMeshData->GetVertexCount();
        subMesh.indexCount = _pCpuMeshData->GetIndexCount();
        subMesh.baseIndexLocation = 0;
        subMesh.baseVertexLocation = 0;
        _subMeshes.push_back(subMesh);
    }
    _pGpuMeshData->SetName(_name);
}

auto Mesh::GetBottomLevelAS() const -> dx::BottomLevelAS * {
    return _pGpuMeshData->GetBottomLevelAS();
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

auto Mesh::RequireBottomLevelAS(dx::IASBuilder *pASBuilder) -> dx::BottomLevelAS * {
    if (_vertexAttributeDirty) {
        DEBUG_BREAK;
        Logger::Error("Mesh::RequireBottomLevelAS, vertexAttribute Dirty!");
	    return nullptr;
    }
    return _pGpuMeshData->RequireBottomLevelAS(pASBuilder);
}
