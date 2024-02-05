#pragma once
#include "D3dStd.h"
#include "AccelerationStructure.h"
#include <glm/glm.hpp>

namespace dx {

class TopLevelASGenerator : private NonCopyable {
public:
    TopLevelASGenerator();
    void AddInstance(const ASInstance &instance);
    void AddInstance(ID3D12Resource *pBottomLevelAs,
        const glm::mat4x4 &transform,
        uint32_t instanceID,
        uint32_t hitGroupIndex,
        uint16_t instanceMask = 0xff);
    // If you're building from an old accelerated structure, you need to provide pPreviousResult
    auto CommitBuildCommand(IASBuilder *pASBuilder, TopLevelAS *pPreviousResult = nullptr) -> SharedPtr<TopLevelAS>;

    // clang-format off
    struct BuildArgs {
        Device *pDevice;
	    ComputeContext *pComputeContext;
        SharedPtr<Buffer> &pInstanceBuffer; // 构建使用的 InstanceBuffer, Build 函数内会自动维护。允许复用。要求是 Dynamic 类型的
        SharedPtr<Buffer> &pScratchBuffer;  // 构建过程中的临时数据 buffer, Build 函数内会自动维护。允许复用。要求是 Static 类型的
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    };
    // clang-format on

    auto Build(const BuildArgs &buildArgs) -> SharedPtr<TopLevelAS>;

    void Reserve(size_t capacity) {
	    _instances.reserve(capacity);
    }
    auto GetInstanceCount() const -> size_t {
        return _instances.size();
    }
private:
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> _instances;
};

}    // namespace dx
