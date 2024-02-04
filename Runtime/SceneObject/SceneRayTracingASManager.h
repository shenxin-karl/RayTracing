#pragma once
#include "RayTracingGeometry.h"
#include "D3d12/D3dStd.h"
#include "Foundation/NonCopyable.h"
#include "Foundation/Memory/SharedPtr.hpp"

class MeshRenderer;
class SceneRayTracingASManager : private NonCopyable {
public:
    SceneRayTracingASManager();
    ~SceneRayTracingASManager();
    void AddMeshRenderer(MeshRenderer *pMeshRenderer);
    void RemoveMeshRenderer(MeshRenderer *pMeshRenderer);
    void OnPreRender();

    void BeginBuildBottomLevelAS();
    auto BuildMeshBottomLevelAS() -> std::shared_ptr<RegionTopLevelAS>;
    void EndBuildBottomLevelAS();

    void BuildTopLevelAS(std::shared_ptr<RegionTopLevelAS> pRegionTopLevelAS);

    auto GetTopLevelAS() const -> dx::TopLevelAS *;
    auto GetRayTracingGeometries() const -> const std::vector<RayTracingGeometry> &;
private:
    void RebuildTopLevelAS();
private:
    // clang-format off
	std::vector<MeshRenderer *>		    _meshRendererList;
    std::vector<RayTracingGeometry>     _rayTracingGeometries;
	SharedPtr<dx::TopLevelAS>           _pTopLevelAs;
    std::unique_ptr<dx::AsyncASBuilder> _pAsyncASBuilder;
    bool                                _rebuildTopLevelAS;
    // clang-format on
};