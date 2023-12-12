#pragma once
#include <memory>
#include <vector>
#include "Foundation/GlmStd.hpp"
#include "Foundation/NonCopyable.h"
#include "Foundation/ReadonlyArraySpan.hpp"

enum class SemanticMask;
enum class SemanticIndex;
class CPUMeshData;
class GPUMeshData;

class Mesh : private NonCopyable {
public:
    Mesh();
    ~Mesh();
public:
	auto GetVertexCount() const -> size_t;
	auto GetIndexCount() const -> size_t;
	auto GetSemanticMask() const -> SemanticMask;
	void GetVertices(std::vector<glm::vec3> &vertices) const;
public:
	void Resize(SemanticMask mask, size_t vertexCount, size_t indexCount);
	void SetVertices(ReadonlyArraySpan<glm::vec3> vertices);
	void SetNormals(ReadonlyArraySpan<glm::vec3> normals);
	void SetTangents(ReadonlyArraySpan<glm::vec4> tangents);
	void SetColors(ReadonlyArraySpan<glm::vec4> colors);
	void SetUV0(ReadonlyArraySpan<glm::vec2> uvs);
	void UploadMeshData(bool generateBottomLevelAS, bool isOpaque = true);
private:
	void SetDataCheck(size_t vertexCount, SemanticIndex index) const;
private:
	// clang-format off
	std::unique_ptr<CPUMeshData>	_pCpuMeshData;
	std::unique_ptr<GPUMeshData>	_pGpuMeshData;
	bool							_vertexAttributeDirty;
	bool							_bottomLevelASDirty;
	// clang-format on
};
