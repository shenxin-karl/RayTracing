#pragma once
#include <memory>
#include <vector>
#include "D3d12/D3dStd.h"
#include "Foundation/GlmStd.hpp"
#include "Foundation/NonCopyable.h"
#include "Foundation/ReadonlyArraySpan.hpp"

enum class SemanticMask;
enum class SemanticIndex;
class CPUMeshData;
class GPUMeshData;

struct SubMesh {
	size_t vertexCount;
	size_t indexCount;
	size_t baseVertexLocation;
	size_t baseIndexLocation;
};

class Mesh : private NonCopyable {
public:
    Mesh();
    ~Mesh();
public:
	auto GetName() const -> std::string_view;
	auto GetVertexCount() const -> size_t;
	auto GetIndexCount() const -> size_t;
	auto GetSemanticMask() const -> SemanticMask;
	void GetVertices(std::vector<glm::vec3> &vertices) const;
	auto GetSubMeshes() const -> const std::vector<SubMesh> &;
	auto GetGPUMeshData() const -> const GPUMeshData *;
public:
	void SetName(std::string_view name);
	void SetIndices(ReadonlyArraySpan<uint32_t> indices);
	void SetIndices(ReadonlyArraySpan<int32_t> indices);
	void SetVertices(ReadonlyArraySpan<glm::vec3> vertices);
	void SetNormals(ReadonlyArraySpan<glm::vec3> normals);
	void SetTangents(ReadonlyArraySpan<glm::vec4> tangents);
	void SetColors(ReadonlyArraySpan<glm::vec4> colors);
	void SetUV0(ReadonlyArraySpan<glm::vec2> uvs);
	void SetSubMeshes(std::vector<SubMesh> subMeshes);
	void Resize(SemanticMask mask, size_t vertexCount, size_t indexCount);
	void UploadMeshData();
	auto GetBottomLevelAS() const -> dx::BottomLevelAS *;
	bool IsGpuDataDirty() const {
		return _vertexAttributeDirty;
	}
private:
	friend class SceneRayTracingASManager;
	void SetDataCheck(size_t vertexCount, SemanticIndex index) const;
	auto RequireBottomLevelAS(dx::IASBuilder *pASBuilder) -> dx::BottomLevelAS *;
private:
	// clang-format off
	std::string						_name;
	std::vector<SubMesh>			_subMeshes;
	std::unique_ptr<CPUMeshData>	_pCpuMeshData;
	std::unique_ptr<GPUMeshData>	_pGpuMeshData;
	bool							_vertexAttributeDirty;
	// clang-format on
};
