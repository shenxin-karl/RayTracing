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

    auto GetInstanceCount() const -> size_t {
        return _instances.size();
    }
    void SetInstances(std::vector<ASInstance> instances) {
        _instances = std::move(instances);
    }
private:
    std::vector<ASInstance> _instances;
};

}    // namespace dx
