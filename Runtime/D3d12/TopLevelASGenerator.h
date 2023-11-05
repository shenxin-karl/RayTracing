#pragma once
#include "D3dUtils.h"
#include "AccelerationStructure.h"
#include <glm/glm.hpp>

namespace dx {

class TopLevelASGenerator : public NonCopyable {
public:
    TopLevelASGenerator();
    void AddInstance(const ASInstance &instance);
    void AddInstance(ID3D12Resource *pBottomLevelAs,
        const glm::mat4x4 &transform,
        uint32_t instanceID,
        uint32_t hitGroupIndex);
    // If you're building from an old accelerated structure, you need to provide pPreviousResult
    auto Generate(ASBuilder *pUploadHeap, TopLevelAS *pPreviousResult = nullptr, bool cleanUpInstances = true) -> TopLevelAS;

    auto GetInstanceCount() const -> size_t {
        return _instances.size();
    }
public:
private:
    // clang-format off
	bool				    _allowUpdate;
	size_t					_scratchSizeInBytes;
	size_t					_resultSizeInBytes;
	std::vector<ASInstance> _instances;
    // clang-format on
};

}    // namespace dx
