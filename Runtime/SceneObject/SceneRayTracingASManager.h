#pragma once
#include "D3d12/D3dStd.h"
#include "Foundation/NonCopyable.h"

class MeshRenderer;
class SceneRayTracingASManager : private NonCopyable {
public:
    SceneRayTracingASManager();
    ~SceneRayTracingASManager();
    void AddMeshRenderer(MeshRenderer *pMeshRenderer);
    void RemoveMeshRenderer(MeshRenderer *pMeshRenderer);
    void OnPreRender();
    auto GetTopLevelAS() const -> dx::TopLevelAS *;
private:
    void RebuildTopLevelAS();
private:
    // clang-format off
	std::vector<MeshRenderer *>		    _meshRendererList;
	std::shared_ptr<dx::TopLevelAS>     _pTopLevelAs;
    std::unique_ptr<dx::AsyncASBuilder> _pAsyncASBuilder;
    bool                                _rebuildTopLevelAS;
    // clang-format on
};