#pragma once
#include "RayTracingGeometry.h"
#include "D3d12/D3dStd.h"
#include "D3d12/Buffer.h"
#include "Foundation/NonCopyable.h"
#include "Foundation/Memory/SharedPtr.hpp"
#include "RayTracingGeometry.h"

class MeshRenderer;
class SceneRayTracingASManager : private NonCopyable {
public:
    SceneRayTracingASManager();
    ~SceneRayTracingASManager();
    void AddMeshRenderer(MeshRenderer *pMeshRenderer);
    void RemoveMeshRenderer(MeshRenderer *pMeshRenderer);
    void BeginBuildBottomLevelAS();
    auto BuildMeshBottomLevelAS() -> std::shared_ptr<RegionTopLevelAS>;
    void EndBuildBottomLevelAS();
    void BuildTopLevelAS(dx::ComputeContext *pComputeContext, const std::shared_ptr<RegionTopLevelAS> &pRegionTopLevelAS);
private:
    // clang-format off
	std::vector<MeshRenderer *>		    _meshRendererList;
    SharedPtr<dx::Buffer>               _pScratchBuffer;
    std::unique_ptr<dx::AsyncASBuilder> _pAsyncASBuilder;
    // clang-format on
};